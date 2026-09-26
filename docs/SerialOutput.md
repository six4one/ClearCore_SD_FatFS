
================================================
 ClearCore SD FILE PRIMITIVE TEST HARNESS
================================================

TEST C - SD CARD DETECTION
----------------------------
disk_initialize: 0
SD card detected and initialized.
TEST C RESULT: PASS - card detected

MOUNT FILESYSTEM
-----------------
f_mount: SUCCESS

TEST A - NORMAL FILE OPERATIONS
---------------------------------
  cleanup SD_Delete: SUCCESS
  cleanup SD_Delete: SUCCESS
  cleanup SD_Delete: SUCCESS
Test environment ready.
Append record #1: SUCCESS
Append record #2: SUCCESS
  Contents:
  --------------------
APPEND RECORD ONE
APPEND RECORD TWO

  --------------------
  Bytes read: 38
  VERIFY: PASS
A2 RESULT: PASS
Rewrite #1: SUCCESS
  VERIFY: PASS
Rewrite #2: SUCCESS
  Contents:
  --------------------
SECOND CONTENT

  --------------------
  Bytes read: 16
  VERIFY: PASS
A3 RESULT: PASS
Create delete-test file: SUCCESS
Delete file: SUCCESS
  SD_Exists: SUCCESS
  VERIFY: PASS - file does not exist
Second delete: FAILED (FRESULT=4)
Second delete: expected FR_NO_FILE received.
A4 RESULT: PASS
  SD_Exists: SUCCESS
  VERIFY: PASS - file does not exist
Append to non-existent file: SUCCESS
  Contents:
  --------------------
CREATED BY APPEND

  --------------------
  Bytes read: 19
  VERIFY: PASS
A5 RESULT: PASS
Create initial file: SUCCESS
Rewrite with zero length: SUCCESS
  f_stat: SUCCESS
  File size: 0 bytes
  VERIFY: PASS
A6 RESULT: PASS
TEST A RESULT: PASS

TEST B - SAFE NEGATIVE API TESTS
----------------------------------
SD_Append(NULL path): FAILED (FRESULT=19)
  VERIFY: PASS
SD_Append(NULL data, length=4): FAILED (FRESULT=19)
  VERIFY: PASS
SD_Exists(NULL exists): FAILED (FRESULT=19)
  VERIFY: PASS
SD_Exists(nonexistent): SUCCESS
  VERIFY: PASS - nonexistent file reported correctly
SD_Delete(nonexistent): FAILED (FRESULT=4)
  VERIFY: PASS - FR_NO_FILE received
TEST B RESULT: PASS

================================================
 ALL SD FILE PRIMITIVE TESTS PASSED
================================================
Test files remain on the SD card for inspection.
System alive.
ALIVE
ALIVE
