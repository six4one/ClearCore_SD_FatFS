/*
 * FatFs disk I/O adapter for the Teknic ClearCore micro-SD interface.
 *
 * This is the ClearCore-specific portion of the port.
 *
 * First experiment:
 *   - read-only
 *   - 512-byte sectors
 *   - synchronous SPI transfers
 *   - no DMA
 */

#include "ClearCore.h"
#include "ff.h"
#include "diskio.h"

using namespace ClearCore;

namespace {

constexpr BYTE CMD0   = 0;
constexpr BYTE CMD8   = 8;
constexpr BYTE CMD16  = 16;
constexpr BYTE CMD17  = 17;
constexpr BYTE CMD24  = 24;
constexpr BYTE CMD55  = 55;
constexpr BYTE CMD58  = 58;
constexpr BYTE ACMD41 = 41;

constexpr BYTE DATA_TOKEN = 0xFE;

bool cardInitialized = false;
bool blockAddressing = false;

static BYTE spi(BYTE value)
{
    return SdCard.SpiTransferData(value);
}

static void select()
{
	SdCard.SpiSsMode(SerialBase::LINE_ON);
}

static void deselect()
{
	SdCard.SpiSsMode(SerialBase::LINE_OFF);
	spi(0xFF);
}

static void clockBytes(UINT count)
{
    while (count--) {
        spi(0xFF);
    }
}

/*
 * Send an SD command while CS is already asserted.
 */
static BYTE sendCommand(BYTE command, DWORD argument, BYTE crc)
{
    spi(static_cast<BYTE>(0x40 | command));
    spi(static_cast<BYTE>(argument >> 24));
    spi(static_cast<BYTE>(argument >> 16));
    spi(static_cast<BYTE>(argument >> 8));
    spi(static_cast<BYTE>(argument));
    spi(crc);

    for (UINT i = 0; i < 10; ++i) {
        BYTE r = spi(0xFF);

        if ((r & 0x80) == 0) {
            return r;
        }
    }

    return 0xFF;
}

static bool waitForToken(BYTE token, UINT limit)
{
    while (limit--) {
        if (spi(0xFF) == token) {
            return true;
        }
    }

    return false;
}

static bool initializeCard()
{
    cardInitialized = false;
    blockAddressing = false;

    /*
     * SD SPI startup requirements.
     * ClearCore's SdCardDriver has already configured the SD pins
     * and SERCOM for SPI during ClearCore initialization.
     */
    SdCard.DataOrder(SerialBase::COM_MSB_FIRST);
    SdCard.SpiClock(SerialBase::SCK_LOW, SerialBase::LEAD_SAMPLE);

    /* Slow clock during card identification. */
    SdCard.Speed(400000);

    deselect();

    /*
     * >= 74 clocks with CS high and MOSI high.
     */
    clockBytes(10);

    /*
     * CMD0 -> idle state.
     */
    select();
    BYTE r1 = sendCommand(CMD0, 0, 0x95);
    deselect();

    if (r1 != 0x01) {
        return false;
    }

    /*
     * CMD8 -> identify SD v2 and check voltage/pattern.
     */
    select();
    r1 = sendCommand(CMD8, 0x000001AAUL, 0x87);

    if (r1 == 0x01) {
        BYTE r7[4];

        for (UINT i = 0; i < 4; ++i) {
            r7[i] = spi(0xFF);
        }

        deselect();

        if (r7[2] != 0x01 || r7[3] != 0xAA) {
            return false;
        }

        /*
         * SD v2: ACMD41 with HCS until card leaves idle state.
         */
        bool ready = false;

        for (UINT attempt = 0; attempt < 1000; ++attempt) {
            select();
            BYTE r55 = sendCommand(CMD55, 0, 0x01);
            deselect();

            if (r55 > 0x01) {
                return false;
            }

            select();
            BYTE ra = sendCommand(ACMD41, 0x40000000UL, 0x01);
            deselect();

            if (ra == 0x00) {
                ready = true;
                break;
            }
        }

        if (!ready) {
            return false;
        }

        /*
         * CMD58 -> OCR. CCS bit selects block vs byte addressing.
         */
        select();
        r1 = sendCommand(CMD58, 0, 0x01);

        if (r1 != 0x00) {
            deselect();
            return false;
        }

        BYTE ocr[4];

        for (UINT i = 0; i < 4; ++i) {
            ocr[i] = spi(0xFF);
        }

        deselect();

        blockAddressing = (ocr[0] & 0x40) != 0;
    }
    else {
        /*
         * SD v1 fallback.
         */
        deselect();

        bool ready = false;

        for (UINT attempt = 0; attempt < 1000; ++attempt) {
            select();
            BYTE r55 = sendCommand(CMD55, 0, 0x01);
            deselect();

            select();
            BYTE ra = sendCommand(ACMD41, 0, 0x01);
            deselect();

            if (ra == 0x00) {
                ready = true;
                break;
            }

            if (r55 > 0x01 && ra != 0x01) {
                return false;
            }
        }

        if (!ready) {
            return false;
        }

        /*
         * SDSC cards use byte addressing and 512-byte blocks.
         */
        select();
        r1 = sendCommand(CMD16, 512, 0x01);
        deselect();

        if (r1 != 0x00) {
            return false;
        }
    }

    /*
     * Normal operating clock.
     */
    SdCard.Speed(10000000);

    cardInitialized = true;
    return true;
}

static bool readBlock(LBA_t sector, BYTE *buffer)
{
    DWORD address;

    if (blockAddressing) {
        address = static_cast<DWORD>(sector);
    }
    else {
        address = static_cast<DWORD>(sector * 512UL);
    }

    select();

    BYTE r1 = sendCommand(CMD17, address, 0x01);

    if (r1 != 0x00) {
        deselect();
        return false;
    }

    if (!waitForToken(DATA_TOKEN, 100000)) {
        deselect();
        return false;
    }

    /*
     * Synchronous transfer deliberately used for this first experiment.
     * This avoids introducing DMA/alignment issues while proving the
     * filesystem path.
     */
    for (UINT i = 0; i < 512; ++i) {
        buffer[i] = spi(0xFF);
    }

    /* CRC16 -- ignored here. */
    spi(0xFF);
    spi(0xFF);

    deselect();
    return true;
}

static bool writeBlock(LBA_t sector, const BYTE *buffer)
{
    DWORD address;

    if (blockAddressing) {
        address = static_cast<DWORD>(sector);
    }
    else {
        address = static_cast<DWORD>(sector * 512UL);
    }

    select();

    /*
     * CMD24 = WRITE_BLOCK
     */
    BYTE r1 = sendCommand(CMD24, address, 0x01);

    if (r1 != 0x00) {
        deselect();
        return false;
    }

    /*
     * Give the card one byte before the data token.
     */
    spi(0xFF);

    /*
     * Start single-block data transfer.
     */
    spi(DATA_TOKEN);

    /*
     * Send exactly one 512-byte sector.
     */
    for (UINT i = 0; i < 512; ++i) {
        spi(buffer[i]);
    }

    /*
     * CRC16.
     *
     * CRC checking is disabled during normal SPI operation,
     * so these bytes can be dummy values.
     */
    spi(0xFF);
    spi(0xFF);

    /*
     * The card returns a data-response token.
     *
     * Bits 4:0 should be 00101 for "data accepted".
     */
    BYTE response = spi(0xFF);

    if ((response & 0x1F) != 0x05) {
        deselect();
        return false;
    }

    /*
     * The card remains busy (DO held low) while programming
     * the flash.  Keep CS asserted while waiting.
     */
    if (!waitForToken(0xFF, 1000000)) {
        deselect();
        return false;
    }

    deselect();

    return true;
}

} // namespace

extern "C" {

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != 0) {
        return STA_NOINIT;
    }

    return cardInitialized ? 0 : STA_NOINIT;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0) {
        return STA_NOINIT;
    }

    return initializeCard() ? 0 : STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != 0 || buff == nullptr || count == 0) {
        return RES_PARERR;
    }

    if (!cardInitialized) {
        return RES_NOTRDY;
    }

    while (count--) {
        if (!readBlock(sector, buff)) {
            return RES_ERROR;
        }

        ++sector;
        buff += 512;
    }

    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
	if (pdrv != 0 || buff == nullptr || count == 0) {
		return RES_PARERR;
	}

	if (!cardInitialized) {
		return RES_NOTRDY;
	}

	while (count--) {

		if (!writeBlock(sector, buff)) {
			return RES_ERROR;
		}

		++sector;
		buff += 512;
	}

	return RES_OK;
}

/*
 * With fixed 512-byte sectors and read-only FatFs, this first test
 * does not require any useful ioctl operations.
 */
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != 0) {
        return RES_PARERR;
    }

    if (!cardInitialized) {
        return RES_NOTRDY;
    }

    switch (cmd) {
        case CTRL_SYNC:
            select();

            /*
             * Wait for the card to stop being busy.
             */
            for (UINT i = 0; i < 100000; ++i) {
                if (spi(0xFF) == 0xFF) {
                    deselect();
                    return RES_OK;
                }
            }

            deselect();
            return RES_ERROR;

        case GET_SECTOR_SIZE:
            if (buff == nullptr) {
                return RES_PARERR;
            }

            *static_cast<WORD *>(buff) = 512;
            return RES_OK;

        case GET_BLOCK_SIZE:
            if (buff == nullptr) {
                return RES_PARERR;
            }

            *static_cast<DWORD *>(buff) = 1;
            return RES_OK;

        default:
            return RES_PARERR;
    }
}



} // extern "C"
