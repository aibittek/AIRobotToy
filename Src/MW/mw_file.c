#include "log.h"
#include "mw_file.h"

__weak uint8_t mw_open(struct mw_file *file, const char *path, int8_t mode);
__weak uint32_t mw_read(struct mw_file *file, void *buff, uint32_t size);
__weak uint32_t mw_write(struct mw_file *file, const void *buff, uint32_t size);
__weak uint8_t mw_lseek(struct mw_file *file, uint32_t ulSize);
__weak void mw_close(struct mw_file *file);

void mw_file_init(File *file)
{
    file->open = mw_open;
    file->read = mw_read;
    file->write = mw_write;
    file->close = mw_close;
    file->lseek = mw_lseek;
}
void mw_file_uninit(File *file)
{
}

__weak uint8_t mw_open(struct mw_file *file, const char *path, int8_t mode)
{
    return f_open(&file->fil, path, mode);
}

__weak uint32_t mw_read(struct mw_file *file, void *buff, uint32_t size)
{
    FRESULT uRet;
    uint32_t bw = 0;
    uRet = f_read(&file->fil, buff, size, &bw);
    if (uRet != FR_OK)
    {
        bw = 0;
        printf("uRet:%d\r\n", uRet);
    }
    return bw;
}

__weak uint32_t mw_write(struct mw_file *file, const void *buff, uint32_t size)
{
    FRESULT uRet;
    uint32_t bw = 0;
    uRet = f_write(&file->fil, buff, size, &bw);
    if (uRet != FR_OK)
    {
        printf("uRet:%d\r\n", uRet);
        bw = 0;
    }
    return bw;
}

__weak uint8_t mw_lseek(struct mw_file *file, uint32_t ulSize)
{
    f_lseek(&file->fil, ulSize);
}

__weak void mw_close(struct mw_file *file)
{
    f_close(&file->fil);
}

void test_sdcard()
{
    uint32_t uStart, uEnd;
    uint32_t bw, totalw = 0, totalr = 0;
    uint8_t uRet;
    static char buffer[4096];

    // sdcard object initialize
    File file;
    mw_file_init(&file);

    // write speed test
    const char *path = "0:/testspeed.txt";
    uRet = file.open(&file, path, FILE_CREATE_ALWAYS | FILE_WRITE);
    if (0 != uRet)
    {
        LOG(EERROR, "open failed");
        return;
    }

    uStart = HAL_GetTick();
    while (1)
    {
        bw = file.write(&file, buffer, sizeof(buffer));
        if (0 == bw)
        {
            LOG(EERROR, "write error");
            file.close(&file);
            return;
        }
        totalw += bw;
        uEnd = HAL_GetTick();
        if (uEnd - uStart >= 10000)
        {
            LOG(EDEBUG, "write size:%d B/s", totalw / 10);
            file.close(&file);
            break;
        }
    }

    // read speed test
    uRet = file.open(&file, path, FILE_READ);
    if (0 != uRet)
    {
        LOG(EERROR, "open failed");
        return;
    }
    uStart = HAL_GetTick();
    while (1)
    {
        bw = file.read(&file, buffer, sizeof(buffer));
        totalr += bw;
        uEnd = HAL_GetTick();
        if (uEnd - uStart >= 1000)
        {
            LOG(EDEBUG, "read size:%d B/s", totalr);
            file.close(&file);
            break;
        }
    }

    // write speed to file
    uRet = file.open(&file, path, FILE_CREATE_ALWAYS | FILE_WRITE);
    if (0 != uRet)
    {
        LOG(EERROR, "open failed");
        return;
    }
    snprintf(buffer, sizeof(buffer), "\r\nwirte speed:%.2f MB/s\r\nread speed:%.2f MB/s\r\n",
             (float)totalw / 1024 / 10240, (float)totalr / 1024 / 1024);
    file.write(&file, buffer, strlen(buffer));
    file.close(&file);
    LOG(EDEBUG, "%s", buffer);
}
