#include "fsmeminstance.hpp"
#include <iostream>
#include <iomanip>

int main()
{
    using inst_t = TTFileSystem::MemoryInstance<1024, 256, 256>;

    auto inst = inst_t{};
    auto print_payload = [&inst]() {
        uint32_t i = inst.payload();
        float payload = inst.payload();
        payload /= inst.BlockCount;
        payload *= 100;
        int w = std::log10(inst.BlockCount) + 1;
        std::cout << std::fixed << std::setprecision(2) << std::setw(6) << payload << "% [" << std::setw(w) << inst.payload() << '/' << std::setw(w) << inst.BlockCount << "]\n";
        };
    {
        auto file = inst_t::FileReference::fileAt(0, &inst);
        if (!file.exsits())
            file.createFile();
        file.resizeFile(3675);
        print_payload();
    }
    {
        auto file = inst_t::FileReference::fileAt(1, &inst);
        if (!file.exsits())
            file.createFile();
        file.resizeFile(3675);
        print_payload();
    }
    {
        auto file = inst_t::FileReference::fileAt(0, &inst);
        file.resizeFile(1024 * 1024 * 20);
        print_payload();
    }
    {
        auto file = inst_t::FileReference::fileAt(0, &inst);
        file.resizeFile(1024 * 1024 * 2);
        print_payload();
    }
    {
        auto file = inst_t::FileReference::fileAt(0, &inst);
        file.deletFile();
        print_payload();
    }
    {
        auto file = inst_t::FileReference::fileAt(1, &inst);
        file.deletFile();
        print_payload();
    }
    
    return 0;
}