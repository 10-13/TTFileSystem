#include "fsmeminstance.hpp"
#include <iostream>




int main()
{
    using inst_t = TTFileSystem::MemoryInstance<4096, 1024, 256>;
    using file_t = inst_t::FileReference;

    auto inst = inst_t{};
    auto file = file_t::fileAt(0, &inst);
    file.createFile();
    {
        inst_t::OFileStream str{ file };
        str << "Hello!";
    }
    {
        auto beg = file_t::DataIterator<TTFileSystem::byte_t>{ &file, 0, 0 };
        auto end = file_t::DataIterator<TTFileSystem::byte_t>{ &file };
    }
    
    return 0;
}