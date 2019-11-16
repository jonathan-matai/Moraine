#pragma once

namespace moraine
{
    typedef bool bRemove;

    typedef uint32_t ObjectType;

    enum ObjectTypeFlags
    {
        OBJECT_TYPE_INVISIBLE   = 0b00 << 30,
        OBJECT_TYPE_GRAPHICS    = 0b01 << 30,
        OBJECT_TYPE_AUDIO       = 0b10 << 30
    };

    class Object_T
    {
    public:

        virtual ~Object_T() = default;

        virtual bRemove tick(float delta, uint32_t frameIndex) = 0;

        

        virtual ObjectType type() = 0;
    };
}