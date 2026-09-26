
============================================
 ClearCore SD FILE PRIMITIVE TEST HARNESS
============================================

SD INITIALIZATION
-----------------
disk_initialize: 0
SD initialization: SUCCESS

MOUNT FILESYSTEM
-----------------
f_mount: SUCCESS

CLEAN TEST ENVIRONMENT
----------------------
Test environment ready.

TEST 1 - APPEND
----------------
Append record #1: SUCCESS
Append record #2: SUCCESS

Reading append_test.txt...
  Contents:
  --------------------
APPEND RECORD ONE
APPEND RECORD TWO

  --------------------
  Bytes read: 38
  VERIFY: PASS
TEST 1 RESULT: PASS

TEST 2 - REWRITE
-----------------
Rewrite #1: SUCCESS
  VERIFY: PASS
Rewrite #2: SUCCESS

Reading rewrite_test.txt...
  Contents:
  --------------------
SECOND CONTENT

  --------------------
  Bytes read: 16
  VERIFY: PASS
TEST 2 RESULT: PASS

TEST 3 - DELETE
----------------
Create delete-test file: SUCCESS
Check file before delete: SUCCESS
delete_test.txt: EXISTS
Delete file: SUCCESS
  SD_Exists: SUCCESS
  VERIFY: PASS - file does not exist
Attempting second delete (expected FR_NO_FILE)...
Second delete: FAILED (FRESULT=4)
Second delete: expected FR_NO_FILE received.
TEST 3 RESULT: PASS

TEST 4 - CREATE ON DEMAND
--------------------------
Confirming file does not exist before append...
  SD_Exists: SUCCESS
  VERIFY: PASS - file does not exist
Append to non-existent file: SUCCESS

Reading create_test.txt...
  Contents:
  --------------------
CREATED BY APPEND

  --------------------
  Bytes read: 19
  VERIFY: PASS
TEST 4 RESULT: PASS

============================================
 ALL SD FILE PRIMITIVE TESTS PASSED
============================================

Test files remain on the SD card for inspection.
System alive.
ALIVE
ALIVE
