/*
 * SDFile.cpp
 *
 * Created: 2026-09-26 8:42:22 AM
 *  Author: Fausto Zecca
 */ 

#include "SDFile.h"

FRESULT SD_Append(
    const char *path,
    const void *data,
    UINT length
)
{
    FIL file;
    FRESULT result;
    UINT bytesWritten = 0;

    if (path == 0 || data == 0) {
        return FR_INVALID_PARAMETER;
    }

    /*
     * FA_OPEN_APPEND:
     *   - Opens the file if it exists.
     *   - Creates it if it does not exist.
     *   - Positions the file pointer at EOF.
     */
    result = f_open(
        &file,
        path,
        FA_OPEN_APPEND | FA_WRITE
    );

    if (result != FR_OK) {
        return result;
    }

    result = f_write(
        &file,
        data,
        length,
        &bytesWritten
    );

    if (result == FR_OK && bytesWritten != length) {
        result = FR_DISK_ERR;
    }

    /*
     * Make the write durable before closing the file.
     */
    if (result == FR_OK) {
        result = f_sync(&file);
    }

    /*
     * Preserve the first operation error if one occurred.
     * Otherwise report a close error, if any.
     */
    FRESULT closeResult = f_close(&file);

    if (result == FR_OK) {
        result = closeResult;
    }

    return result;
}


FRESULT SD_Rewrite(
    const char *path,
    const void *data,
    UINT length
)
{
    FIL file;
    FRESULT result;
    UINT bytesWritten = 0;

    if (path == 0 || data == 0) {
        return FR_INVALID_PARAMETER;
    }

    /*
     * FA_CREATE_ALWAYS:
     *   - Creates the file if it does not exist.
     *   - Truncates an existing file to zero length.
     */
    result = f_open(
        &file,
        path,
        FA_CREATE_ALWAYS | FA_WRITE
    );

    if (result != FR_OK) {
        return result;
    }

    result = f_write(
        &file,
        data,
        length,
        &bytesWritten
    );

    if (result == FR_OK && bytesWritten != length) {
        result = FR_DISK_ERR;
    }

    /*
     * Make the replacement contents durable before closing.
     */
    if (result == FR_OK) {
        result = f_sync(&file);
    }

    /*
     * Preserve the first operation error if one occurred.
     * Otherwise report a close error, if any.
     */
    FRESULT closeResult = f_close(&file);

    if (result == FR_OK) {
        result = closeResult;
    }

    return result;
}


FRESULT SD_Delete(
    const char *path
)
{
    if (path == 0) {
        return FR_INVALID_PARAMETER;
    }

    return f_unlink(path);
}


FRESULT SD_Exists(
    const char *path,
    bool *exists
)
{
    FILINFO info;
    FRESULT result;

    if (path == 0 || exists == 0) {
        return FR_INVALID_PARAMETER;
    }

    *exists = false;

    result = f_stat(path, &info);

    if (result == FR_OK) {
        *exists = true;
        return FR_OK;
    }

    if (result == FR_NO_FILE || result == FR_NO_PATH) {
        return FR_OK;
    }

    return result;
}
