# ClearCore SD Card Access — FatFs Bring-Up and Validation

**Project:** ClearCore SD Card Access / SD Test  
**Target:** Teknic ClearCore controller  
**MCU:** Microchip SAME53N19A, ARM Cortex-M4  
**IDE:** Microchip Studio 7  
**Toolchain:** ARM GNU GCC 6.3.1  
**Filesystem:** FatFs  
**Purpose:** Document the experimentally verified process used to obtain working SD-card filesystem read/write access on a ClearCore, as a reference for later integration with the Elevator Controller Supervisor HSM.

## 1. Objective

The immediate engineering requirement was to provide the Elevator Controller with a mechanism capable of reading persistent state from the ClearCore's SD card.

The work proceeded experimentally because no immediately usable example matching the required ClearCore + Microchip Studio + SAME53 + SD + FatFs combination was available.

The final result proved:

1. SD initialization.
2. Raw sector reading.
3. FatFs filesystem mounting.
4. Reading an existing file, `TEST.txt`.
5. Filesystem geometry discovery.
6. SD write-path operation.
7. Creation of `WRITE.txt`.
8. Writing data to the file.
9. Closing the file.
10. Reopening the file.
11. Reading the data back.
12. Verifying that the data matched.

## 2. Architecture

FatFs provides the filesystem layer but depends on a hardware-specific disk I/O layer.

The working architecture is:

    Application
        |
        v
      FatFs
        |
        v
     diskio.cpp
        |
        v
    ClearCore SPI
        |
        v
      SD card

The critical ClearCore-specific work therefore resides in `diskio.cpp`.

## 3. Development Environment

The Microchip Studio project compiled:

- ClearCore library
- LwIP library
- FatFs
- ClearCore-specific `diskio.cpp`
- test `main.cpp`

The target was the ClearCore SAME53N19A ARM Cortex-M4.

The existing post-build process converted the generated `.bin` image into a UF2 image for ClearCore deployment.

## 4. Milestone 1 — SD Initialization

The first test called:

    disk_initialize(0)

and produced:

    disk_initialize returned: 0

This established that the ClearCore could initialize the SD card through the implemented low-level interface.

## 5. Milestone 2 — FatFs Mount

FatFs was then mounted using:

    f_mount(&fs, "", 1)

The result was:

    f_mount result: 0

This proved communication between FatFs, `diskio.cpp`, ClearCore SPI, and the SD card.

## 6. Milestone 3 — Read TEST.txt

A seed file named `TEST.txt` was placed on the SD card.

The application opened and read it through FatFs.

Successful output:

    ===== TEST.txt =====
    ChatGPT and Fausto did it!
    ===== EOF =====

This proved the complete filesystem read path:

    SD hardware
      -> ClearCore SPI
      -> disk_read()
      -> FatFs
      -> f_open()
      -> f_read()
      -> application

The low-level read implementation used the SD single-block read command (`CMD17`), handled the data token, read 512 bytes, discarded the CRC bytes, and deselected the card.

## 7. Milestone 4 — Filesystem Geometry

A read-only diagnostic used `f_getfree()`.

The working FatFs configuration exposed:

    f_getfree(const TCHAR*, DWORD*, FATFS**)

Therefore the correct call required:

    FATFS *fsPtr = &fs;
    result = f_getfree("", &freeClusters, &fsPtr);

The measured geometry was:

    sectors/cluster: 16
    total clusters:  1,899,904
    free clusters:   1,899,899
    total sectors:   30,398,464
    free sectors:    30,398,384

    FAT start sector: 5,128
    data start sector: 34,816

The volume is approximately 15.6 GB.

## 8. Why We Did Not Perform an Arbitrary Raw-Sector Write

Although filesystem geometry was known, an arbitrary sector was not selected for testing.

`f_getfree()` reports free space but does not establish that a particular manually chosen LBA is safe to overwrite.

Instead, the write test was performed through FatFs so that FatFs itself could select free storage and maintain the FAT filesystem correctly.

This made the test both safer and more representative of the eventual application.

## 9. Milestone 5 — SD Write Path

A write implementation was added to `diskio.cpp` using the SD single-block write command (`CMD24`).

The write sequence was treated as:

    CMD24
      -> command response
      -> data-start token
      -> 512-byte data
      -> CRC
      -> data-response token
      -> card busy period
      -> card ready

Attention was paid to chip-select handling, command responses, the data-start token, the data-response token, and the card busy period following a write.

## 10. Milestone 6 — FatFs File Write

The known-good read project was copied to create:

    SD_Write_Test

The test created:

    WRITE.txt

using:

    f_open(&file, "WRITE.txt", FA_WRITE | FA_CREATE_ALWAYS)

It wrote:

    ChatGPT and Fausto wrote this!

The controller reported:

    f_write result: 0
    bytes written: 32

The file was closed successfully:

    f_close result: 0

It was then reopened read-only:

    reopen result: 0

and read back:

    f_read result: 0
    bytes read: 32

The actual contents returned from the physical SD card were:

    ChatGPT and Fausto wrote this!

The comparison passed:

    *** WRITE/READ-BACK PASS ***

This is the principal hardware validation result.

## 11. What the Write Test Proved

The successful test exercised this complete chain:

    Application
        |
        v
    f_open()
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
    f_open() for read
        |
        v
    f_read()
        |
        v
    disk_read()
        |
        v
    application verification

The data was physically written and subsequently read back with matching contents.

## 12. Build and Deployment

The final `SD_Write_Test` build succeeded in Microchip Studio.

Reported resource usage:

    Program Memory Usage : 147,148 bytes
    Data Memory Usage    :   9,132 bytes

Microchip Studio reported approximately:

    Program Flash: 28.1%
    Data Memory:    4.5%

These figures describe the complete test firmware, including ClearCore, LwIP, FatFs, startup/support code, SD code, and the test application. They are not the memory cost of SD functionality alone.

The project successfully produced:

    SD_Write_Test.bin

and the existing post-build process generated:

    SD_Write_Test.uf2

The UF2 image was uploaded to the ClearCore and executed successfully.

## 13. Current Proven Capabilities

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
| Close file after write | PROVEN |
| Reopen written file | PROVEN |
| Read written file | PROVEN |
| Read/write data verification | PROVEN |
| UF2 deployment | PROVEN |

## 14. Not Yet Tested

The following remain future work:

- Directory creation.
- Directory enumeration.
- Nested directories.
- Multiple files and directories.
- Append operations.
- File seeking.
- File truncation.
- File deletion.
- File rename.
- File metadata/status operations.
- Repeated append cycles.
- Large-file behavior.
- Power-loss behavior during writes.
- Recovery from interrupted writes.
- Event-log record design.
- Binary record formats.
- Log rotation.

These are not known failures; they simply were outside the scope of this bring-up.

## 15. Architectural Stopping Point

The SD work began as a fast-track response to the Elevator Supervisor HSM requirement for `ReadPersistentState`.

Once reliable SD read/write access was established, several additional possibilities emerged, including directory structures, event logging, binary formats, and log rotation.

Those features were deliberately deferred.

The working implementation should be preserved as a **known-good ClearCore/FatFs SD reference** before the SD code is reorganized for Elevator integration.

## 16. Intended Elevator Integration

The eventual relationship should be approximately:

    Supervisor HSM
          |
          v
    ReadPersistentState
          |
          v
    SD storage abstraction
          |
          v
        FatFs
          |
          v
     ClearCore diskio
          |
          v
       SD card

The Supervisor HSM should not need to know about SPI transactions, SD commands, FAT structures, sectors, clusters, or FatFs internals.

## 17. Engineering Lessons

### Experimentation beat assumptions

Rather than assuming a complete vendor solution existed, the implementation was developed and validated incrementally on real hardware.

### Test one layer at a time

The progression was:

    disk_initialize()
    -> disk_read()
    -> f_mount()
    -> f_open()/f_read()
    -> filesystem geometry
    -> disk_write()
    -> f_write()
    -> close/reopen/read-back

This created clear diagnostic boundaries.

### Return codes are not enough

A successful API return was treated as evidence for the next step, not as final proof. The strongest proof was reading the written data back from the physical card and comparing it with the original data.

### Avoid unnecessary filesystem risk

An arbitrary raw-sector write was rejected because free-space statistics do not prove that a chosen LBA is safe. Filesystem-level tests should allow FatFs to allocate storage.

### Preserve known-good projects

The write project was created by copying the working read project. This preserved a known-good baseline while adding new functionality.

## 18. Final Known-Good Statement

At the conclusion of the bring-up work:

> A Teknic ClearCore controller running firmware built with Microchip Studio can successfully use FatFs to initialize, mount, read, create, write, close, reopen, and read back files from its SD card using a ClearCore-specific `diskio` implementation.

The final demonstrated file operation was:

    WRITE.txt

with:

    write:
        ChatGPT and Fausto wrote this!

    read-back:
        ChatGPT and Fausto wrote this!

    result:
        WRITE/READ-BACK PASS

## 19. Recommended Next Action

Preserve this implementation and return to the Elevator Supervisor HSM and `ReadPersistentState` work.

Future SD development can be resumed independently when the Elevator application actually requires:

- directories,
- event logging,
- append semantics,
- binary records,
- log recovery,
- or other persistent-storage features.

## Appendix A — Final Successful Write-Test Output

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

## Appendix B — Filesystem Geometry Output

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
