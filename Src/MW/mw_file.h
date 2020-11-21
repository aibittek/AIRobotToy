#ifndef _MW_FILE_H
#define _MW_FILE_H

#include "ff.h"

#define FILE_READ FA_READ
#define FILE_WRITE FA_WRITE
#define FILE_OPEN_EXISTING FA_OPEN_EXISTING
#define FILE_CREATE_NEW FA_CREATE_NEW
#define FILE_CREATE_ALWAYS FA_CREATE_ALWAYS
#define FILE_OPEN_ALWAYS FA_OPEN_ALWAYS
#define FILE_OPEN_APPEND FA_OPEN_APPEND

typedef struct mw_file
{
    FIL fil;
    uint8_t (*open)(struct mw_file *, const char *path, int8_t mode);
    uint32_t (*read)(struct mw_file *, void *buff, uint32_t size);
    uint32_t (*write)(struct mw_file *, const void *buff, uint32_t size);
    uint8_t (*lseek)(struct mw_file *, uint32_t ulSize);
    void (*close)(struct mw_file *);
} mw_file_t;
typedef mw_file_t File;
void mw_file_init(File *file);
void mw_file_uninit(File *file);

#endif
