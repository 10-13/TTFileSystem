namespace TTFileSystem
{
    using num_t = unsigned long long;
    using num32_t = unsigned int;
    using byte_t = unsigned char;

    template<typename T, num_t size>
    using array_type = T[size];

    template<typename T>
    T* simplify_new()
    {
        return reinterpret_cast<T*>(new byte_t[sizeof(T)]);
    }

    template<num_t BLOCK, num_t SBLOCK, num_t DESCRIPTOS, num_t SBLOCK_COUNT>
    struct StaticInstance
    {
        struct Header
        {
            const num_t block_size{ BLOCK },
                super_block_size{ SBLOCK },
                descriptors_count{ DESCRIPTOS },
                super_block_count{ SBLOCK_COUNT };

            const array_type<num_t, 4> user_data;
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

        struct Block
        {
            array_type<byte_t, BLOCK> data;
        };

        struct SuperBlock
        {
            constexpr static const num_t EVALUATED_BITSET_SIZE = (SBLOCK + 7) / 8;

            num_t taken_blocks;
            array_type<byte_t, EVALUATED_BITSET_SIZE> bit_taken_flags;
            array_type<Block, SBLOCK> data;
        };

        struct PointerBlock
        {
            array_type<num_t, BLOCK / sizeof(num_t)> offests;
        };

        Header header;
        array_type<Descriptor, DESCRIPTOS> descriptors;
        array_type<SuperBlock, SBLOCK_COUNT> data;

        void initialize_empty()
        {
            for (auto&& i : descriptors)
                i.header.flags = 0;
        }

        num_t create_file(SecurityAttributes attribs)
        {
            num_t index = -1;
            for (; index < DESCRIPTOS; index++)
                if (descriptors[index].attributes.flags & Descriptor::SecurityAttributes::EX == 0)
                    break;

            if (index == DESCRIPTOS)
                return -1;

            Descriptor& trg_dsc = descriptors[index];
            for (auto i = (byte_t*)(&trg_dsc) + 1; i != (byte_t*)(&trg_dsc + 1); i++)
                *i = 0;

            trg_dsc.attributes = attribs;
            trg_dsc.attributes.flags |= Descriptor::SecurityAttributes::EX;

            return index;
        }

        num_t craete_file(byte_t creation_flags = 0, num32_t group_id = 0, num32_t user_id = 0)
        {
            num_t r = creation_flags << (32 + 24);
            r |= (group_id & 0x00ffffffU) << 32;
            r |= user_id;
            return create_file(reinterpret_cast<SecurityAttributes>(num_t));
        }

        num_t get_offest(Descriptor& desc)
        {
            num_t addr = &desc;
            if (addr < (num_t)this + sizeof(Header) || addr > (numt_t)(&descriptors[DESCRIPTOS - 1]))
                return -1;

            addr -= (num_t)this + sizeof(Header);
            return addr / sizeof(Descriptor);
        }

        constexpr static const num_t DATA_SPACE = BLOCK * SBLOCK * SBLOCK_COUNT;
        constexpr static const num_t TOTAL_SAPCE = sizeof(Header) + sizeof(array_type<Descriptor, DESCRIPTOS>) + sizeof(array_type<SuperBlock, SBLOCK_COUNT>);
    };

    template<num_t BLOCK, num_t SBLOCK, num_t DESCRIPTOS, num_t SBLOCK_COUNT>
    class DynamicHandle
    {
    private:
        using d_type = StaticInstance<BLOCK, SBLOCK, DESCRIPTOS, SBLOCK_COUNT>;
        d_type* data_;

    public:

        DynamicHandle()
        {
            data_ = simplify_new<d_type>();
            data_->initialize_empty();
        }
        DynamicHandle(void*) : data_(nullptr) {}

        DynamicHandle(const DynamicHandle&) = delete;
        DynamicHandle(DynamicHandle&& a)
        {
            data_ = a.data_;
            a.data_ = nullptr;
        }

        DynamicHandle& operator=(const DynamicHandle&) = delete;
        DynamicHandle& operator=(DynamicHandle&& a)
        {
            data_ = a.data_;
            a.data_ = nullptr;
        }

        ~DynamicHandle()
        {
            if (data_ != nullptr)
                delete data_;
        }

        d_type& data()
        {
            return *reinterpret_cast<d_type*>(data_);
        }
    };
}

int main()
{
    
    return 0;
}