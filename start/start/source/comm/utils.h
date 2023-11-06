#ifndef UTILS_H
#define UTILS_H

#include "comm/types.h"

static void uint32_to_str(uint32_t value, char *str)
{
    int i = 0;
    do
    {
        str[i++] = value % 10 + '0';
        value /= 10;
    } while (value);
    str[i] = '\0';

    // 反转
    int start = 0;
    int end = i - 1;
    while (start < end)
    {
        char tmp = str[start];
        str[start] = str[end];
        str[end] = tmp;
        start++;
        end--;
    }
}

#endif // UTILS_H