#include "fsmeminstance.hpp"

int main()
{
    auto inst = TTFileSystem::MemoryInstance<64, 64, 128>{};
    auto& desc = inst.getDescriptor(0);
    auto of_base = (TTFileSystem::num_t)(&inst);
    auto& ptr_block = inst.getPtrBlock(0);
    desc.data.data_3_ptr = 0;
    for (auto&& i : ptr_block.ptrs)
        i = 0;
    inst.getIndexedPtr(0, 128);
    return 0;
}