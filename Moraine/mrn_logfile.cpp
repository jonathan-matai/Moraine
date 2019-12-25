#include "mrn_core.h"

#include <Windows.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>

namespace moraine
{
    struct Table_I : public Table_T
    {
        Table_I(Stringr title, Color color, std::initializer_list<String> columns) :
            m_title(title),
            m_color(color)
        {
            m_header.reserve(columns.size());

            for (const auto& a : columns)
                m_header.push_back(std::make_pair(a, a.length()));
        }

        Table_I(Stringr title, Color color, std::vector<String>& columns) :
            m_title(title),
            m_color(color)
        {
            m_header.reserve(columns.size());

            for (const auto& a : columns)
                m_header.push_back(std::make_pair(a, a.length()));
        }

        ~Table_I() override { }

        void addRow(Color color, std::initializer_list<String> row) override
        {
            std::vector<std::pair<Color, String>> rowvec;

            size_t i = 0;
            for (const auto& a : row)
            {
                rowvec.push_back(std::make_pair(color, a));

                if (m_header[i].second < a.length())
                    m_header[i].second = a.length();

                ++i;
            }

            m_content.push_back(move(rowvec));
        }

        void addRow(std::initializer_list<std::pair<Color, String>> row) override
        {
            size_t i = 0;
            for (const auto& a : row)
            {
                if (m_header[i].second < a.second.length())
                    m_header[i].second = a.second.length();

                ++i;
            }

            m_content.push_back(move(row));
        }

        void addColumn(Color color, std::initializer_list<String> row) override
        {
            if (m_content.size() < row.size())
                m_content.resize(row.size());

            size_t width = 0;

            size_t i = 0;
            for (const auto& a : row)
            {
                if (a.length() > width)
                    width = a.length();

                m_content[i].push_back(std::make_pair(color, a));

                ++i;
            }

            if (width > m_header[m_content[0].size() - 1].second)
                m_header[m_content[0].size() - 1].second = width;
        }

        void addColumn(std::initializer_list<std::pair<Color, String>> row) override
        {
            if (m_content.size() < row.size())
                m_content.resize(row.size());

            size_t width = 0;

            size_t i = 0;
            for (const auto& a : row)
            {
                if (a.second.length() > width)
                    width = a.second.length();

                m_content[i].push_back(a);

                ++i;
            }

            if (width > m_header[m_content[0].size() - 1].second)
                m_header[m_content[0].size() - 1].second = width;
        }

        void dbgconv() override
        {
            for (const auto& a : m_header)
            {
                a.first.mbstr();
                a.first.wcstr();
            }

            for (const auto& a : m_content)
            {
                for (const auto& b : a)
                {
                    b.second.mbstr();
                    b.second.wcstr();
                }
            }  
        }

        String                                              m_title;
        Color                                               m_color;
        std::vector<std::pair<String, size_t>>              m_header;
        std::vector<std::vector<std::pair<Color, String>>>  m_content;
    };

    struct Logfile_I : public Logfile_T
    {
        Logfile_I(Stringr path, Stringr title) :
            m_logName(title)
        {
            if (_wfopen_s(&m_fileHandle, path.wcstr(), L"w, ccs=UNICODE") or m_fileHandle == 0)
                throw std::exception("Error!");

            fwprintf_s(m_fileHandle,
                L"<html><head><title>%s</title></head>"
                L"<body style=\"background-color: #222; color: #eee; font-family: Cascadia Code, Consolas; font-size: 15\">"
                L"<h1>%s</h1>"
                L"<p>%s</p><hr>",
                title.wcstr(), title.wcstr(), Time::now().timestamp(L"%F %T", Time::MILLISECONDS).wcstr());

            fflush(m_fileHandle);

            DWORD mode = 0;
            HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

            if (not GetConsoleMode(handle, &mode))
                throw std::exception("Console API Error");

            if (not SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
                throw std::exception("Console API Error");

            if (_setmode(_fileno(stdout), _O_U16TEXT) == -1)
                throw std::exception("Console API Error");
        }

        ~Logfile_I() override
        {
            fwprintf_s(m_fileHandle, L"<hr></body></html>");
            fclose(m_fileHandle);
        }

        void print(Color color, const String& string) override
        {
            String time = Time::now().timestamp(L"%X", Time::MILLISECONDS);

            fwprintf_s(m_fileHandle, L"<p style=\"color:#%02x%02x%02x;\">[%s] %s</p>\n",
                color.r, color.g, color.b, time.wcstr(), string.wcstr());

            fflush(m_fileHandle);

            wprintf_s(L"\x1b[38;2;%u;%u;%um[%s, %s] %s\x1b[0m\n\n",
                color.r, color.g, color.b, m_logName.wcstr(), time.wcstr(), string.wcstr());
        }

        void print(Color color, const String& string, DebugInfo debugInfo) override
        {
            String time = Time::now().timestamp(L"%X", Time::MILLISECONDS);

            const char* lastSlash = strrchr(debugInfo.file, '\\');

            if (lastSlash != nullptr)
                debugInfo.file = lastSlash + 1;

            fwprintf_s(m_fileHandle, L"<p style=\"color:#%02x%02x%02x;\">[%s, %S:%d (%S)] %s</p>\n",
                color.r, color.g, color.b, time.wcstr(), debugInfo.file, debugInfo.line, debugInfo.function, string.wcstr());

            fflush(m_fileHandle);

            wprintf_s(L"\x1b[38;2;%u;%u;%um[%s, %s, %S:%d (%S)] %s\x1b[0m\n\n",
                color.r, color.g, color.b, m_logName.wcstr(), time.wcstr(), debugInfo.file, debugInfo.line, debugInfo.function, string.wcstr());
        }

        void print(Table table) override
        {
            Table_I* t = dynamic_cast<Table_I*>(table.get());

            String time = Time::now().timestamp(L"%X", Time::MILLISECONDS);

            fwprintf_s(m_fileHandle, L"<p style=\"color:#%02x%02x%02x;\">[%s] %s:</p>\n<div><table style=\"font-size: 15; display: inline-block\">\n<tr style=\"background-color: #%02x%02x%02x; color: #222\">\n",
                t->m_color.r, t->m_color.g, t->m_color.b, time.wcstr(), t->m_title.wcstr(), t->m_color.r, t->m_color.g, t->m_color.b);

            wprintf_s(L"\x1b[38;2;%u;%u;%um[%s, %s] %s:\x1b[0m\n\x1b[30;48;2;%u;%u;%um",
                t->m_color.r, t->m_color.g, t->m_color.b, m_logName.wcstr(), time.wcstr(), t->m_title.wcstr(), t->m_color.r, t->m_color.g, t->m_color.b);

            for (auto& a : t->m_header)
            {
                fwprintf_s(m_fileHandle, L"<th style=\"padding-right:50px;text-align:left\">%s</th>\n", a.first.wcstr());
                wprintf_s(L"%-*s", static_cast<int>(a.second + 5), a.first.wcstr());
            }

            fwprintf_s(m_fileHandle, L"</tr>\n");
            wprintf_s(L"\x1b[0m\n");

            for (auto& a : t->m_content)
            {
                fwprintf_s(m_fileHandle, L"<tr>\n");

                auto b = t->m_header.begin();
                for (auto& c : a)
                {
                    fwprintf_s(m_fileHandle, L"<td style=\"color:#%02x%02x%02x; padding-right:50px;\">%s</td>\n", c.first.r, c.first.g, c.first.b, c.second.wcstr());
                    wprintf_s(L"\x1b[38;2;%u;%u;%um%-*s\x1b[0m", c.first.r, c.first.g, c.first.b, static_cast<int>(b->second + 5), c.second.wcstr());
                    ++b;
                }

                fwprintf_s(m_fileHandle, L"</tr>\n");
                wprintf_s(L"\n");
            }

            fwprintf_s(m_fileHandle, L"</table></div><br>\n");
            fflush(m_fileHandle);
            wprintf_s(L"\n");
        }

        FILE* m_fileHandle;
        String m_logName;
    };
}

moraine::Table moraine::createTable(Stringr title, Color color, std::initializer_list<String> columns)
{
    return std::make_shared<Table_I>(title, color, columns);
}

moraine::Table moraine::createTable(Stringr title, Color color, std::vector<String>& columns)
{
    return std::make_shared<Table_I>(title, color, columns);
}

moraine::Logfile moraine::createLogfile(Stringr path, Stringr title)
{
    return std::make_shared<Logfile_I>(path, title);
}
