#pragma once

class Spiral : public mrn::Object_Graphics_T
{
public:

    struct ConstantBufferData
    {
        mrn::float2 center;
        mrn::float3 color;
        float       angleRad;
    };

    int i = sizeof(ConstantBufferData);

    static std::shared_ptr<mrn::GraphicsParameters> s_graphicsParameters;

    Spiral(mrn::float2 center, mrn::Color color) :
        Object_Graphics_T(s_graphicsParameters),
        m_angleDeg(0)
    {
        for (uint32_t i = 0; i < 3; ++i)
            *static_cast<ConstantBufferData*>(m_graphicsParameters->m_constantArrays[0]->data(i, m_constantArrayIndicies[0])) =
            {
                center,
                mrn::float3(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f),
                0
            };
    }

    mrn::bRemove tick(float delta, uint32_t frameIndex)
    {
        m_angleDeg += (delta * 0.3f);

        if (m_angleDeg > 360.0f)
            m_angleDeg -= 360.0f;

        static_cast<ConstantBufferData*>(m_graphicsParameters->m_constantArrays[0]->data(frameIndex, m_constantArrayIndicies[0]))->angleRad = mrn::degToRad(m_angleDeg);

        return false;
    }

    float m_angleDeg;
};