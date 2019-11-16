#pragma once

#include "mrn_object.h"
#include "mrn_buffer.h"
#include "mrn_constset.h"

namespace moraine
{
    struct GraphicsParameters
    {
        GraphicsParameters(Shader shader, 
                           IndexBuffer indexBuffer, 
                           std::initializer_list<VertexBuffer> vertexBuffers,
                           std::initializer_list<ConstantSet> constantSets,
                           std::initializer_list<ConstantArray> constantArrays,
                           std::initializer_list<uint32_t> constantArrayIndicies,
                           uint32_t vertexCount,
                           uint32_t instanceCount) :
            m_shader(shader),
            m_indexBuffer(indexBuffer),
            m_vertexBuffers(vertexBuffers),
            m_constantSets(constantSets),
            m_constantArrays(constantArrays),
            m_constantArrayIndicies(constantArrayIndicies),
            m_vertexCount(vertexCount),
            m_instanceCount(instanceCount)
        { }

        Shader                      m_shader;
        IndexBuffer                 m_indexBuffer;
        std::vector<VertexBuffer>   m_vertexBuffers;
        std::vector<ConstantSet>    m_constantSets;
        std::vector<ConstantArray>  m_constantArrays;
        std::vector<uint32_t>       m_constantArrayIndicies; // UINT32_MAX = create new index
        uint32_t                    m_vertexCount;
        uint32_t                    m_instanceCount;
    };

    class Object_Graphics_T : public Object_T
    {
        friend class Renderer_IVulkan;

    public:

        Object_Graphics_T(std::shared_ptr<GraphicsParameters> params) :
            m_graphicsParameters(params),
            m_constantArrayIndicies(params->m_constantArrayIndicies)
        {
            for (uint32_t i = 0; i < m_constantArrayIndicies.size(); ++i)
                if (m_constantArrayIndicies[i] == UINT32_MAX)
                    m_constantArrayIndicies[i] = m_graphicsParameters->m_constantArrays[i]->addElement();
        }

        ~Object_Graphics_T() = default;

        inline ObjectType type() { return OBJECT_TYPE_GRAPHICS; }

    protected:

        std::shared_ptr<GraphicsParameters> m_graphicsParameters;
        std::vector<uint32_t> m_constantArrayIndicies;
    };
}