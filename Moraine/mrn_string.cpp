#include "mrn_core.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <memory>

namespace moraine
{
    thread_local char    s_mbbuf[4096];
    thread_local wchar_t s_wcbuf[4096];
}

moraine::String::String()
{
    m_mbbuf = nullptr;
    m_wcbuf = nullptr;
    m_size = 0;
}

moraine::String::String(const char* raw)
{
    m_size = strlen(raw);
    size_t sizeInBytes = sizeof(char) * (m_size + 1);

    m_mbbuf = static_cast<char*>(::operator new(sizeInBytes));
    memcpy_s(m_mbbuf, sizeInBytes, raw, sizeInBytes);

    m_wcbuf = nullptr;
}

moraine::String::String(const wchar_t* raw)
{
    m_size = wcslen(raw);
    size_t sizeInBytes = sizeof(wchar_t) * (m_size + 1);

    m_wcbuf = static_cast<wchar_t*>(::operator new(sizeInBytes));
    memcpy_s(m_wcbuf, sizeInBytes, raw, sizeInBytes);

    m_mbbuf = nullptr;
}

moraine::String::~String()
{
    ::operator delete(m_mbbuf);
    m_mbbuf = nullptr;
    ::operator delete(m_wcbuf);
    m_wcbuf = nullptr;
}

moraine::String::String(const String& other)
{
    m_size = other.m_size;

    if (other.m_mbbuf != nullptr)
    {
        size_t MBSizeInBytes = sizeof(char) * (m_size + 1);
        m_mbbuf = static_cast<char*>(::operator new(MBSizeInBytes));
        memcpy_s(m_mbbuf, MBSizeInBytes, other.m_mbbuf, MBSizeInBytes);
    }
    else
        m_mbbuf = nullptr;

    if (other.m_wcbuf != nullptr)
    {
        size_t WCSizeInBytes = sizeof(wchar_t) * (m_size + 1);
        m_wcbuf = static_cast<wchar_t*>(::operator new(WCSizeInBytes));
        memcpy_s(m_wcbuf, WCSizeInBytes, other.m_wcbuf, WCSizeInBytes);
    }
    else
        m_wcbuf = nullptr;
}

moraine::String::String(String&& other)
{
    m_size = other.m_size;
    m_mbbuf = other.m_mbbuf;
    m_wcbuf = other.m_wcbuf;

    other.m_size = 0;
    other.m_mbbuf = nullptr;
    other.m_wcbuf = nullptr;
}

MRN_API moraine::String& moraine::String::operator=(const String& other)
{
    m_size = other.m_size;

    if (other.m_mbbuf != nullptr)
    {
        size_t MBSizeInBytes = sizeof(char) * (m_size + 1);
        m_mbbuf = static_cast<char*>(::operator new(MBSizeInBytes));
        memcpy_s(m_mbbuf, MBSizeInBytes, other.m_mbbuf, MBSizeInBytes);
    }
    else
        m_mbbuf = nullptr;

    if (other.m_wcbuf != nullptr)
    {
        size_t WCSizeInBytes = sizeof(wchar_t) * (m_size + 1);
        m_wcbuf = static_cast<wchar_t*>(::operator new(WCSizeInBytes));
        memcpy_s(m_wcbuf, WCSizeInBytes, other.m_wcbuf, WCSizeInBytes);
    }
    else
        m_wcbuf = nullptr;

    return *this;
}

MRN_API moraine::String& moraine::String::operator=(String&& other)
{
    m_size = other.m_size;
    m_mbbuf = other.m_mbbuf;
    m_wcbuf = other.m_wcbuf;

    other.m_size = 0;
    other.m_mbbuf = nullptr;
    other.m_wcbuf = nullptr;

    return *this;
}

bool moraine::String::operator==(const String& other) const
{
    if (not m_wcbuf and not other.m_wcbuf)
        return strcmp(mbstr(), other.mbstr()) == 0;
    else
        return wcscmp(wcstr(), other.wcstr()) == 0;
}

bool moraine::String::operator==(const char* other) const
{
    return strcmp(mbstr(), other) == 0;
}

bool moraine::String::operator==(const wchar_t* other) const
{
    return wcscmp(wcstr(), other) == 0;
}

char* moraine::String::mbbuf() const
{
    if (m_mbbuf == nullptr && m_wcbuf != nullptr)
    {
        m_mbbuf = static_cast<char*>(::operator new(sizeof(char) * (m_size + 1)));
        wcstombs_s(nullptr, m_mbbuf, m_size + 1, m_wcbuf, m_size);
    }

    if (not m_mbbuf)
        throw std::exception("String empty!");

    return m_mbbuf;
}

wchar_t* moraine::String::wcbuf() const
{
    if (m_wcbuf == nullptr && m_mbbuf != nullptr)
    {
        m_wcbuf = static_cast<wchar_t*>(::operator new(sizeof(wchar_t) * (m_size + 1)));
        mbstowcs_s(nullptr, m_wcbuf, m_size + 1, m_mbbuf, m_size);
    }

    if (not m_wcbuf)
        throw std::exception("String empty!");

    return m_wcbuf;
}

moraine::String moraine::sprintf(const char* format, ...)
{
    va_list args;
    __crt_va_start(args, format);
    vsprintf_s(s_mbbuf, format, args);
    __crt_va_end(args);

    return s_mbbuf;
}

moraine::String moraine::sprintf(const wchar_t* format, ...)
{
    va_list args;
    __crt_va_start(args, format);
    vswprintf_s(s_wcbuf, format, args);
    __crt_va_end(args);

    return s_wcbuf;
}
