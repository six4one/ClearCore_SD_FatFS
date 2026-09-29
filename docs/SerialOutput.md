# SD File Primitive Test — 2026-09-29

Hardware verification performed after merging the `SD_Read()` implementation
into `main` and removing obsolete SDFile copies.

Result: PASS

## Test Output

```text
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

TEST D - SD_Read() TESTS
--------------------------
D1 create read-test file: SUCCESS
D2 SD_Read normal file: SUCCESS
  Contents:
  --------------------
SD_Read TEST CONTENT
SECOND LINE
  --------------------
  Bytes read: 35
  VERIFY: PASS
D3 SD_Read exact-size buffer: SUCCESS
  VERIFY: PASS - exact-size buffer accepted
D4 SD_Read undersized buffer: FAILED (FRESULT=19)
  VERIFY: PASS - buffer-too-small detected
D5 SD_Read empty file: SUCCESS
  VERIFY: PASS - zero bytes returned
D6 SD_Read nonexistent file: FAILED (FRESULT=4)
  VERIFY: PASS - FR_NO_FILE received
D7 SD_Read(NULL path): FAILED (FRESULT=19)
  VERIFY: PASS
D7 SD_Read(NULL buffer): FAILED (FRESULT=19)
  VERIFY: PASS
D7 SD_Read(NULL bytesRead): FAILED (FRESULT=19)
  VERIFY: PASS
D7 SD_Read(NULL bufferTooSmall): FAILED (FRESULT=19)
  VERIFY: PASS
D8 SD_Read and display: SUCCESS
  SD_Read contents:
  --------------------
SD_Read TEST CONTENT
SECOND LINE
  --------------------
  VERIFY: PASS - file read through SD_Read()
TEST D RESULT: PASS

================================================
 ALL SD FILE PRIMITIVE TESTS PASSED
================================================
Test files remain on the SD card for inspection.
System alive.
ALIVE
ALIVE
