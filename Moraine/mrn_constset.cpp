#include "mrn_constset.h"
#include "mrn_constset_vk.h"

moraine::ConstantSet moraine::createConstantSet(Shader shader, uint32_t set, std::initializer_list<std::pair<ConstantResource, uint32_t>> resources)
{
    return std::make_shared<ConstantSet_IVulkan>(shader, set, resources);
}
