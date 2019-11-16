#pragma once

#include "mrn_object.h"

#include <list>

namespace moraine
{
    class Layer_T
    {
    public:

        Layer_T(Stringr name) :
            m_name(name)
        { }

        virtual ~Layer_T() = default;

        virtual void tick(float delta, uint32_t frameIndex) = 0;

        virtual void add(std::unique_ptr<Object_T>&& object) = 0;

        Stringr name() const { return m_name; }

        virtual std::list<std::unique_ptr<Object_T>>::iterator begin() = 0;
        virtual std::list<std::unique_ptr<Object_T>>::iterator end() = 0;

    private:

        String m_name;
    };

    typedef std::shared_ptr<Layer_T> Layer;

    MRN_API Layer createLayer(Stringr layerName);
}