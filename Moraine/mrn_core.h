#pragma once

#ifdef MORAINE_EXPORTS

#define MRN_API __declspec(dllexport)

#else

#define MRN_API __declspec(dllimport)
#pragma comment(lib, "Moraine.lib")

#endif

#ifdef assert
#undef assert
#endif

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#include "mrn_math.h"
#include <memory>
#include <initializer_list>
#include <functional>
#include <algorithm>

#include <list>

namespace moraine
{
    struct Color
    {
        constexpr Color(uint8_t _r, uint8_t _g, uint8_t _b) :
            r(_r), g(_g), b(_b) { }

        constexpr Color(uint32_t colorCode) :
            r((colorCode & 0xff0000) >> 16),
            g((colorCode & 0x00ff00) >> 8),
            b(colorCode & 0x0000ff)
        { }

        uint8_t r, g, b;
    };

    constexpr Color RED(0xff0000);
    constexpr Color GREEN(0x00ff00);
    constexpr Color BLUE(0x0040ff);
    constexpr Color WHITE(0xffffff);
    constexpr Color GREY(0x808080);
    constexpr Color YELLOW(0xffff00);
    constexpr Color CYAN(0x00ffff);
    constexpr Color MAGENTA(0xff00ff);
    constexpr Color ORANGE(0xff6000);
}

namespace mrn = moraine;

#include "mrn_string.h"
#include "mrn_time.h"
#include "mrn_logfile.h"

namespace moraine
{
    inline void assert(Logfile logfile, bool assertion, Stringr message, const DebugInfo& debugInfo)
    {
        if (not assertion)
        {
            logfile->print(RED, message, debugInfo);
            throw std::exception();
        }
    }
}