#include "fsheaders.hpp"

namespace TTFileSystem
{
	template<num_t SuperBlockSize, num_t SuperBlockCount>
	constexpr const num_t DefaultDescriptorsCount = SuperBlockCount * SuperBlockSize / 4;

	template<num_t BlockSize, num_t SuperBlockSize, num_t SuperBlockCount, num_t DescriptorCount = DefaultDescriptorsCount<SuperBlockSize, SuperBlockCount>>
	struct MemoryInstance
	{
	public:
		using SuperBlockType = Primitives::SuperBlock<SuperBlockSize, BlockSize>;
		using BlockType = SuperBlockType::BlockType;
		using PtrBlockType = BlockType::PointerBlock;
		using NameBlock = BlockType::NameBlock;

		constexpr const static num_t TotalSize = sizeof(Primitives::Header) + DescriptorCount * sizeof(Primitives::Descriptor) + SuperBlockCount * sizeof(SuperBlockType);
		constexpr const static num_t BlockCount = SuperBlockSize * SuperBlockCount;

		constexpr const static num_t SuperBlocksOffset = sizeof(Primitives::Header) + DescriptorCount * sizeof(Primitives::Descriptor);
		constexpr const static num_t BlockOffset = offsetof(SuperBlockType, data);
		constexpr const static num_t DescriptorsOffset = sizeof(Primitives::Header);
	public:
		byte_t* data_;

		template<typename T>
		T* getOffsetedPtr(num_t offset, num_t index)
		{
			return (T*)(data_ + offset + index * sizeof(T));
		}

		Primitives::Header& getHeader()
		{
			return *getOffsetedPtr<Primitives::Header>(0, 0);
		}

		Primitives::Descriptor& getDescriptor(num_t index = 0)
		{
			if (index >= DescriptorCount)
				throw new std::out_of_range("Accessing non descriptor data.");
			return *getOffsetedPtr<Primitives::Descriptor>(DescriptorsOffset, index);
		}

		SuperBlockType& getSuperBlock(num_t index = 0)
		{
			if (index >= SuperBlockCount)
				throw new std::out_of_range("Accessing non instance data.");
			return *getOffsetedPtr<SuperBlockType>(SuperBlocksOffset, index);
		}

		SuperBlockType& getSuperBlockByBlockIndex(num_t global_index) {
			return getSuperBlock(global_index / SuperBlockSize);
		}

		BlockType& getBlock(num_t global_index)
		{
			if (global_index >= BlockCount)
				throw new std::out_of_range("Accessing non instance data.");
			return *getOffsetedPtr<BlockType>(SuperBlocksOffset + sizeof(SuperBlockType) * (global_index / SuperBlockSize) + BlockOffset, global_index % SuperBlockSize);
		}

		PtrBlockType& getPtrBlock(num_t global_index) {
			return reinterpret_cast<PtrBlockType&>(getBlock(global_index));
		}

		constexpr static num_t CPower(num_t Number, num_t Power) {
			num_t res = 1;
			num_t mult = Number;
			while (Power > 0) {
				if (Power % 2 == 1)
					res *= mult;
				mult *=  mult;
				Power >>= 1;
			}
			return res;
		}

		template<num_t Depth>
		num_t getIndexedPtr(PtrBlockType& block, num_t index) {
			if constexpr (Depth == 0)
				return block.ptrs[index];

			num_t power = CPower(block.Size, Depth);
			PtrBlockType* f_block = &block;
			for (num_t i = Depth; i > 0; i--) {
				num_t addr = index / power;
				f_block = &getPtrBlock(f_block->ptrs[addr]);
				index %= power;
				power /= block.Size;
			}

			return f_block->ptrs[index];
		}

		num_t getIndexedPtr(num_t descriptor_index, num_t ptr_index) {
			constexpr const num_t Size0 = 1;
			constexpr const num_t Size1 = PtrBlockType::Size;
			constexpr const num_t Size2 = Size1 * PtrBlockType::Size;
			constexpr const num_t Size3 = Size2 * PtrBlockType::Size;

			auto desc = getDescriptor(descriptor_index);

			if (ptr_index < Size0)
				return desc.data.data_0_ptr;

			ptr_index -= Size0;
			if (ptr_index < Size1)
				return getPtrBlock(desc.data.data_1_ptr).ptrs[ptr_index];

			ptr_index -= Size1;
			if (ptr_index < Size2) {
				return getIndexedPtr<1>(getPtrBlock(desc.data.data_2_ptr), ptr_index);
			}

			ptr_index -= Size2;
			if (ptr_index < Size3) {
				return getIndexedPtr<2>(getPtrBlock(desc.data.data_3_ptr), ptr_index);
			}
		}

		num_t getFreeBlock() {
			for (num_t i = 0; i < SuperBlockCount; i++)
				if (getSuperBlock(i).taken_amount < SuperBlockSize) {
					return getSuperBlock(i).firstFreeIndex();
				}
		}

		template<bool Emptify = false>
		num_t allocateSingleBlock() {
			num_t free = getFreeBlock();
			SuperBlockType& sb = getSuperBlockByBlockIndex(free);
			sb.allocBlock(free % SuperBlockSize);
			if constexpr (Emptify)
				getBlock(free).initEmpty();
			return free;
		}

		void freeSingleBlock(num_t block) {
			SuperBlockType& sb = getSuperBlockByBlockIndex(block);
			sb.freeBlock(block);
		}

		byte_t* transfer()
		{
			auto tmp = data_;
			data_ = nullptr;
			return tmp;
		}

	public:
		MemoryInstance()
		{
			data_ = (byte_t*)malloc(TotalSize);
			auto bl = getBlock(0);
			for (auto&& i : bl.data)
				i = 0;
			for (num_t i = 0; i < SuperBlockCount; i++)
				getSuperBlock(i).initEmpty();
			getSuperBlock(0).allocBlock(0);
			for (num_t i = 0; i < DescriptorCount; i++)
				getDescriptor(i).attributes.flags = 0;
		}

		MemoryInstance(const MemoryInstance&) = delete;
		MemoryInstance(MemoryInstance&& a)
		{
			if (&a != this)
				data_ = transfer();
		}

		MemoryInstance& operator=(const MemoryInstance&) = delete;
		MemoryInstance& operator=(MemoryInstance&& a)
		{
			~MemoryInstance();

			if (&a != this)
				data_ = transfer();
		}

		~MemoryInstance()
		{
			if (data_ != nullptr)
				free(data_);
		}

		struct FileReference {
		private:
			MemoryInstance* mem_inst;
			num_t index;

			num_t allocateInPlace(PtrBlockType& block, int index) {
				if (block.ptrs[index] == 0)
					block.ptrs[index] = mem_inst->allocateSingleBlock<true>();
				return block.ptrs[index];
			}

			void allocateBlock(num_t index) {
				constexpr const num_t Size0 = 1;
				constexpr const num_t Size1 = PtrBlockType::Size;
				constexpr const num_t Size2 = Size1 * PtrBlockType::Size;
				constexpr const num_t Size3 = Size2 * PtrBlockType::Size;

				auto& desc = descriptor().data;

				if (index == 0)
					if (desc.data_0_ptr == 0)
						desc.data_0_ptr = mem_inst->allocateSingleBlock<true>();

				if (index < Size0)
					return;
				index -= Size0;
				if (index == 0)
					if (desc.data_1_ptr == 0)
						desc.data_1_ptr = mem_inst->allocateSingleBlock<true>();
				{
					auto& block = mem_inst->getPtrBlock(desc.data_1_ptr);
					allocateInPlace(block, index);
				}

				if (index < Size1)
					return;
				index -= Size1;
				if (index == 0)
					if (desc.data_2_ptr == 0)
						desc.data_2_ptr = mem_inst->allocateSingleBlock<true>();

				{
					auto& block1 = mem_inst->getPtrBlock(desc.data_2_ptr);
					auto& block2 = mem_inst->getPtrBlock(allocateInPlace(block1, index / Size1));
					allocateInPlace(block2, index % Size1);
				}

				if (index < Size2)
					return;
				index -= Size2;
				if (index == 0)
					if (desc.data_3_ptr == 0)
						desc.data_3_ptr = mem_inst->allocateSingleBlock<true>();

				{
					auto& block1 = mem_inst->getPtrBlock(desc.data_3_ptr);
					auto& block2 = mem_inst->getPtrBlock(allocateInPlace(block1, index / Size2));
					auto& block3 = mem_inst->getPtrBlock(allocateInPlace(block2, (index / Size1) % Size1));
					allocateInPlace(block3, index % Size1);
				}
			}

			void allocate(num_t amount) {
				auto& desc = descriptor();
				while (amount-- > 0) {
					num_t allocated_blocks = (desc.header.size + BlockSize - 1) / BlockSize;
					allocateBlock(allocated_blocks);
					desc.header.size += BlockSize;
				}
			}

			FileReference() = default;

		public:
			
			Primitives::Descriptor& descriptor() {
				return mem_inst->getDescriptor(index);
			}

			bool exsits() {
				return descriptor().attributes.flags && descriptor().attributes.EX;
			}

			void set_name(std::string name, bool except_on_oversize = true);

			void deletFile() {
				descriptor().attributes.flags &= !descriptor().attributes.EX;
			}
			void createFile() {
				if (exsits())
					throw new std::bad_alloc();

				descriptor().attributes.flags |= descriptor().attributes.EX;
				descriptor().initEmpty();
			}
			void resizeFile(num_t new_size) {
				auto& desc = descriptor();
				num_t allocated_blocks = (desc.header.size + BlockSize - 1) / BlockSize;
				num_t required_block = (new_size + BlockSize - 1) / BlockSize;
				if (required_block > allocated_blocks)
					allocate(required_block - allocated_blocks);
				auto& pb = mem_inst->getPtrBlock(desc.data.data_1_ptr);
			}

			static FileReference fileAt(num_t index, MemoryInstance* src) {
				FileReference res{};
				res.index = index;
				res.mem_inst = src;
				return res;
			}
		};
	};
}