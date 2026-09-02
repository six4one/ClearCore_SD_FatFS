# ProjectTemplate

This repository contains a basic ClearCore project that can be used as a template for new application development.
Place this repository rooted in the same parent directory as libClearCore and LwIP to properly find include files and libraries.ClearCore SD Card + FatFs

A working example of integrating ChaN FatFs with the Teknic ClearCore microSD interface using Microchip Studio.

This project demonstrates:

ClearCore SD-card initialization
Low-level sector/block access
FatFs disk I/O integration
FAT filesystem mounting
File creation and writing
File reopening and read-back verification
Free-space/volume information
Deployment to ClearCore

The repository contains the final working test project developed during the bring-up process, together with documentation describing the architecture, implementation, pitfalls encountered, and verification results.

Hardware / Software
Teknic ClearCore
Microchip Studio 7
ClearCore Library
FatFs
microSD card
Verified operation

The final test successfully:

Initialized the SD card.
Mounted the FAT filesystem.
Created WRITE.txt.
Wrote test data.
Closed the file.
Reopened the file.
Read the data back.
Verified the expected contents.

Example result:

===== WRITE.txt READ-BACK =====
ChatGPT and Fausto wrote this!
===== END READ-BACK =====

*** WRITE/READ-BACK PASS ***
Documentation

See docs/ClearCore_SD_FatFs_Practical_Guide.md for the practical implementation guide.

docs/ClearCore_SD_FatFs_BringUp.md contains the more complete development history and technical record.