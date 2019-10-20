#pragma once

namespace moraine
{
    class Time
    {
    public:

        inline float getNanosecondsF() const        { return static_cast<float>(m_nanoseconds); }
        inline float getMicrosecondsF() const       { return static_cast<float>(m_nanoseconds) / 1000.0f; }
        inline float getMillisecondsF() const       { return static_cast<float>(m_nanoseconds) / 1000000.0f; }
        inline float getSecondsF() const            { return static_cast<float>(m_nanoseconds) / 1000000000.0f; }
        inline float getMinutesF() const            { return static_cast<float>(m_nanoseconds) / 60000000000.0f; }
        inline float getHoursF() const              { return static_cast<float>(m_nanoseconds) / 3600000000000.0f; }

        inline uint64_t getNanosecondsU() const     { return m_nanoseconds; }
        inline uint64_t getMicrosecondsU() const    { return m_nanoseconds / 1000; }
        inline uint64_t getMillisecondsU() const    { return m_nanoseconds / 1000000; }
        inline uint32_t getSecondsU() const         { return static_cast<uint32_t>(m_nanoseconds / 1000000000u); }
        inline uint32_t getMinutesU() const         { return static_cast<uint32_t>(m_nanoseconds / 60000000000ull); }
        inline uint32_t getHoursU()const            { return static_cast<uint32_t>(m_nanoseconds / 3600000000000ull); }

        enum NanoFormat
        {
            SECONDS,
            MILLISECONDS,
            MICROSECONDS,
            NANOSECONDS
        };

        // See http://www.cplusplus.com/reference/ctime/strftime/ for 'format' parameter
        MRN_API String timestamp(const String& format, NanoFormat nanoFormat);

        // Return the current time
        MRN_API static Time now();

        // Return the duration between two timepoints
        static Time duration(Time _1, Time _2) { return _2.m_nanoseconds - _1.m_nanoseconds; }

    private:

        Time(uint64_t rawTime) { m_nanoseconds = rawTime; }

        uint64_t m_nanoseconds;
    };
}