#include "ClearCore.h"
#include "ff.h"
#include "diskio.h"
#include "SDFile.h"
#include <string.h>

using namespace ClearCore;

namespace {
	constexpr const char *APPEND_TEST_FILE="append_test.txt";
	constexpr const char *REWRITE_TEST_FILE="rewrite_test.txt";
	constexpr const char *DELETE_TEST_FILE="delete_test.txt";
	constexpr const char *CREATE_TEST_FILE="create_test.txt";
	constexpr const char *EMPTY_TEST_FILE="empty_test.txt";

	void Print(const char *s) {
		while (*s) ConnectorUsb.SendChar(static_cast<uint8_t>(*s++));
	}
	void PrintNumber(DWORD value) {
		char b[16]; int i=15; b[i]='\0';
		if (!value) { Print("0"); return; }
		while (value && i>0) { b[--i]='0'+(value%10); value/=10; }
		Print(&b[i]);
	}
	void PrintResult(const char *op, FRESULT r) {
		Print(op); Print(": ");
		if (r==FR_OK) Print("SUCCESS");
		else { Print("FAILED (FRESULT="); PrintNumber(r); Print(")"); }
		Print("\r\n");
	}
	void Stop(const char *s) {
		Print(s);
		while (true) { Delay_ms(1000); Print("ALIVE\r\n"); }
	}
	bool VerifyContents(const char *path, const char *expected) {
		FIL f; UINT n=0; char b[256]; FRESULT r=f_open(&f,path,FA_READ);
		if(r!=FR_OK){PrintResult("  VERIFY f_open",r);return false;}
		r=f_read(&f,b,sizeof(b)-1,&n);
		if(r!=FR_OK){PrintResult("  VERIFY f_read",r);f_close(&f);return false;}
		b[n]='\0'; r=f_close(&f);
		if(r!=FR_OK){PrintResult("  VERIFY f_close",r);return false;}
		bool ok=(strcmp(b,expected)==0);
		if(ok) Print("  VERIFY: PASS\r\n");
		else { Print("  VERIFY: FAIL\r\n"); Print("  Expected: "); Print(expected); Print("\r\n  Actual: "); Print(b); Print("\r\n"); }
		return ok;
	}
	bool VerifyAbsent(const char *path) {
		bool exists=false; FRESULT r=SD_Exists(path,&exists);
		PrintResult("  SD_Exists",r);
		if(r==FR_OK && !exists){Print("  VERIFY: PASS - file does not exist\r\n");return true;}
		Print("  VERIFY: FAIL\r\n"); return false;
	}
	bool VerifySize(const char *path, FSIZE_t expected) {
		FILINFO i; FRESULT r=f_stat(path,&i); PrintResult("  f_stat",r);
		if(r!=FR_OK)return false;
		Print("  File size: ");PrintNumber((DWORD)i.fsize);Print(" bytes\r\n");
		bool ok=(i.fsize==expected);Print(ok?"  VERIFY: PASS\r\n":"  VERIFY: FAIL\r\n");return ok;
	}
	bool ReadAndPrint(const char *path) {
		FIL f;UINT n=0;char b[256];FRESULT r=f_open(&f,path,FA_READ);
		if(r!=FR_OK){PrintResult("  f_open",r);return false;}
		r=f_read(&f,b,sizeof(b)-1,&n);
		if(r!=FR_OK){PrintResult("  f_read",r);f_close(&f);return false;}
		b[n]='\0';r=f_close(&f);if(r!=FR_OK){PrintResult("  f_close",r);return false;}
		Print("  Contents:\r\n  --------------------\r\n");Print(b);
		Print("\r\n  --------------------\r\n  Bytes read: ");PrintNumber(n);Print("\r\n");return true;
	}
}

int main() {
	// Preserve the original 10-second serial-monitor activation delay.
	Delay_ms(10000);
	ConnectorUsb.PortOpen();

	FATFS fs; FRESULT r;

	Print("\r\n================================================\r\n");
	Print(" ClearCore SD FILE PRIMITIVE TEST HARNESS\r\n");
	Print("================================================\r\n\r\n");

	// ------------------------------------------------------------
	// C: physical SD-card presence / initialization
	// ------------------------------------------------------------
	Print("TEST C - SD CARD DETECTION\r\n");
	Print("----------------------------\r\n");

	DSTATUS status=disk_initialize(0);
	Print("disk_initialize: ");PrintNumber(status);Print("\r\n");

	if(status & STA_NOINIT) {
		Print("SD CARD NOT AVAILABLE / NOT INITIALIZED\r\n");
		Print("TEST C RESULT: PASS - absence detected\r\n\r\n");
		Print("Reinsert the card and reboot to execute tests A/B.\r\n");
		Stop("SD ABSENCE TEST COMPLETE\r\n");
	}

	Print("SD card detected and initialized.\r\n");
	Print("TEST C RESULT: PASS - card detected\r\n");

	Print("\r\nMOUNT FILESYSTEM\r\n-----------------\r\n");
	r=f_mount(&fs,"",1);PrintResult("f_mount",r);
	if(r!=FR_OK)Stop("MOUNT FAILED\r\n");

	// ------------------------------------------------------------
	// A: normal operations
	// ------------------------------------------------------------
	Print("\r\nTEST A - NORMAL FILE OPERATIONS\r\n---------------------------------\r\n");

	const char *files[]={APPEND_TEST_FILE,REWRITE_TEST_FILE,DELETE_TEST_FILE,CREATE_TEST_FILE,EMPTY_TEST_FILE};
	for(unsigned int i=0;i<sizeof(files)/sizeof(files[0]);++i){
		bool e=false;r=SD_Exists(files[i],&e);
		if(r!=FR_OK)Stop("CLEANUP CHECK FAILED\r\n");
		if(e){r=SD_Delete(files[i]);PrintResult("  cleanup SD_Delete",r);if(r!=FR_OK)Stop("CLEANUP FAILED\r\n");}
	}
	Print("Test environment ready.\r\n");

	const char *a1="APPEND RECORD ONE\r\n",*a2="APPEND RECORD TWO\r\n";
	r=SD_Append(APPEND_TEST_FILE,a1,(UINT)strlen(a1));PrintResult("Append record #1",r);
	if(r!=FR_OK)Stop("APPEND FAILED\r\n");
	r=SD_Append(APPEND_TEST_FILE,a2,(UINT)strlen(a2));PrintResult("Append record #2",r);
	if(r!=FR_OK||!ReadAndPrint(APPEND_TEST_FILE)||!VerifyContents(APPEND_TEST_FILE,"APPEND RECORD ONE\r\nAPPEND RECORD TWO\r\n"))Stop("APPEND VERIFICATION FAILED\r\n");
	Print("A2 RESULT: PASS\r\n");

	const char *w1="FIRST CONTENT\r\n",*w2="SECOND CONTENT\r\n";
	r=SD_Rewrite(REWRITE_TEST_FILE,w1,(UINT)strlen(w1));PrintResult("Rewrite #1",r);
	if(r!=FR_OK||!VerifyContents(REWRITE_TEST_FILE,w1))Stop("REWRITE #1 FAILED\r\n");
	r=SD_Rewrite(REWRITE_TEST_FILE,w2,(UINT)strlen(w2));PrintResult("Rewrite #2",r);
	if(r!=FR_OK||!ReadAndPrint(REWRITE_TEST_FILE)||!VerifyContents(REWRITE_TEST_FILE,w2))Stop("REWRITE #2 FAILED\r\n");
	Print("A3 RESULT: PASS\r\n");

	const char *dc="THIS FILE WILL BE DELETED\r\n";
	r=SD_Rewrite(DELETE_TEST_FILE,dc,(UINT)strlen(dc));PrintResult("Create delete-test file",r);
	if(r!=FR_OK)Stop("DELETE SETUP FAILED\r\n");
	r=SD_Delete(DELETE_TEST_FILE);PrintResult("Delete file",r);
	if(r!=FR_OK||!VerifyAbsent(DELETE_TEST_FILE))Stop("DELETE FAILED\r\n");
	r=SD_Delete(DELETE_TEST_FILE);PrintResult("Second delete",r);
	if(r!=FR_NO_FILE)Stop("UNEXPECTED SECOND DELETE RESULT\r\n");
	Print("Second delete: expected FR_NO_FILE received.\r\nA4 RESULT: PASS\r\n");

	const char *cc="CREATED BY APPEND\r\n";
	if(!VerifyAbsent(CREATE_TEST_FILE))Stop("CREATE-ON-DEMAND PRECONDITION FAILED\r\n");
	r=SD_Append(CREATE_TEST_FILE,cc,(UINT)strlen(cc));PrintResult("Append to non-existent file",r);
	if(r!=FR_OK||!ReadAndPrint(CREATE_TEST_FILE)||!VerifyContents(CREATE_TEST_FILE,cc))Stop("CREATE-ON-DEMAND FAILED\r\n");
	Print("A5 RESULT: PASS\r\n");

	const char *ec="THIS CONTENT WILL BE ERASED\r\n";
	r=SD_Rewrite(EMPTY_TEST_FILE,ec,(UINT)strlen(ec));PrintResult("Create initial file",r);
	if(r!=FR_OK)Stop("ZERO-LENGTH SETUP FAILED\r\n");
	r=SD_Rewrite(EMPTY_TEST_FILE,0,0);PrintResult("Rewrite with zero length",r);
	if(r!=FR_OK||!VerifySize(EMPTY_TEST_FILE,0))Stop("ZERO-LENGTH REWRITE FAILED\r\n");
	Print("A6 RESULT: PASS\r\n");
	Print("TEST A RESULT: PASS\r\n");

	// ------------------------------------------------------------
	// B: safe negative API tests
	// ------------------------------------------------------------
	Print("\r\nTEST B - SAFE NEGATIVE API TESTS\r\n----------------------------------\r\n");
	bool pass=true;

	r=SD_Append(0,"DATA",4);PrintResult("SD_Append(NULL path)",r);
	if(r!=FR_INVALID_PARAMETER){Print("  VERIFY: FAIL\r\n");pass=false;}else Print("  VERIFY: PASS\r\n");

	r=SD_Append(APPEND_TEST_FILE,0,4);PrintResult("SD_Append(NULL data, length=4)",r);
	if(r!=FR_INVALID_PARAMETER){Print("  VERIFY: FAIL\r\n");pass=false;}else Print("  VERIFY: PASS\r\n");

	r=SD_Exists(APPEND_TEST_FILE,0);PrintResult("SD_Exists(NULL exists)",r);
	if(r!=FR_INVALID_PARAMETER){Print("  VERIFY: FAIL\r\n");pass=false;}else Print("  VERIFY: PASS\r\n");

	bool e=false;r=SD_Exists("this_file_should_not_exist.txt",&e);PrintResult("SD_Exists(nonexistent)",r);
	if(r!=FR_OK||e){Print("  VERIFY: FAIL\r\n");pass=false;}else Print("  VERIFY: PASS - nonexistent file reported correctly\r\n");

	r=SD_Delete("this_file_should_not_exist.txt");PrintResult("SD_Delete(nonexistent)",r);
	if(r!=FR_NO_FILE){Print("  VERIFY: FAIL\r\n");pass=false;}else Print("  VERIFY: PASS - FR_NO_FILE received\r\n");

	if(!pass)Stop("TEST B FAILED\r\n");
	Print("TEST B RESULT: PASS\r\n");

	Print("\r\n================================================\r\n");
	Print(" ALL SD FILE PRIMITIVE TESTS PASSED\r\n");
	Print("================================================\r\n");
	Print("Test files remain on the SD card for inspection.\r\nSystem alive.\r\n");

	while(true){Delay_ms(1000);Print("ALIVE\r\n");}
}
