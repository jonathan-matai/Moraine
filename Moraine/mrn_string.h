#pragma once


namespace moraine
{
    class String
    {
    public:

        MRN_API String();
        MRN_API String(const char* raw);
        MRN_API String(const wchar_t* raw);
        MRN_API ~String();
        MRN_API String(const String& other);            // Copy constructor
        MRN_API String(String&& other);                 // Move constructor
        MRN_API String& operator=(const String& other); // Copy assignment operator
        MRN_API String& operator=(String&& other);      // Move assignment operator
        
        MRN_API bool operator==(const String& other) const;
        MRN_API bool operator==(const char* other) const;
        MRN_API bool operator==(const wchar_t* other) const;

        MRN_API char* mbbuf() const;
        MRN_API wchar_t* wcbuf() const;

        inline const char* mbstr() const
        {
            return mbbuf();
        }

        inline const wchar_t* wcstr() const
        {
            return wcbuf();
        }

        inline operator const char* () const
        {
            return mbstr();
        }

        inline operator const wchar_t* () const
        {
            return wcstr();
        }

        size_t length() const
        {
            return m_size;
        }

        size_t size() const
        {
            return m_size;
        }

    private:

        mutable char*       m_mbbuf;
        mutable wchar_t*    m_wcbuf;
        size_t              m_size;
    };

    MRN_API String sprintf(const char* format, ...);
    MRN_API String sprintf(const wchar_t* format, ...);

    typedef const String& Stringr;
}