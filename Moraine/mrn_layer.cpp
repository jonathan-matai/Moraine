#include "mrn_core.h"
#include "mrn_layer.h"


namespace moraine
{
    class Layer_I : public Layer_T
    {
    public:

        Layer_I(Stringr name);

        void tick(float delta, uint32_t frameIndex) override;

        void add(std::unique_ptr<Object_T>&& object) override;

        std::list<std::unique_ptr<Object_T>>::iterator begin() override { return m_objects.begin(); }
        std::list<std::unique_ptr<Object_T>>::iterator end() override { return m_objects.end(); }

    private:


        std::list<std::unique_ptr<Object_T>> m_objects;
    };
}

moraine::Layer moraine::createLayer(Stringr layerName)
{
    return std::make_shared<Layer_I>(layerName);
}

void moraine::Layer_I::add(std::unique_ptr<Object_T>&& object)
{
    m_objects.push_back(std::move(object));
}


moraine::Layer_I::Layer_I(Stringr name) :
    Layer_T(name)
{ }

void moraine::Layer_I::tick(float delta, uint32_t frameIndex)
{
    for (auto a = m_objects.begin(); a != m_objects.end();)
        if ((**a).tick(delta, frameIndex))
            a = m_objects.erase(a);
        else
            ++a;
}
