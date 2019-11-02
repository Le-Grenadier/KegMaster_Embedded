#pragma once

#include <stdbool.h>
#include <time.h>

static __inline bool DateTime_Compare(struct tm* base, struct tm* challenge) {
    bool gt;

    if (base == NULL || challenge == NULL)
    {
        return(0);
    }

    gt = challenge->tm_year > base->tm_year ? true :
        challenge->tm_year < base->tm_year ? false :
        \
        challenge->tm_mon  > base->tm_mon ? true :
        challenge->tm_mon  < base->tm_mon ? false :
        \
        challenge->tm_mday > base->tm_mday ? true : false;

    return(gt);
}