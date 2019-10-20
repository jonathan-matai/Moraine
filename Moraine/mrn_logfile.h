#pragma once

#include <vector>

namespace moraine
{
    struct DebugInfo
    {
        const char* file;
        int         line;
        const char* function;
    };

    class Table_T
    {
    public:

        virtual ~Table_T() = default;

        virtual void addRow(Color color, std::initializer_list<String> row) = 0;
        virtual void addRow(std::initializer_list<std::pair<Color, String>> row) = 0;

        virtual void addColumn(Color color, std::initializer_list<String> row) = 0;
        virtual void addColumn(std::initializer_list<std::pair<Color, String>> row) = 0;

        virtual void dbgconv() = 0;
    };

    typedef std::shared_ptr<Table_T> Table;

    MRN_API Table createTable(Stringr title, Color color, std::initializer_list<String> columns);
    MRN_API Table createTable(Stringr title, Color color, std::vector<String>& columns);

    class Logfile_T
    {
    public:

        virtual ~Logfile_T() = default;

        virtual void print(Color color, Stringr string) = 0;
        virtual void print(Color color, Stringr string, DebugInfo debugInfo) = 0;
        virtual void print(Table table) = 0;
    };

    typedef std::shared_ptr<Logfile_T> Logfile;

    MRN_API Logfile createLogfile(Stringr path, Stringr title);
}

#define MRN_DEBUG_INFO moraine::DebugInfo({ __FILE__, __LINE__, __FUNCSIG__ })