#include "fsmeminstance.hpp"

int main()
{
    using inst_t = TTFileSystem::MemoryInstance<1024, 256, 256>;
    auto inst = inst_t{};
    {
        auto file = inst_t::FileReference::fileAt(0, &inst);
        if (!file.exsits())
            file.createFile();
        file.resizeFile(3675);
    }
    {
        auto file = inst_t::FileReference::fileAt(1, &inst);
        if (!file.exsits())
            file.createFile();
        file.resizeFile(3675);
    }
    {
        auto file = inst_t::FileReference::fileAt(0, &inst);
        file.resizeFile(100000);
    }
    
    return 0;
}