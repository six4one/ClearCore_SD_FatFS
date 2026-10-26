/*
 * SDFile.h
 *
 * Created: 2026-09-26 8:37:07 AM
 * Author: Fausto Zecca
 *
 * ClearCore SD file primitives.
 */

#ifndef SDFILE_H_
#define SDFILE_H_

#include "ff.h"

FRESULT SD_Append(
    const char *path,
    const void *data,
    UINT length
);

FRESULT SD_Rewrite(
    const char *path,
    const void *data,
    UINT length
);

FRESULT SD_Delete(
    const char *path
);

FRESULT SD_Exists(
    const char *path,
    bool *exists
);

FRESULT SD_Read(
    const char *path,
    void *buffer,
    UINT bufferSize,
    UINT *bytesRead,
    bool *bufferTooSmall
);


/*
 * Reads the final non-empty line from a file without loading the whole file.
 *
 * CR and LF bytes are line terminators and are excluded from the returned
 * data. Trailing CR/LF bytes (including blank trailing lines) are skipped.
 * A file that is empty or contains only CR/LF bytes succeeds with *bytesRead
 * set to zero. A final line without a terminator is supported.
 *
 * The returned bytes are not NUL-terminated. If the line does not fit in
 * bufferSize, returns FR_INVALID_PARAMETER, sets *bufferTooSmall=true, and
 * leaves *bytesRead at zero. Memory use is bounded independently of file size.
 * The file is opened read-only and is never modified.
 */
FRESULT SD_ReadLastLine(
    const char *path,
    void *buffer,
    UINT bufferSize,
    UINT *bytesRead,
    bool *bufferTooSmall
);

#endif /* SDFILE_H_ */
