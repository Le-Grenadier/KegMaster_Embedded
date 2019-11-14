#pragma once

#include <assert.h>
#include <stdbool.h>
#include <time.h>


/*-----------------------------------------------------------------------------
Compare Date portion of date-time struct. (Ignores time)
 - return > 0   === challenge >  base
 - return < 0   === challenge <  base
 - return == 0  === challenge == base
-----------------------------------------------------------------------------*/
static __inline double DateTime_Compare(struct tm* base, struct tm* challenge) {
    double tDiff_sec;
    struct tm b = { 0 }, c = { 0 };

    if (base == NULL || challenge == NULL)
    {
        return(0);
    }

    /* Only compare date, ignore time for now (No way in app to set that) */
    b.tm_year = base->tm_year;
    b.tm_mon = base->tm_mon;
    b.tm_mday = base->tm_mday;

    c.tm_year = challenge->tm_year;
    c.tm_mon = challenge->tm_mon;
    c.tm_mday = challenge->tm_mday;

    tDiff_sec = difftime(mktime(&c), mktime(&b));

    return(tDiff_sec);
}