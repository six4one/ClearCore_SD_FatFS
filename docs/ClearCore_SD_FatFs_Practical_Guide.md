# ClearCore SD Card Access with FatFs
## Practical Bring-Up and Porting Guide for Microchip Studio

**Target:** Teknic ClearCore controller  
**MCU:** SAME53N19A / ARM Cortex-M4  
**IDE:** Microchip Studio 7  
**Toolchain:** ARM GNU GCC 6.3.1  
**Filesystem:** FatFs

---

## 1. Purpose

This document is a practical, instructional record of the steps used to establish working SD-card filesystem access on a Teknic ClearCore controller using FatFs in a Microchip Studio project.

It is intentionally application-independent.

The goal is to provide another developer with a reproducible path from:

> “I have a ClearCore, an SD card, Microchip Studio, and FatFs.”

to:

> “I can initialize the card, mount its filesystem, read an existing file, create a file, write it, close it, reopen it, and verify the data by reading it back.”

The implementation was developed experimentally because a ready-made example matching this exact environment was not available.

---

# 2. The Important Architectural Point

FatFs is a filesystem implementation. It does **not** directly know how to communicate with the SD card hardware.

The integration therefore consists of two major layers:

```text
Application
    |
    v
  FatFs
    |
    v
 diskio.cpp
    |
    v
ClearCore SPI / SD hardware
    |
    v
  SD card
```

FatFs supplies the filesystem operations such as:

```text
f_mount()
f_open()
f_read()
f_write()
f_close()
f_getfree()
```

The platform-specific `diskio.cpp` supplies the block-device operations required by FatFs.

This distinction is the key to understanding the port.

---

# 3. Development Strategy

Do not attempt to make the entire system work at once.

Use a ladder of progressively stronger tests:

```text
1. Compile FatFs
       |
2. Initialize SD card
       |
3. Prove sector read
       |
4. Mount filesystem
       |
5. Read an existing file
       |
6. Query filesystem geometry
       |
7. Implement sector write
       |
8. Create/write a file
       |
9. Close it
       |
10. Reopen it
       |
11. Read it back
       |
12. Compare the data
```

Each successful step provides a known-good foundation for the next step.

This approach also makes failures much easier to localize.

---

# 4. Project Structure

A minimal experimental Microchip Studio project can contain:

```text
Project/
    main.cpp
    diskio.cpp
    ff.c
    ff.h
    ffconf.h
    diskio.h
    ...
```

The exact project organization can be refined later.

During initial bring-up, keeping the number of moving parts small is advantageous.

The ClearCore library can remain a linked library rather than being mixed into the FatFs source tree.

---

# 5. First Build: Make Sure FatFs Compiles

Add the FatFs source and headers to the Microchip Studio project.

At minimum, the project must contain the FatFs core implementation and the platform disk interface.

A critical practical point:

**The FatFs source is normally C code, while the ClearCore application is often compiled as C++.**

The Microchip Studio project therefore needs to compile:

```text
ff.c       -> C compiler
main.cpp   -> C++ compiler
diskio.cpp -> C++ compiler
```

Do not rename `ff.c` to `ff.cpp` simply to make it fit a C++ project.

The successful build used the ARM GNU C compiler for `ff.c` and the ARM GNU C++ compiler for the C++ sources.

---

# 6. Build Configuration

The working build targeted:

```text
__SAME53N19A__
ARM Cortex-M4
-mfloat-abi=hard
-mfpu=fpv4-sp-d16
```

and linked against the existing ClearCore and LwIP libraries.

The exact include/library paths are project-specific, but the important requirement is that the FatFs headers and ClearCore headers are visible to the appropriate source files.

---

# 7. Implement the Disk I/O Layer

The FatFs core expects a platform-specific disk interface.

The important functions include:

```cpp
disk_initialize()
disk_status()
disk_read()
disk_write()
disk_ioctl()
```

The ClearCore-specific implementation is responsible for translating these operations into SD-card transactions over the ClearCore's SPI interface.

This file is the hardware-specific portion of the FatFs port.

---

# 8. First Hardware Test: `disk_initialize()`

Before attempting any filesystem operation, test the SD initialization function.

Use a small application that reports:

```text
START
disk_initialize returned: 0
ALIVE
```

A return value of `0` corresponds to successful disk initialization in the tested implementation.

The `ALIVE` message is useful because it establishes that the application continued running after the initialization attempt.

### Useful debugging technique

During early development, place explicit messages before and after potentially problematic operations:

```text
BEFORE operation
AFTER operation
result = ...
```

This became particularly useful later with `f_mount()`, `f_open()`, `f_write()`, and `f_read()`.

---

# 9. Second Hardware Test: Mount FatFs

Once `disk_initialize()` succeeds, mount the filesystem:

```cpp
FATFS fs;

FRESULT result = f_mount(&fs, "", 1);
```

The successful test produced:

```text
BEFORE f_mount
AFTER f_mount
f_mount result: 0
```

At this point the hardware interface and filesystem layer have successfully communicated.

---

# 10. Third Test: Read an Existing File

Place a known file on the SD card:

```text
TEST.txt
```

Use FatFs to:

1. Open it.
2. Read its contents.
3. Close it.
4. Print the data.

The successful test produced:

```text
BEFORE f_open(TEST.txt)
AFTER f_open
f_open result: 0

===== TEST.txt =====
ChatGPT and Fausto did it!
===== EOF =====
```

This is the first complete filesystem-level proof.

It demonstrates that the following chain works:

```text
SD card
  |
  v
ClearCore SPI
  |
  v
disk_read()
  |
  v
FatFs
  |
  v
f_open()
  |
  v
f_read()
  |
  v
application
```

---

# 11. Low-Level Read Details

The tested low-level SD read path used the SD single-block read operation (`CMD17`).

The general sequence was:

```text
Select card
    |
Send CMD17
    |
Wait for command response
    |
Wait for data token
    |
Read 512-byte sector
    |
Read/discard CRC
    |
Deselect card
```

The implementation should not be considered proven merely because `disk_read()` compiles.

The higher-level `TEST.txt` read provided the practical validation that the returned sector data was actually usable by FatFs.

---

# 12. Filesystem Geometry Test

Once mounting and file reading worked, filesystem information was queried with `f_getfree()`.

The tested FatFs version exposed:

```cpp
f_getfree(const TCHAR*, DWORD*, FATFS**)
```

This produced an important compile-time trap.

Passing:

```cpp
&fs
```

where a `FATFS**` was expected generated:

```text
error: cannot convert 'FATFS*' to 'FATFS**'
```

The working approach was:

```cpp
FATFS *fsPtr = &fs;
result = f_getfree("", &freeClusters, &fsPtr);
```

### Lesson

Do not assume that a FatFs API signature from another example or version matches the headers actually being compiled.

When the compiler disagrees with an example:

> Trust the actual header in the project.

---

# 13. Geometry Results

The working SD card produced:

```text
sectors/cluster: 16
total clusters:  1899904
free clusters:   1899899

total sectors:   30398464
free sectors:    30398384

FAT start sector:  5128
data start sector: 34816
```

This established that FatFs could successfully interrogate the filesystem allocation structures.

---

# 14. Important Write-Test Pitfall

At this point it may be tempting to perform a direct raw-sector write.

Do not choose an arbitrary sector merely because the filesystem reports that free space exists.

`f_getfree()` tells you how much free space exists. It does not make an arbitrary LBA safe to overwrite.

A direct write could damage:

- the FAT,
- a directory,
- a file,
- filesystem metadata,
- or another structure.

For a filesystem-level write test, let FatFs allocate the storage.

---

# 15. Implement the Write Path

The ClearCore-specific `disk_write()` implementation was added after the read path was known to work.

The SD single-block write command is:

```text
CMD24
```

The general write sequence is:

```text
Select card
    |
Send CMD24 + sector address
    |
Wait for command response
    |
Send data-start token
    |
Send 512 bytes
    |
Send CRC
    |
Read data-response token
    |
Wait for card busy period to finish
    |
Deselect card
```

The write implementation must correctly handle the SD card's busy period.

Returning immediately after transmitting the data is not sufficient.

---

# 16. Build a Dedicated Write Test

A useful development technique is to copy the known-good read project rather than modifying the original test in place.

The working test was copied to:

```text
SD_Write_Test
```

This preserved the known-good read implementation while adding write functionality.

That proved to be a smooth and reliable workflow.

---

# 17. Create a File Through FatFs

Use FatFs rather than manually selecting an SD sector.

The test created:

```text
WRITE.txt
```

using:

```cpp
f_open(&file, "WRITE.txt", FA_WRITE | FA_CREATE_ALWAYS);
```

The successful output included:

```text
BEFORE f_open(WRITE.txt)
AFTER f_open
f_open result: 0
```

---

# 18. Write Known Data

A short, recognizable test string was written:

```text
ChatGPT and Fausto wrote this!
```

The result:

```text
BEFORE f_write
AFTER f_write
f_write result: 0
bytes written: 32
```

The number of bytes reported by FatFs should be checked rather than assuming the requested count was actually written.

---

# 19. Close the File

The test explicitly closed the file:

```cpp
f_close(&file);
```

Successful output:

```text
BEFORE f_close
AFTER f_close
f_close result: 0
```

Closing the file is important in a meaningful filesystem test because it exercises the filesystem's completion/metadata path rather than merely testing a data transfer.

---

# 20. Reopen the File

The file was then reopened:

```text
BEFORE REOPEN
AFTER REOPEN
reopen result: 0
```

This is a stronger test than simply trusting the return code from `f_write()`.

The test now asks:

> Can the filesystem locate the file after the write operation has completed?

---

# 21. Read the Data Back

The reopened file was read:

```text
BEFORE f_read
AFTER f_read
f_read result: 0
bytes read: 32
```

The returned data was:

```text
===== WRITE.txt READ-BACK =====
ChatGPT and Fausto wrote this!
===== END READ-BACK =====
```

The test concluded:

```text
*** WRITE/READ-BACK PASS ***
```

This is the strongest validation milestone reached during the experiment.

---

# 22. What the Final Test Proves

The final test exercised:

```text
Application
    |
    v
FatFs f_open()
    |
    v
FatFs allocation
    |
    v
f_write()
    |
    v
disk_write()
    |
    v
ClearCore SPI
    |
    v
SD card
    |
    v
f_close()
    |
    v
f_open() again
    |
    v
f_read()
    |
    v
disk_read()
    |
    v
Application comparison
```

The data read back from the SD card matched the data originally written.

That establishes working filesystem-level SD read/write functionality.

---

# 23. Build and Deployment

The final write-test project built successfully.

The build reported:

```text
Program Memory Usage : 147148 bytes
Data Memory Usage    :   9132 bytes
```

Microchip Studio reported approximately:

```text
Program Flash: 28.1%
Data Memory:    4.5%
```

These numbers represent the complete firmware image, not FatFs alone.

The build generated:

```text
SD_Write_Test.bin
```

The existing post-build process then generated:

```text
SD_Write_Test.uf2
```

The UF2 image was successfully deployed to the ClearCore.

---

# 24. Resource-Usage Interpretation

Do not interpret the 28.1% Flash figure as:

> “FatFs used 28.1% of Flash.”

It means approximately 28.1% of the available program memory was occupied by the **entire test firmware**.

That includes things such as:

- ClearCore library
- LwIP
- startup code
- support libraries
- FatFs
- SD disk I/O
- test application

Likewise, the 4.5% data-memory figure describes the complete application image.

The test therefore demonstrated substantial remaining reported memory capacity.

Nevertheless, Flash and RAM usage should be monitored as additional application functionality is added.

---

# 25. Known-Good Capability Set

At the end of the experiment, the following were experimentally proven:

| Capability | Status |
|---|---|
| SD initialization | PROVEN |
| SD sector read | PROVEN |
| FatFs mount | PROVEN |
| Open existing file | PROVEN |
| Read existing file | PROVEN |
| Filesystem free-space query | PROVEN |
| Create file | PROVEN |
| Write file | PROVEN |
| Close file | PROVEN |
| Reopen file | PROVEN |
| Read written file | PROVEN |
| Read/write verification | PROVEN |
| UF2 deployment | PROVEN |

---

# 26. Things Not Yet Proven

The following were intentionally not developed during this bring-up:

- Directory creation.
- Directory enumeration.
- Nested directories.
- File append behavior.
- File seeking.
- File truncation.
- File deletion.
- File rename.
- File metadata/status operations.
- Repeated append cycles.
- Large-file behavior.
- Power-loss behavior during a write.
- Recovery from an interrupted write.
- Log rotation.
- Binary record formats.

These should be treated as **untested**, not as known failures.

---

# 27. Recommended Development Pattern

For future FatFs work on the platform:

### Step 1

Keep a known-good SD read test.

### Step 2

Make a copy before introducing a new capability.

### Step 3

Add one filesystem operation at a time.

### Step 4

Print before and after important calls while debugging.

For example:

```text
BEFORE f_write
AFTER f_write
f_write result: ...
```

### Step 5

Check both the FatFs return code and the byte count.

### Step 6

Whenever possible, verify writes by closing, reopening, and reading the data back.

### Step 7

Do not declare success merely because the code compiles.

The strongest test is:

> Data written to the physical card can subsequently be recovered and verified.

---

# 28. Troubleshooting Lessons

## `disk_initialize()` succeeds but the application appears dead

Add output immediately before and after the next operation.

This distinguishes a failed operation from a reset, hang, or communication problem.

## `f_mount()` returns unexpectedly

Print:

```text
BEFORE f_mount
AFTER f_mount
f_mount result
```

If `AFTER f_mount` never appears, the problem is below or inside that operation rather than in subsequent filesystem code.

## A FatFs API example does not compile

Check the actual declaration in the project's `ff.h`.

FatFs versions/configurations can expose different signatures.

## `disk_write()` helper is reported as undeclared

In C++, a helper function must be declared before it is called.

If a helper is defined later in the source file, provide a forward declaration or move its definition above the caller.

A typical symptom is:

```text
'writeBlock' was not declared in this scope
```

while the compiler later reports that the same function was defined but unused.

## Write returns success but the file cannot be read back

Do not assume the write path is correct.

Check:

- SD command response.
- Data-start token.
- Data-response token.
- Card busy period.
- Chip select.
- File close result.
- Reopen result.
- Read result.
- Number of bytes written/read.

The read-back test is the decisive check.

---

# 29. Final Known-Good Result

The completed experiment demonstrated:

> A Teknic ClearCore controller running firmware built with Microchip Studio can use FatFs to initialize, mount, read, create, write, close, reopen, and read back files on its SD card using a ClearCore-specific `diskio` implementation.

The final write/read-back result was:

```text
WRITE.txt

Written:
    ChatGPT and Fausto wrote this!

Read back:
    ChatGPT and Fausto wrote this!

Result:
    *** WRITE/READ-BACK PASS ***
```

This implementation should be retained as a known-good reference before further refactoring.

---

# Appendix A — Final Successful Serial Output

```text
SD WRITE TEST
=============
disk_initialize: 0
BEFORE f_mount
AFTER f_mount
f_mount result: 0

BEFORE f_open(WRITE.txt)
AFTER f_open
f_open result: 0

BEFORE f_write
AFTER f_write
f_write result: 0
bytes written: 32

BEFORE f_close
AFTER f_close
f_close result: 0

BEFORE REOPEN
AFTER REOPEN
reopen result: 0

BEFORE f_read
AFTER f_read
f_read result: 0
bytes read: 32

===== WRITE.txt READ-BACK =====
ChatGPT and Fausto wrote this!
===== END READ-BACK =====

*** WRITE/READ-BACK PASS ***
ALIVE
```

# Appendix B — Filesystem Geometry Output

```text
SD GEOMETRY TEST
================
disk_initialize: 0
f_mount: 0
f_getfree: 0

Filesystem geometry:
  sectors/cluster: 16
  total clusters:  1899904
  free clusters:   1899899
  total sectors:   30398464
  free sectors:    30398384

Filesystem structure:
  FAT start sector: 5128
  data start sector: 34816

NO DATA WRITTEN
ALIVE
```

# Appendix C — Practical Bottom Line

The difficult part was not learning how to call `f_open()`.

The difficult part was establishing the missing hardware-specific bridge:

```text
FatFs
   |
   v
diskio.cpp
   |
   v
ClearCore SD interface
```

Once that bridge was working, the standard FatFs filesystem operations behaved as expected.

The most reliable path was therefore:

> **Prove the hardware layer first, then prove the filesystem layer, then prove write/read persistence.**
