/*
 * SDFile.cpp
 *
 * ClearCore SD file primitives.
 */

#include "SDFile.h"

FRESULT SD_Append(const char *path, const void *data, UINT length)
{
    FIL file;
    FRESULT result;
    UINT bytesWritten = 0;

    if (path == 0) return FR_INVALID_PARAMETER;
    if (data == 0 && length != 0) return FR_INVALID_PARAMETER;

    result = f_open(&file, path, FA_OPEN_APPEND | FA_WRITE);
    if (result != FR_OK) return result;

    if (length != 0) {
        result = f_write(&file, data, length, &bytesWritten);
        if (result == FR_OK && bytesWritten != length) result = FR_DISK_ERR;
    }

    if (result == FR_OK) result = f_sync(&file);

    FRESULT closeResult = f_close(&file);
    if (result == FR_OK) result = closeResult;

    return result;
}

FRESULT SD_Rewrite(const char *path, const void *data, UINT length)
{
    FIL file;
    FRESULT result;
    UINT bytesWritten = 0;

    if (path == 0) return FR_INVALID_PARAMETER;
    if (data == 0 && length != 0) return FR_INVALID_PARAMETER;

    result = f_open(&file, path, FA_CREATE_ALWAYS | FA_WRITE);
    if (result != FR_OK) return result;

    if (length != 0) {
        result = f_write(&file, data, length, &bytesWritten);
        if (result == FR_OK && bytesWritten != length) result = FR_DISK_ERR;
    }

    if (result == FR_OK) result = f_sync(&file);

    FRESULT closeResult = f_close(&file);
    if (result == FR_OK) result = closeResult;

    return result;
}

FRESULT SD_Delete(const char *path)
{
    if (path == 0) return FR_INVALID_PARAMETER;
    return f_unlink(path);
}

FRESULT SD_Exists(const char *path, bool *exists)
{
    FILINFO info;
    FRESULT result;

    if (path == 0 || exists == 0) return FR_INVALID_PARAMETER;

    *exists = false;
    result = f_stat(path, &info);

    if (result == FR_OK) {
        *exists = true;
        return FR_OK;
    }

    if (result == FR_NO_FILE || result == FR_NO_PATH) return FR_OK;

    return result;
}

FRESULT SD_Read(
    const char *path,
    void *buffer,
    UINT bufferSize,
    UINT *bytesRead,
    bool *bufferTooSmall)
{
    FILINFO info;
    FIL file;
    FRESULT result;
    UINT actualBytesRead = 0;

    /*
     * The output parameters are required because the caller needs both
     * the actual byte count and an explicit indication of a buffer-too-small
     * condition.
     */
    if (bytesRead == 0 || bufferTooSmall == 0) {
        return FR_INVALID_PARAMETER;
    }

    *bytesRead = 0;
    *bufferTooSmall = false;

    if (path == 0) {
        return FR_INVALID_PARAMETER;
    }

    if (buffer == 0 && bufferSize != 0) {
        return FR_INVALID_PARAMETER;
    }

    /*
     * Determine the file size before opening it for reading.  This lets
     * us reject a destination buffer that cannot contain the complete file
     * without performing a partial read.
     */
    result = f_stat(path, &info);
    if (result != FR_OK) {
        return result;
    }

    if (info.fsize > (FSIZE_t)bufferSize) {
        *bufferTooSmall = true;
        return FR_INVALID_PARAMETER;
    }

    /*
     * A zero-length file is a valid read and requires no destination
     * buffer.  A non-empty file does require one.
     */
    if (info.fsize != 0 && buffer == 0) {
        return FR_INVALID_PARAMETER;
    }

    result = f_open(&file, path, FA_READ);
    if (result != FR_OK) {
        return result;
    }

    if (info.fsize != 0) {
        result = f_read(&file, buffer, bufferSize, &actualBytesRead);
    }

    /*
     * Always close the file.  If the read itself succeeded, the close
     * result becomes the final operation result.
     */
    FRESULT closeResult = f_close(&file);

    if (result == FR_OK) {
        result = closeResult;
    }

    /*
     * A successful read must account for the complete file.  A short read
     * with FR_OK is treated as a disk error, matching the write-side
     * short-operation protection already used by this framework.
     */
    if (result == FR_OK) {
        *bytesRead = actualBytesRead;

        if (actualBytesRead != (UINT)info.fsize) {
            result = FR_DISK_ERR;
        }
    }

    return result;
}


FRESULT SD_ReadLastLine(
    const char *path,
    void *buffer,
    UINT bufferSize,
    UINT *bytesRead,
    bool *bufferTooSmall)
{
    FIL file;
    FRESULT result;
    UINT actualBytesRead = 0;
    FSIZE_t fileSize = 0;
    FSIZE_t lineStart = 0;
    FSIZE_t lineEnd = 0;
    bool foundContent = false;
    bool foundStart = false;

    /*
     * Keep the working buffer fixed-size: RAM use does not grow with the
     * file. Scanning proceeds backward in chunks, then the selected line
     * alone is read into the caller's buffer.
     */
    BYTE scanBuffer[64];

    if (bytesRead == 0 || bufferTooSmall == 0) {
        return FR_INVALID_PARAMETER;
    }

    *bytesRead = 0;
    *bufferTooSmall = false;

    if (path == 0) {
        return FR_INVALID_PARAMETER;
    }

    if (buffer == 0 && bufferSize != 0) {
        return FR_INVALID_PARAMETER;
    }

    result = f_open(&file, path, FA_READ);
    if (result != FR_OK) {
        return result;
    }

    fileSize = f_size(&file);
    FSIZE_t scanEnd = fileSize;

    while (result == FR_OK && scanEnd > 0 && !foundStart) {
        FSIZE_t chunkStart =
            (scanEnd > (FSIZE_t)sizeof(scanBuffer))
                ? scanEnd - (FSIZE_t)sizeof(scanBuffer)
                : 0;
        UINT requested = (UINT)(scanEnd - chunkStart);
        UINT chunkBytesRead = 0;

        result = f_lseek(&file, chunkStart);
        if (result != FR_OK) {
            break;
        }

        result = f_read(&file, scanBuffer, requested, &chunkBytesRead);
        if (result != FR_OK) {
            break;
        }

        if (chunkBytesRead != requested) {
            result = FR_DISK_ERR;
            break;
        }

        UINT i = chunkBytesRead;
        while (i > 0) {
            --i;
            BYTE c = scanBuffer[i];
            FSIZE_t absoluteOffset = chunkStart + (FSIZE_t)i;

            if (c == '\r' || c == '\n') {
                if (foundContent) {
                    lineStart = absoluteOffset + 1;
                    foundStart = true;
                    break;
                }
                /* Ignore trailing line terminators and blank lines. */
            } else if (!foundContent) {
                foundContent = true;
                lineEnd = absoluteOffset + 1;
            }
        }

        scanEnd = chunkStart;
    }

    /*
     * If no non-terminator byte was found, the file is empty or consists
     * only of CR/LF bytes; that is a successful zero-byte result.
     * If content was found but no earlier terminator exists, the line
     * begins at offset zero.
     */
    if (result == FR_OK && foundContent) {
        FSIZE_t lineLength = lineEnd - lineStart;

        if (lineLength > (FSIZE_t)bufferSize) {
            *bufferTooSmall = true;
            result = FR_INVALID_PARAMETER;
        } else if (lineLength != 0) {
            result = f_lseek(&file, lineStart);
            if (result == FR_OK) {
                result = f_read(
                    &file,
                    buffer,
                    (UINT)lineLength,
                    &actualBytesRead);
            }

            if (result == FR_OK && actualBytesRead != (UINT)lineLength) {
                result = FR_DISK_ERR;
            }
        }
    }

    /*
     * Always close an opened file. Preserve the earlier operation error;
     * otherwise propagate a close error.
     */
    FRESULT closeResult = f_close(&file);
    if (result == FR_OK) {
        result = closeResult;
    }

    if (result == FR_OK) {
        *bytesRead = actualBytesRead;
    }

    return result;
}
