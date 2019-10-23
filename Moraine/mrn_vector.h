#pragma once

#include <immintrin.h>

namespace moraine
{
    /*
    Explanation of third parameter of the function _mm_dp_ps:

    0b[0][1][2][3][4][5][6][7]

    [0]: calculate with w-component
    [1]: calculate with z-component
    [2]: calculate with y-component
    [3]: calculate with x-component

    [4]: write output to w-component
    [5]: write output to z-component
    [6]: write output to y-component
    [7]: write output to x-component

    Explanation of second parameter of the function _mm_permute_ps:

    0b[01][23][45][67]

    [01] The value stored in w (00 = x, 01 = y, 10 = z, 11 = w)
    [01] The value stored in z (00 = x, 01 = y, 10 = z, 11 = w)
    [01] The value stored in y (00 = x, 01 = y, 10 = z, 11 = w)
    [01] The value stored in w (00 = x, 01 = y, 10 = z, 11 = w)
    */

    union float2
    {
    public:

        __m128 m_sse;

        struct
        {
            float x, y;
        };

        float2()                                            { m_sse = _mm_set_ps1(0.0f); }
        float2(__m128 sse)                                  { m_sse = sse; }
        float2(float value)                                 { m_sse = _mm_set_ps(0.0f, 0.0f, value, value); };
        float2(float x, float y)                            { m_sse = _mm_set_ps(0.0f, 0.0f, y, x); }

        float2 operator+(const float2& vec)                 { return _mm_add_ps(m_sse, vec.m_sse); }
        float2 operator-(const float2& vec)                 { return _mm_sub_ps(m_sse, vec.m_sse); }
        float2 operator*(const float2& vec)                 { return _mm_mul_ps(m_sse, vec.m_sse); }
        float2 operator*(float value)                       { return _mm_mul_ps(m_sse, _mm_set_ps1(value)); }
        float2 operator/(const float2& vec)                 { return _mm_div_ps(m_sse, vec.m_sse); }
        float2 operator/(float value)                       { return _mm_mul_ps(m_sse, _mm_set_ps(1.0f, 1.0f, value, value)); }
              
        float2& operator+=(const float2& vec)               { return *this = _mm_add_ps(m_sse, vec.m_sse); }
        float2& operator-=(const float2& vec)               { return *this = _mm_sub_ps(m_sse, vec.m_sse); }
        float2& operator*=(const float2& vec)               { return *this = _mm_mul_ps(m_sse, vec.m_sse); }
        float2& operator*=(float value)                     { return *this = _mm_mul_ps(m_sse, _mm_set_ps1(value)); }
        float2& operator/=(const float2& vec)               { return *this = _mm_div_ps(m_sse, vec.m_sse); }
        float2& operator/=(float value)                     { return *this = _mm_mul_ps(m_sse, _mm_set_ps(1.0f, 1.0f, value, value)); }
    };

    inline float2 operator*(float value, const float2& vec) { return _mm_mul_ps(_mm_set_ps1(value), vec.m_sse); }
    inline float length(const float2& vec)                  { return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(vec.m_sse, vec.m_sse, 0b00110001))); }
    inline float2 normalize(const float2& vec)              { return _mm_mul_ps(vec.m_sse, _mm_rsqrt_ps(_mm_dp_ps(vec.m_sse, vec.m_sse, 0b00110011))); }
    inline float dot(const float2& a, const float2& b)      { return _mm_cvtss_f32(_mm_dp_ps(a.m_sse, b.m_sse, 0b00110001)); }

    union float3
    {
    public:

        __m128 m_sse;

        struct
        {
            float x, y, z;
        };

        float3()                                            { m_sse = _mm_set_ps1(0.0f); }
        float3(__m128 sse)                                  { m_sse = sse; }
        float3(float value)                                 { m_sse = _mm_set_ps(0.0f, value, value, value); };
        float3(float x, float y, float z)                   { m_sse = _mm_set_ps(0.0f, z, y, x); }

        float3 operator+(const float3& vec)                 { return _mm_add_ps(m_sse, vec.m_sse); }
        float3 operator-(const float3& vec)                 { return _mm_sub_ps(m_sse, vec.m_sse); }
        float3 operator*(const float3& vec)                 { return _mm_mul_ps(m_sse, vec.m_sse); }
        float3 operator*(float value)                       { return _mm_mul_ps(m_sse, _mm_set_ps1(value)); }
        float3 operator/(const float3& vec)                 { return _mm_div_ps(m_sse, vec.m_sse); }
        float3 operator/(float value)                       { return _mm_mul_ps(m_sse, _mm_set_ps(1.0f, value, value, value)); }
              
        float3& operator+=(const float3& vec)               { return *this = _mm_add_ps(m_sse, vec.m_sse); }
        float3& operator-=(const float3& vec)               { return *this = _mm_sub_ps(m_sse, vec.m_sse); }
        float3& operator*=(const float3& vec)               { return *this = _mm_mul_ps(m_sse, vec.m_sse); }
        float3& operator*=(float value)                     { return *this = _mm_mul_ps(m_sse, _mm_set_ps1(value)); }
        float3& operator/=(const float3& vec)               { return *this = _mm_div_ps(m_sse, vec.m_sse); }
        float3& operator/=(float value)                     { return *this = _mm_mul_ps(m_sse, _mm_set_ps(1.0f, value, value, value)); }
    };

    inline float3 operator*(float value, const float3& vec) { return _mm_mul_ps(_mm_set_ps1(value), vec.m_sse); }
    inline float length(const float3& vec)                  { return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(vec.m_sse, vec.m_sse, 0b01110001))); }
    inline float3 normalize(const float3& vec)              { return _mm_mul_ps(vec.m_sse, _mm_rsqrt_ps(_mm_dp_ps(vec.m_sse, vec.m_sse, 0b01110111))); }
    inline float dot(const float3& a, const float3& b)      { return _mm_cvtss_f32(_mm_dp_ps(a.m_sse, b.m_sse, 0b01110001)); }

    inline float3 cross(const float3& a, const float3& b)
    {
        return _mm_sub_ps(
            _mm_mul_ps(
            _mm_permute_ps(a.m_sse, 0b11001001),
            _mm_permute_ps(b.m_sse, 0b11010010)
        ),
            _mm_mul_ps(
            _mm_permute_ps(a.m_sse, 0b11010010),
            _mm_permute_ps(b.m_sse, 0b11001001)
        )
        );
    }

    union float4
    {
    public:

        __m128 m_sse;

        struct
        {
            float x, y, z, w;
        };

        float4()                                            { m_sse = _mm_set_ps1(0.0f); }
        float4(__m128 sse)                                  { m_sse = sse; }
        float4(float value)                                 { m_sse = _mm_set_ps(0.0f, 0.0f, value, value); };
        float4(float x, float y, float z, float w)          { m_sse = _mm_set_ps(w, z, y, x); }

        float4 operator+(const float4& vec)                 { return _mm_add_ps(m_sse, vec.m_sse); }
        float4 operator-(const float4& vec)                 { return _mm_sub_ps(m_sse, vec.m_sse); }
        float4 operator*(const float4& vec)                 { return _mm_mul_ps(m_sse, vec.m_sse); }
        float4 operator*(float value)                       { return _mm_mul_ps(m_sse, _mm_set_ps1(value)); }
        float4 operator/(const float4& vec)                 { return _mm_div_ps(m_sse, vec.m_sse); }
        float4 operator/(float value)                       { return _mm_mul_ps(m_sse, _mm_set_ps1(value)); }
              
        float4& operator+=(const float4& vec)               { return *this = _mm_add_ps(m_sse, vec.m_sse); }
        float4& operator-=(const float4& vec)               { return *this = _mm_sub_ps(m_sse, vec.m_sse); }
        float4& operator*=(const float4& vec)               { return *this = _mm_mul_ps(m_sse, vec.m_sse); }
        float4& operator*=(float value)                     { return *this = _mm_mul_ps(m_sse, _mm_set_ps1(value)); }
        float4& operator/=(const float4& vec)               { return *this = _mm_div_ps(m_sse, vec.m_sse); }
        float4& operator/=(float value)                     { return *this = _mm_mul_ps(m_sse, _mm_set_ps1(value)); }
    };

    inline float4 operator*(float value, const float4& vec) { return _mm_mul_ps(_mm_set_ps1(value), vec.m_sse); }
    inline float length(const float4& vec)                  { return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(vec.m_sse, vec.m_sse, 0b11110001))); }
    inline float4 normalize(const float4& vec)              { return _mm_mul_ps(vec.m_sse, _mm_rsqrt_ps(_mm_dp_ps(vec.m_sse, vec.m_sse, 0b11111111))); }
    inline float dot(const float4& a, const float4& b)      { return _mm_cvtss_f32(_mm_dp_ps(a.m_sse, b.m_sse, 0b11110001)); }
}