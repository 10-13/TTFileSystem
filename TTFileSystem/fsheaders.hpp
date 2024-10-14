#include <cstdint>
#include <concepts>
#include <array>
#include <stdexcept>
#include <cstddef>

namespace TTFileSystem
{
    using num_t = uint64_t;
    using num32_t = uint32_t;
    using byte_t = uint8_t;

    template<typename T, num_t size>
    using array_type = std::array<T, size>;

    namespace Primitives
    {
        constexpr num_t CountBits(num_t t)
        {
            num_t a = t % 2;
            while ((t /= 2) > 0)
                a += t % 2;
            return a;
        }

        template<num_t Value>
        concept SingleBitNumber = (CountBits(Value) == 1);

        template<num_t Value>
        concept PointerMultipleNumber = (Value % sizeof(num_t) == 0);

        template<num_t BlockSize>
        requires PointerMultipleNumber<BlockSize>
        struct Block
        {
            struct PointerBlock
            {
                constexpr const static num_t Size = BlockSize / sizeof(num_t);

                array_type<num_t, Size> ptrs;

                num_t getAllocatedCount() {
                    for (num_t i = Size - 1; i > 0; i++)
                        if (ptrs[i] != 0)
                            return i + 1;
                    return 0;
                }
            };

            constexpr const static num_t Size = BlockSize;

            array_type<byte_t, BlockSize> data;
        };

        template<num_t SuperBlockSize, num_t BlockSize>
            requires (SuperBlockSize % 8 == 0)
        struct SuperBlock
        {
            constexpr static const num_t BitDataSize = SuperBlockSize / 8;
            constexpr static const num_t Size = SuperBlockSize;

            using BlockType = Block<BlockSize>;

            num_t taken_amount;
            array_type<byte_t, BitDataSize> taken_flags;
            array_type<BlockType, SuperBlockSize> data;
        };

        struct Descriptor
        {
            struct SecurityAttributes
            {
                enum Flags
                {
                    RG = 0b00000001,
                    WG = 0b00000010,
                    VG = 0b00000100,
                    RE = 0b00001000,
                    WE = 0b00010000,
                    VE = 0b00100000,
                    DR = 0b01000000,
                    EX = 0b10000000,
                };

                byte_t flags;
                array_type<byte_t, 3> group_id;
                num32_t user_id;
            };
            struct FileHeader
            {
                num_t size;
                num_t creation_time;
                num_t name_ptr;

            };
            struct FileData
            {
                num_t data_0_ptr;
                num_t data_1_ptr;
                num_t data_2_ptr;
                num_t data_3_ptr;
            };

            SecurityAttributes attributes;
            FileHeader header;
            FileData data;
        };

        struct Header
        {
            num_t block_size;
            num_t super_block_size;
            num_t descriptors_count;
            num_t super_block_count;

            array_type<num_t, 4> user_data;
        };
    }
}