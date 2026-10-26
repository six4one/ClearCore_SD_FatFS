# SD_ReadLastLine implementation notes

## API
`SD_ReadLastLine(path, buffer, bufferSize, bytesRead, bufferTooSmall)`

- Opens the file with `FA_READ` only.
- Scans backward in fixed 64-byte chunks, then reads only the selected line.
- Working memory is fixed-size and does not scale with file size.
- Returns the final non-empty byte line without CR/LF terminators and without adding a NUL terminator.
- Skips trailing CR and LF bytes, including trailing blank lines.
- Empty files and files containing only CR/LF bytes return `FR_OK` and zero bytes.
- A final line without a newline is supported.
- If the line is larger than the caller buffer, returns `FR_INVALID_PARAMETER`, sets `bufferTooSmall=true`, and leaves `bytesRead=0`.
- Ordinary FatFS errors are propagated. An opened file is closed on every exit path.
- Existing SD primitives are unchanged.

## Test harness additions
Test E is appended after the existing A/B/C/D tests. It covers LF, CRLF, no final terminator, single-line files, trailing blank lines, empty/all-terminator files, oversized lines, an 8192-byte line, missing files, invalid arguments, and read-only behavior. The existing 10-second startup delay is retained.

## Validation status
The updated sources passed a host C++ syntax-only check using stub ClearCore/FatFS declarations. This is not a Microchip Studio target build and is not a physical ClearCore test. Build and run the harness on the actual project/hardware before accepting the change.

## Git reminder
The uploaded source files do not include the local repository's `.git` metadata, so local Git status could not be checked. Before replacing project files, verify that the three ClearCore SD test projects are protected by Git and preserve a clean checkpoint.
