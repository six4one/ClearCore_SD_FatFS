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

#endif /* SDFILE_H_ */
