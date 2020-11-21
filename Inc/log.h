#ifndef _LOG_H
#define _LOG_H

#include <stdio.h>

// 调试代码
enum EDEBUG
{
    EVERBOSE,
    EDEBUG,
    EINFO,
    EWARNNING,
    EERROR,
    EDEBUG_MAX
};

#define EI_DEBUG_LEVEL EDEBUG
#define LOG(level, format, ...) \
    { \
        if (level >= EI_DEBUG_LEVEL) { \
            printf("[%s %s:%d]""["#level"]"format"\r\n",__FILE__,__FUNCTION__,__LINE__, ##__VA_ARGS__); \
        } \
    }

#endif
