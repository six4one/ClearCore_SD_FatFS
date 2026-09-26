#include "ClearCore.h"
#include "ff.h"
#include "diskio.h"

using namespace ClearCore;

namespace {

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

	char readBuffer[128];

	Print("\r\n");
	Print("SD LOCKOUT FILE READ DIAGNOSTIC\r\n");
	Print("================================\r\n");

	DSTATUS status = disk_initialize(0);

	Print("disk_initialize: ");
	PrintNumber(status);
	Print("\r\n");

	if (status & STA_NOINIT) {
		StopWithAliveMessage("SD INIT FAILED\r\n");
	}

	Print("\r\nBEFORE f_mount\r\n");

	result = f_mount(&fs, "", 1);

	Print("AFTER f_mount\r\n");

	Print("f_mount result: ");
	PrintNumber(result);
	Print("\r\n");

	if (result != FR_OK) {
		StopWithAliveMessage("MOUNT FAILED\r\n");
	}

	Print("\r\n===== ROOT DIRECTORY =====\r\n");

	DIR directory;
	FILINFO info;

	result = f_opendir(&directory, "");

	Print("f_opendir result: ");
	PrintNumber(result);
	Print("\r\n");

	if (result == FR_OK) {
		while (true) {
			result = f_readdir(&directory, &info);

			if (result != FR_OK) {
				Print("f_readdir result: ");
				PrintNumber(result);
				Print("\r\n");
				break;
			}

			if (info.fname[0] == '\0') {
				break;
			}

			if (info.fattrib & AM_DIR) {
				Print("[DIR ] ");
			}
			else {
				Print("[FILE] ");
			}

			Print(info.fname);
			Print("\r\n");
		}

		f_closedir(&directory);
	}
	else {
		Print("ROOT DIRECTORY OPEN FAILED\r\n");
	}

	Print("===== END DIRECTORY =====\r\n");

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
		Print("lockout.json OPEN FAILED\r\n");
		StopWithAliveMessage("READ TEST ABORTED\r\n");
	}

	Print("lockout.json OPENED SUCCESSFULLY\r\n");

	Print("\r\n===== FILE CONTENT =====\r\n");

	result = f_read(
	&file,
	readBuffer,
	sizeof(readBuffer) - 1,
	&bytesRead
	);

	if (result != FR_OK) {
		Print("f_read result: ");
		PrintNumber(result);
		Print("\r\n");

		f_close(&file);

		StopWithAliveMessage("FILE READ FAILED\r\n");
	}

	readBuffer[bytesRead] = '\0';

	Print("f_read result: ");
	PrintNumber(result);
	Print("\r\n");

	Print("bytesRead: ");
	PrintNumber(bytesRead);
	Print("\r\n");

	Print("\r\n");
	Print(readBuffer);
	Print("\r\n");

	Print("===== END FILE CONTENT =====\r\n");

	f_close(&file);

	Print("\r\nLOCKOUT FILE READ SUCCESSFUL\r\n");

	while (true) {
		Delay_ms(1000);
		Print("ALIVE\r\n");
	}
}