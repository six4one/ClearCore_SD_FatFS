#include "ClearCore.h"
#include "ff.h"
#include "diskio.h"

using namespace ClearCore;

namespace {

	// This is the file used by the Lockout-state read test.
	// The Elevator project will later supply its canonical filename
	// through Elevator_Params.h.
	constexpr const char *LOCKOUT_TEST_FILE = "lockout.json";

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

	static void StopWithAliveMessage(const char *message)
	{
		Print(message);

		while (true) {
			Delay_ms(1000);
			Print("ALIVE\r\n");
		}
	}

} // namespace

int main()
{
	Delay_ms(10000);

	ConnectorUsb.PortOpen();

	FATFS fs;
	FIL file;

	FRESULT result;
	UINT bytesRead;

	// Leave room for a terminating '\0' after the file contents.
	char readBuffer[128];

	Print("\r\n");
	Print("SD LOCKOUT STATE READ TEST\r\n");
	Print("===========================\r\n");

	// ---------------------------------------------------------
	// Initialize SD card
	// ---------------------------------------------------------

	DSTATUS status = disk_initialize(0);

	Print("disk_initialize: ");
	PrintNumber(status);
	Print("\r\n");

	if (status & STA_NOINIT) {
		StopWithAliveMessage("SD INIT FAILED\r\n");
	}

	// ---------------------------------------------------------
	// Mount filesystem
	// ---------------------------------------------------------

	Print("\r\nBEFORE f_mount\r\n");

	result = f_mount(&fs, "", 1);

	Print("AFTER f_mount\r\n");

	Print("f_mount result: ");
	PrintNumber(result);
	Print("\r\n");

	if (result != FR_OK) {
		StopWithAliveMessage("MOUNT FAILED\r\n");
	}

	// ---------------------------------------------------------
	// Open lockout state file for reading
	// ---------------------------------------------------------

	Print("\r\nBEFORE f_open(lockout.json)\r\n");

	result = f_open(
	&file,
	LOCKOUT_TEST_FILE,
	FA_READ
	);

	Print("AFTER f_open\r\n");

	Print("f_open result: ");
	PrintNumber(result);
	Print("\r\n");

	if (result != FR_OK) {
		StopWithAliveMessage("OPEN FAILED\r\n");
	}

	// ---------------------------------------------------------
	// Read lockout state file
	// ---------------------------------------------------------

	for (UINT i = 0; i < sizeof(readBuffer); ++i) {
		readBuffer[i] = '\0';
	}

	Print("\r\nBEFORE f_read\r\n");

	result = f_read(
	&file,
	readBuffer,
	sizeof(readBuffer) - 1,
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
		f_close(&file);
		StopWithAliveMessage("READ FAILED\r\n");
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
		StopWithAliveMessage("CLOSE FAILED\r\n");
	}

	// ---------------------------------------------------------
	// Display file contents
	// ---------------------------------------------------------

	Print("\r\n===== lockout.json =====\r\n");

	for (UINT i = 0; i < bytesRead; ++i) {
		ConnectorUsb.SendChar(
		static_cast<uint8_t>(readBuffer[i])
		);
	}

	Print("===== END FILE =====\r\n");

	// ---------------------------------------------------------
	// Test result
	// ---------------------------------------------------------

	if (bytesRead > 0) {
		Print("\r\n*** LOCKOUT FILE READ PASS ***\r\n");
	}
	else {
		Print("\r\n*** LOCKOUT FILE READ FAIL: EMPTY FILE ***\r\n");
	}

	while (true) {
		Delay_ms(1000);
		Print("ALIVE\r\n");
	}
}
