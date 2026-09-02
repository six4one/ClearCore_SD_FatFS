#include "ClearCore.h"
#include "ff.h"
#include "diskio.h"

using namespace ClearCore;

static void Print(const char *s)
{
	while (*s) {
		ConnectorUsb.SendChar(static_cast<uint8_t>(*s++));
	}
}

static void PrintNumber(DWORD value)
{
	char buffer[16];
	int i = 15;

	buffer[i] = '\0';

	if (value == 0) {
		Print("0");
		return;
	}

	while (value > 0 && i > 0) {
		buffer[--i] = '0' + (value % 10);
		value /= 10;
	}

	Print(&buffer[i]);
}

int main()
{
	Delay_ms(10000);

	ConnectorUsb.PortOpen();

	FATFS fs;
	FIL file;

	FRESULT result;
	UINT bytesWritten;
	UINT bytesRead;

	const char writeText[] =
	"ChatGPT and Fausto wrote this!\r\n";

	char readBuffer[64];

	Print("\r\n");
	Print("SD WRITE TEST\r\n");
	Print("=============\r\n");

	// ---------------------------------------------------------
	// Initialize SD card
	// ---------------------------------------------------------

	DSTATUS status = disk_initialize(0);

	Print("disk_initialize: ");
	PrintNumber(status);
	Print("\r\n");

	if (status & STA_NOINIT) {
		Print("SD INIT FAILED\r\n");

		while (true) {
			Delay_ms(1000);
			Print("ALIVE\r\n");
		}
	}

	// ---------------------------------------------------------
	// Mount filesystem
	// ---------------------------------------------------------

	Print("BEFORE f_mount\r\n");

	result = f_mount(&fs, "", 1);

	Print("AFTER f_mount\r\n");

	Print("f_mount result: ");
	PrintNumber(result);
	Print("\r\n");

	if (result != FR_OK) {
		Print("MOUNT FAILED\r\n");

		while (true) {
			Delay_ms(1000);
			Print("ALIVE\r\n");
		}
	}

	// ---------------------------------------------------------
	// Create / open WRITE.txt
	// ---------------------------------------------------------

	Print("\r\nBEFORE f_open(WRITE.txt)\r\n");

	result = f_open(
	&file,
	"WRITE.txt",
	FA_WRITE | FA_CREATE_ALWAYS
	);

	Print("AFTER f_open\r\n");

	Print("f_open result: ");
	PrintNumber(result);
	Print("\r\n");

	if (result != FR_OK) {
		Print("OPEN FAILED\r\n");

		while (true) {
			Delay_ms(1000);
			Print("ALIVE\r\n");
		}
	}

	// ---------------------------------------------------------
	// Write file
	// ---------------------------------------------------------

	Print("\r\nBEFORE f_write\r\n");

	result = f_write(
	&file,
	writeText,
	sizeof(writeText) - 1,
	&bytesWritten
	);

	Print("AFTER f_write\r\n");

	Print("f_write result: ");
	PrintNumber(result);
	Print("\r\n");

	Print("bytes written: ");
	PrintNumber(bytesWritten);
	Print("\r\n");

	if (result != FR_OK ||
	bytesWritten != sizeof(writeText) - 1) {

		Print("WRITE FAILED\r\n");

		f_close(&file);

		while (true) {
			Delay_ms(1000);
			Print("ALIVE\r\n");
		}
	}

	// ---------------------------------------------------------
	// Close file
	// ---------------------------------------------------------

	Print("\r\nBEFORE f_close\r\n");

	result = f_close(&file);

	Print("AFTER f_close\r\n");

	Print("f_close result: ");
	PrintNumber(result);
	Print("\r\n");

	if (result != FR_OK) {
		Print("CLOSE FAILED\r\n");

		while (true) {
			Delay_ms(1000);
			Print("ALIVE\r\n");
		}
	}

	// ---------------------------------------------------------
	// Reopen file for reading
	// ---------------------------------------------------------

	Print("\r\nBEFORE REOPEN\r\n");

	result = f_open(
	&file,
	"WRITE.txt",
	FA_READ
	);

	Print("AFTER REOPEN\r\n");

	Print("reopen result: ");
	PrintNumber(result);
	Print("\r\n");

	if (result != FR_OK) {
		Print("REOPEN FAILED\r\n");

		while (true) {
			Delay_ms(1000);
			Print("ALIVE\r\n");
		}
	}

	// ---------------------------------------------------------
	// Read file back
	// ---------------------------------------------------------

	for (UINT i = 0; i < sizeof(readBuffer); ++i) {
		readBuffer[i] = '\0';
	}

	Print("\r\nBEFORE f_read\r\n");

	result = f_read(
	&file,
	readBuffer,
	sizeof(writeText) - 1,
	&bytesRead
	);

	Print("AFTER f_read\r\n");

	Print("f_read result: ");
	PrintNumber(result);
	Print("\r\n");

	Print("bytes read: ");
	PrintNumber(bytesRead);
	Print("\r\n");

	if (result != FR_OK) {
		Print("READ-BACK FAILED\r\n");

		f_close(&file);

		while (true) {
			Delay_ms(1000);
			Print("ALIVE\r\n");
		}
	}

	// ---------------------------------------------------------
	// Display read-back contents
	// ---------------------------------------------------------

	Print("\r\n===== WRITE.txt READ-BACK =====\r\n");

	for (UINT i = 0; i < bytesRead; ++i) {
		ConnectorUsb.SendChar(
		static_cast<uint8_t>(readBuffer[i])
		);
	}

	Print("===== END READ-BACK =====\r\n");

	// ---------------------------------------------------------
	// Verify contents
	// ---------------------------------------------------------

	bool match = true;

	for (UINT i = 0; i < bytesRead; ++i) {
		if (readBuffer[i] != writeText[i]) {
			match = false;
			break;
		}
	}

	if (bytesRead != sizeof(writeText) - 1) {
		match = false;
	}

	if (match) {
		Print("\r\n*** WRITE/READ-BACK PASS ***\r\n");
	}
	else {
		Print("\r\n*** WRITE/READ-BACK FAIL ***\r\n");
	}

	f_close(&file);

	while (true) {
		Delay_ms(1000);
		Print("ALIVE\r\n");
	}
}