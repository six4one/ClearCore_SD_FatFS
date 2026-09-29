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
