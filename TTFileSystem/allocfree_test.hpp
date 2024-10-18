#include "fsmeminstance.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>

#define LARGE

#define TIME_MESURE(a) { \
auto start = std::chrono::high_resolution_clock::now(); \
{ a } \
auto end = std::chrono::high_resolution_clock::now(); \
std::chrono::duration<double, std::milli> elapsed = end - start; \
std::cout << "Time: " << elapsed.count() << std::endl; \
}

void test()
{
    using inst_t = TTFileSystem::MemoryInstance<4096, 4096, 256>;

    auto inst = inst_t{};
    auto print_payload = [&inst]()
        {
            uint64_t dev = 1024 * 1024;
            auto szf = "MB";

            uint32_t i = inst.payload();
            float payload = inst.payload() * inst_t::BlockType::Size / dev;
            payload /= inst.TotalSize;
            payload *= 100;
            int w = std::log10(inst.TotalSize / dev) + 1;
            std::cout << std::fixed << std::setprecision(2) << std::setw(6) << payload << "% [" << std::setw(w) << inst.payload() * inst_t::BlockType::Size / dev << '/' << std::setw(w) << inst.TotalSize / dev << szf << "]\n";
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
#ifdef LARGE
    {
        auto file = inst_t::FileReference::fileAt(0, &inst);
        TIME_MESURE(file.resizeFile(1024ULL * 1024 * 3072););
        print_payload();
    }
    {
        auto file = inst_t::FileReference::fileAt(0, &inst);
        TIME_MESURE(file.resizeFile(3675););
        print_payload();
    }
    {
        auto file = inst_t::FileReference::fileAt(0, &inst);
        TIME_MESURE(file.resizeFile(1024 * 1024 * 500););
        print_payload();
    }
    {
        auto file = inst_t::FileReference::fileAt(1, &inst);
        TIME_MESURE(file.resizeFile(1024 * 1024 * 500););
        print_payload();
    }
    {
        auto file = inst_t::FileReference::fileAt(1, &inst);
        TIME_MESURE(file.resizeFile(1024 * 1024 * 2););
        print_payload();
    }
    {
        auto file = inst_t::FileReference::fileAt(1, &inst);
        TIME_MESURE(file.resizeFile(1024 * 1024 * 500););
        print_payload();
    }
#endif
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
    {
        constexpr const int FileCount = 512;
        constexpr const int FileSize = 1024 * 1024;

        TIME_MESURE(
            for (int i = 0; i < FileCount; i++)
            {
                auto file = inst_t::FileReference::fileAt(i + 2, &inst);
                if (!file.exsits())
                    file.createFile();
                file.resizeFile(FileSize);
            }
        );
        print_payload();
        TIME_MESURE(
            for (int i = 0; i < FileCount; i++)
            {
                auto file = inst_t::FileReference::fileAt(i + 2, &inst);
                if (file.exsits())
                    file.deletFile();
            }
        );
        print_payload();
    }
    {
        constexpr const int FileCount = 4096 * 256 / 4 - 3;
        constexpr const int FileSize = 1024;

        TIME_MESURE(
            for (int i = 0; i < FileCount; i++)
            {
                auto file = inst_t::FileReference::fileAt(i + 2, &inst);
                if (!file.exsits())
                    file.createFile();
                file.resizeFile(FileSize);
            }
        );
        print_payload();
        TIME_MESURE(
            for (int i = 0; i < FileCount; i++)
            {
                auto file = inst_t::FileReference::fileAt(i + 2, &inst);
                if (file.exsits())
                    file.deletFile();
            }
        );
        print_payload();
    }
}