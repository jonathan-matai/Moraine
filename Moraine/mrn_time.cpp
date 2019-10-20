#include "mrn_core.h"

#include <ctime>
#include <cstring>
#include <cstdio>

moraine::String moraine::Time::timestamp(const String& format, NanoFormat nanoFormat)
{
    struct timespec t;
    if (!timespec_get(&t, TIME_UTC))
        throw L"Couldn't retrieve time!";

    struct tm tm;
    if (localtime_s(&tm, &t.tv_sec))
        throw L"Couldn't retrieve time!";

    wchar_t buffer[64];
    wcsftime(buffer, 64, format.wcstr(), &tm);
    size_t length = wcslen(buffer);

    if (nanoFormat == MILLISECONDS)
        swprintf_s(buffer + length, 64 - length, L".%03ld", t.tv_nsec / 1000000);
    else if (nanoFormat == MICROSECONDS)
        swprintf_s(buffer + length, 64 - length, L".%06ld", t.tv_nsec / 1000);
    else if (nanoFormat == NANOSECONDS)
        swprintf_s(buffer + length, 64 - length, L".%09ld", t.tv_nsec);

    return buffer;
}

moraine::Time moraine::Time::now()
{
    struct timespec t;
    if (!timespec_get(&t, TIME_UTC))
        throw L"Couldn't retrieve time!";
    return (uint64_t)t.tv_nsec + (uint64_t)t.tv_sec * 1000000000;
}
