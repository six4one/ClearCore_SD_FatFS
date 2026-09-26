#include "ClearCore.h"
#include "ff.h"
#include "diskio.h"
#include "SDFile.h"

using namespace ClearCore;

namespace {

	constexpr const char *APPEND_TEST_FILE  = "append_test.txt";
	constexpr const char *REWRITE_TEST_FILE = "rewrite_test.txt";
	constexpr const char *DELETE_TEST_FILE  = "delete_test.txt";
	constexpr const char *CREATE_TEST_FILE  = "create_test.txt";

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

	static void PrintResult(const char *operation, FRESULT result)
	{
		Print(operation);
		Print(": ");

		if (result == FR_OK) {
			Print("SUCCESS");
		}
		else {
			Print("FAILED (FRESULT=");
			PrintNumber(result);
			Print(")");
		}

		Print("\r\n");
	}

	static void PrintExists(const char *path, bool exists)
	{
		Print(path);
		Print(": ");

		if (exists) {
			Print("EXISTS\r\n");
		}
		else {
			Print("DOES NOT EXIST\r\n");
		}
	}

	static void StopWithAliveMessage(const char *message)
	{
		Print(message);

		while (true) {
			Delay_ms(1000);
			Print("ALIVE\r\n");
		}
	}

	static bool ReadAndPrintFile(const char *path)
	{
		FIL file;
		FRESULT result;
		UINT bytesRead;
		char buffer[256];

		Print("\r\nReading ");
		Print(path);
		Print("...\r\n");

		result = f_open(&file, path, FA_READ);

		if (result != FR_OK) {
			PrintResult("  f_open", result);
			return false;
		}

		result = f_read(
		&file,
		buffer,
		sizeof(buffer) - 1,
		&bytesRead
		);

		if (result != FR_OK) {
			PrintResult("  f_read", result);
			f_close(&file);
			return false;
		}

		buffer[bytesRead] = '\0';

		result = f_close(&file);

		if (result != FR_OK) {
			PrintResult("  f_close", result);
			return false;
		}

		Print("  Contents:\r\n");
		Print("  --------------------\r\n");
		Print(buffer);
		Print("\r\n");
		Print("  --------------------\r\n");

		Print("  Bytes read: ");
		PrintNumber(bytesRead);
		Print("\r\n");

		return true;
	}

	static bool VerifyFileContents(const char *path, const char *expected)
	{
		FIL file;
		FRESULT result;
		UINT bytesRead;
		char buffer[256];

		result = f_open(&file, path, FA_READ);

		if (result != FR_OK) {
			PrintResult("  VERIFY f_open", result);
			return false;
		}

		result = f_read(
		&file,
		buffer,
		sizeof(buffer) - 1,
		&bytesRead
		);

		if (result != FR_OK) {
			PrintResult("  VERIFY f_read", result);
			f_close(&file);
			return false;
		}

		buffer[bytesRead] = '\0';

		result = f_close(&file);

		if (result != FR_OK) {
			PrintResult("  VERIFY f_close", result);
			return false;
		}

		bool match = true;

		const char *actual = buffer;
		const char *wanted = expected;

		while (*actual || *wanted) {
			if (*actual != *wanted) {
				match = false;
				break;
			}

			++actual;
			++wanted;
		}

		if (match) {
			Print("  VERIFY: PASS\r\n");
		}
		else {
			Print("  VERIFY: FAIL\r\n");
			Print("  Expected:\r\n");
			Print(expected);
			Print("\r\n");
			Print("  Actual:\r\n");
			Print(buffer);
			Print("\r\n");
		}

		return match;
	}

	static bool VerifyFileDoesNotExist(const char *path)
	{
		bool exists = false;
		FRESULT result = SD_Exists(path, &exists);

		PrintResult("  SD_Exists", result);

		if (result != FR_OK) {
			return false;
		}

		if (!exists) {
			Print("  VERIFY: PASS - file does not exist\r\n");
			return true;
		}

		Print("  VERIFY: FAIL - file still exists\r\n");
		return false;
	}

} // namespace

int main()
{
	// Allow time to open the serial monitor after reset.
	Delay_ms(10000);

	ConnectorUsb.PortOpen();

	FATFS fs;
	FRESULT result;

	Print("\r\n");
	Print("============================================\r\n");
	Print(" ClearCore SD FILE PRIMITIVE TEST HARNESS\r\n");
	Print("============================================\r\n");
	Print("\r\n");

	// ------------------------------------------------------------
	// SD initialization
	// ------------------------------------------------------------

	Print("SD INITIALIZATION\r\n");
	Print("-----------------\r\n");

	DSTATUS status = disk_initialize(0);

	Print("disk_initialize: ");
	PrintNumber(status);
	Print("\r\n");

	if (status & STA_NOINIT) {
		StopWithAliveMessage("SD INIT FAILED\r\n");
	}

	Print("SD initialization: SUCCESS\r\n");

	// ------------------------------------------------------------
	// Mount filesystem
	// ------------------------------------------------------------

	Print("\r\nMOUNT FILESYSTEM\r\n");
	Print("-----------------\r\n");

	result = f_mount(&fs, "", 1);

	PrintResult("f_mount", result);

	if (result != FR_OK) {
		StopWithAliveMessage("MOUNT FAILED\r\n");
	}

	// ------------------------------------------------------------
	// Establish a clean test environment
	// ------------------------------------------------------------

	Print("\r\nCLEAN TEST ENVIRONMENT\r\n");
	Print("----------------------\r\n");

	const char *testFiles[] = {
		APPEND_TEST_FILE,
		REWRITE_TEST_FILE,
		DELETE_TEST_FILE,
		CREATE_TEST_FILE
	};

	for (unsigned int i = 0;
	i < sizeof(testFiles) / sizeof(testFiles[0]);
	++i) {

		bool exists = false;

		result = SD_Exists(testFiles[i], &exists);

		if (result != FR_OK) {
			PrintResult("SD_Exists", result);
			StopWithAliveMessage("CLEANUP CHECK FAILED\r\n");
		}

		if (exists) {
			Print("Removing previous test file: ");
			Print(testFiles[i]);
			Print("\r\n");

			result = SD_Delete(testFiles[i]);

			PrintResult("  SD_Delete", result);

			if (result != FR_OK) {
				StopWithAliveMessage("TEST ENVIRONMENT CLEANUP FAILED\r\n");
			}
		}
	}

	Print("Test environment ready.\r\n");

	// ------------------------------------------------------------
	// TEST 1: Append
	// ------------------------------------------------------------

	Print("\r\n");
	Print("TEST 1 - APPEND\r\n");
	Print("----------------\r\n");

	const char *append1 = "APPEND RECORD ONE\r\n";
	const char *append2 = "APPEND RECORD TWO\r\n";

	result = SD_Append(
	APPEND_TEST_FILE,
	append1,
	static_cast<UINT>(strlen(append1))
	);

	PrintResult("Append record #1", result);

	if (result != FR_OK) {
		StopWithAliveMessage("APPEND TEST FAILED\r\n");
	}

	result = SD_Append(
	APPEND_TEST_FILE,
	append2,
	static_cast<UINT>(strlen(append2))
	);

	PrintResult("Append record #2", result);

	if (result != FR_OK) {
		StopWithAliveMessage("APPEND TEST FAILED\r\n");
	}

	if (!ReadAndPrintFile(APPEND_TEST_FILE)) {
		StopWithAliveMessage("APPEND READBACK FAILED\r\n");
	}

	if (!VerifyFileContents(
	APPEND_TEST_FILE,
	"APPEND RECORD ONE\r\nAPPEND RECORD TWO\r\n")) {

		StopWithAliveMessage("APPEND CONTENT VERIFICATION FAILED\r\n");
	}

	Print("TEST 1 RESULT: PASS\r\n");

	// ------------------------------------------------------------
	// TEST 2: Rewrite
	// ------------------------------------------------------------

	Print("\r\n");
	Print("TEST 2 - REWRITE\r\n");
	Print("-----------------\r\n");

	const char *rewrite1 = "FIRST CONTENT\r\n";
	const char *rewrite2 = "SECOND CONTENT\r\n";

	result = SD_Rewrite(
	REWRITE_TEST_FILE,
	rewrite1,
	static_cast<UINT>(strlen(rewrite1))
	);

	PrintResult("Rewrite #1", result);

	if (result != FR_OK) {
		StopWithAliveMessage("REWRITE TEST FAILED\r\n");
	}

	if (!VerifyFileContents(REWRITE_TEST_FILE, rewrite1)) {
		StopWithAliveMessage("REWRITE #1 VERIFICATION FAILED\r\n");
	}

	result = SD_Rewrite(
	REWRITE_TEST_FILE,
	rewrite2,
	static_cast<UINT>(strlen(rewrite2))
	);

	PrintResult("Rewrite #2", result);

	if (result != FR_OK) {
		StopWithAliveMessage("REWRITE TEST FAILED\r\n");
	}

	if (!ReadAndPrintFile(REWRITE_TEST_FILE)) {
		StopWithAliveMessage("REWRITE READBACK FAILED\r\n");
	}

	if (!VerifyFileContents(REWRITE_TEST_FILE, rewrite2)) {
		StopWithAliveMessage("REWRITE CONTENT VERIFICATION FAILED\r\n");
	}

	Print("TEST 2 RESULT: PASS\r\n");

	// ------------------------------------------------------------
	// TEST 3: Delete
	// ------------------------------------------------------------

	Print("\r\n");
	Print("TEST 3 - DELETE\r\n");
	Print("----------------\r\n");

	const char *deleteContent = "THIS FILE WILL BE DELETED\r\n";

	result = SD_Rewrite(
	DELETE_TEST_FILE,
	deleteContent,
	static_cast<UINT>(strlen(deleteContent))
	);

	PrintResult("Create delete-test file", result);

	if (result != FR_OK) {
		StopWithAliveMessage("DELETE TEST SETUP FAILED\r\n");
	}

	bool exists = false;

	result = SD_Exists(DELETE_TEST_FILE, &exists);

	PrintResult("Check file before delete", result);
	PrintExists(DELETE_TEST_FILE, exists);

	if (result != FR_OK || !exists) {
		StopWithAliveMessage("DELETE TEST PRECONDITION FAILED\r\n");
	}

	result = SD_Delete(DELETE_TEST_FILE);

	PrintResult("Delete file", result);

	if (result != FR_OK) {
		StopWithAliveMessage("DELETE OPERATION FAILED\r\n");
	}

	if (!VerifyFileDoesNotExist(DELETE_TEST_FILE)) {
		StopWithAliveMessage("DELETE VERIFICATION FAILED\r\n");
	}

	// A second delete should demonstrate the expected FR_NO_FILE
	// behavior for a file that no longer exists.
	Print("Attempting second delete (expected FR_NO_FILE)...\r\n");

	result = SD_Delete(DELETE_TEST_FILE);

	PrintResult("Second delete", result);

	if (result != FR_NO_FILE) {
		StopWithAliveMessage("UNEXPECTED SECOND DELETE RESULT\r\n");
	}

	Print("Second delete: expected FR_NO_FILE received.\r\n");
	Print("TEST 3 RESULT: PASS\r\n");

	// ------------------------------------------------------------
	// TEST 4: Create-on-demand through Append
	// ------------------------------------------------------------

	Print("\r\n");
	Print("TEST 4 - CREATE ON DEMAND\r\n");
	Print("--------------------------\r\n");

	Print("Confirming file does not exist before append...\r\n");

	if (!VerifyFileDoesNotExist(CREATE_TEST_FILE)) {
		StopWithAliveMessage("CREATE-ON-DEMAND PRECONDITION FAILED\r\n");
	}

	const char *createContent = "CREATED BY APPEND\r\n";

	result = SD_Append(
	CREATE_TEST_FILE,
	createContent,
	static_cast<UINT>(strlen(createContent))
	);

	PrintResult("Append to non-existent file", result);

	if (result != FR_OK) {
		StopWithAliveMessage("CREATE-ON-DEMAND APPEND FAILED\r\n");
	}

	if (!ReadAndPrintFile(CREATE_TEST_FILE)) {
		StopWithAliveMessage("CREATE-ON-DEMAND READBACK FAILED\r\n");
	}

	if (!VerifyFileContents(CREATE_TEST_FILE, createContent)) {
		StopWithAliveMessage("CREATE-ON-DEMAND VERIFICATION FAILED\r\n");
	}

	Print("TEST 4 RESULT: PASS\r\n");

	// ------------------------------------------------------------
	// Final result
	// ------------------------------------------------------------

	Print("\r\n");
	Print("============================================\r\n");
	Print(" ALL SD FILE PRIMITIVE TESTS PASSED\r\n");
	Print("============================================\r\n");

	Print("\r\nTest files remain on the SD card for inspection.\r\n");
	Print("System alive.\r\n");

	while (true) {
		Delay_ms(1000);
		Print("ALIVE\r\n");
	}
}
