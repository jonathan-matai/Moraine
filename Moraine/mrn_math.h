#pragma once

#include <cstdint>

#include "mrn_vector.h"

namespace moraine
{
    template<typename T>
    inline T min(const T& a, const T& b)
    {
        return a < b ? a : b;
    }

    template<typename T>
    inline T max(const T& a, const T& b)
    {
        return a > b ? a : b;
    }

    template<typename T>
    inline T clamp(const T& min, const T& value, const T& max)
    {
        return value < min ? min : (value > max ? max : value);
    }

    constexpr double PI = 3.141592653589793116;

    template<typename T>
    inline T degToRad(T degrees)
    {
        return static_cast<T>(degrees / 180.0 * PI);
    }

    template<typename T>
    inline T radToDeg(T radians)
    {
        return static_cast<T>(radians / PI * 180);
    }
}