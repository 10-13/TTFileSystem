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
					return getSuperBlock(i).firstFreeIndex() + i * SuperBlockSize;
				}
			throw new std::bad_alloc();
		}

		template<num_t EmptifyAmount = 0>
		num_t allocateSingleBlock() {
			num_t free = getFreeBlock();
			SuperBlockType& sb = getSuperBlockByBlockIndex(free);
			sb.allocBlock(free % SuperBlockSize);
			auto& b = getBlock(free);
			for (num_t i = 0; i < EmptifyAmount; i++)
				b.data[i] = 0;
			return free;
		}

		void freeSingleBlock(num_t block) {
			SuperBlockType& sb = getSuperBlockByBlockIndex(block);
			sb.freeBlock(block % SuperBlockSize);
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

		num_t payload() {
			num_t res{0};
			for (num_t i = 0; i < SuperBlockCount; i++)
				res += getSuperBlock(i).taken_amount;
			return res;
		}

		struct FileReference {
		private:
			MemoryInstance* mem_inst;
			num_t index;

			constexpr static const num_t Size0 = 1;
			constexpr static const num_t Size1 = PtrBlockType::Size;
			constexpr static const num_t Size2 = Size1 * PtrBlockType::Size;
			constexpr static const num_t Size3 = Size2 * PtrBlockType::Size;

			num_t allocateInPlace(PtrBlockType& block, int index) {
				if (block.ptrs[index] == 0)
					block.ptrs[index] = mem_inst->allocateSingleBlock<8>();
				if (index < PtrBlockType::Size - 2)
					block.ptrs[index + 1] = 0;
				return block.ptrs[index];
			}

			void allocateBlock(num_t index) {
				auto& desc = descriptor().data;

				if (index == 0)
					if (desc.data_0_ptr == 0)
						desc.data_0_ptr = mem_inst->allocateSingleBlock<8>();

				if (index < Size0)
					return;

				index -= Size0;
				if (index == 0)
					if (desc.data_1_ptr == 0)
						desc.data_1_ptr = mem_inst->allocateSingleBlock<8>();
				if (index < Size1) {
					auto& block = mem_inst->getPtrBlock(desc.data_1_ptr);
					allocateInPlace(block, index);
					return;
				}

				index -= Size1;
				if (index == 0)
					if (desc.data_2_ptr == 0)
						desc.data_2_ptr = mem_inst->allocateSingleBlock<8>();

				if (index < Size2)  {
					auto& block1 = mem_inst->getPtrBlock(desc.data_2_ptr);
					auto& block2 = mem_inst->getPtrBlock(allocateInPlace(block1, index / Size1));
					allocateInPlace(block2, index % Size1);
					return;
				}

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

			bool freeInPlace(PtrBlockType& block, num_t index, num_t dp) {
				num_t pow = CPower(PtrBlockType::Size, dp);
				bool a = true;
				if (dp > 0)
					a = freeInPlace(mem_inst->getPtrBlock(block.ptrs[index / pow]), index % pow, dp - 1);
				index /= pow;
				if (a || dp == 0) {
					mem_inst->freeSingleBlock(block.ptrs[index]);
					block.ptrs[index] = 0;
				}
				return index == 0 && a;
			}

			void freeBlock(num_t index) {
				auto& desc = descriptor();

				if (index == 0) {
					mem_inst->freeSingleBlock(desc.data.data_0_ptr);
					desc.data.data_0_ptr = 0;
					return;
				}

				index -= Size0;
				if (index < Size1) {
					freeInPlace(mem_inst->getPtrBlock(desc.data.data_1_ptr), index, 0);
					if (index == 0) {
						mem_inst->freeSingleBlock(desc.data.data_1_ptr);
						desc.data.data_1_ptr = 0;
					}
					return;
				}

				index -= Size1;
				if (index < Size2) {
					freeInPlace(mem_inst->getPtrBlock(desc.data.data_2_ptr), index, 1);
					if (index == 0) {
						mem_inst->freeSingleBlock(desc.data.data_2_ptr);
						desc.data.data_2_ptr = 0;
					}
					return;
				}

				index -= Size2;
				if (index < Size3) {
					freeInPlace(mem_inst->getPtrBlock(desc.data.data_3_ptr), index, 2);
					if (index == 0) {
						mem_inst->freeSingleBlock(desc.data.data_3_ptr);
						desc.data.data_3_ptr = 0;
					}
					return;
				}

				throw new std::out_of_range("free unindexed block");
			}

			void allocate(num_t amount) {
				auto block_max = BlockCount;
				auto& desc = descriptor();
				while (amount-- > 0) {
					num_t allocated_blocks = (desc.header.size + BlockSize - 1) / BlockSize;
					allocateBlock(allocated_blocks);
					desc.header.size += BlockSize;
				}
			}

			void deallocate(num_t amount) {
				auto block_max = BlockCount;
				auto& desc = descriptor();
				while (amount-- > 0) {
					num_t allocated_blocks = (desc.header.size + BlockSize - 1) / BlockSize;
					freeBlock(allocated_blocks - 1);
					desc.header.size -= BlockSize;
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

			void setName(std::string name, bool except_on_oversize = true);

			void deletFile() {
				descriptor().attributes.flags &= !descriptor().attributes.EX;

				num_t allocated_blocks = (descriptor().header.size + BlockSize - 1) / BlockSize;
				deallocate(allocated_blocks);
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
				if (required_block < allocated_blocks)
					deallocate(allocated_blocks - required_block);
			}
			BlockType& getBlock(num_t index) {
				return mem_inst->getBlock(mem_inst->getIndexedPtr(this->index, index));
			}
			num_t getAllocatedBlockCount() {
				return (descriptor().header.size + BlockSize - 1) / BlockSize;
			}

			MemoryInstance* instance() {
				return mem_inst;
			}

			static FileReference fileAt(num_t index, MemoryInstance* src) {
				FileReference res{};
				res.index = index;
				res.mem_inst = src;
				return res;
			}

			template<typename Type>
			struct DataIterator {
			private:
				FileReference* ref_ptr_;
				num_t index_;
				num_t offest_;

			public:
				DataIterator(FileReference* file, num_t index = 0, num_t byte_offest = 0) : ref_ptr_(file), index_(index), offest_(byte_offest) {}

				void set(const Type& a) {
					const auto& data = reinterpret_cast<const std::array<byte_t, sizeof(Type)>&>(a);
					num_t size = sizeof(Type);
					num_t index = 0;
					num_t offest = offest_ + index_ * sizeof(Type);
					num_t bindex = offest % BlockSize;

					BlockType* block;

					while (index < size) {
						block = getBlock((offest + index) / BlockSize);
						for (; bindex < BlockSize && index < size; bindex++, index++)
							block.data[bindex] = data[index];
						bindex = 0;
					}
				}

				Type operator*() {
					std::array<byte_t, sizeof(Type)> data;
					num_t size = sizeof(Type);
					num_t index = 0;
					num_t offest = offest_ + index_ * sizeof(Type);
					num_t bindex = offest % BlockSize;

					BlockType* block;

					while (index < size) {
						block = getBlock((offest + index) / BlockSize);
						for (; bindex < BlockSize && index < size; bindex++, index++)
							data[index] = block.data[bindex];
						bindex = 0;
					}

					return reinterpret_cast<Type>(data);
				}

				DataIterator& operator++() {
					index_++;
					return *this;
				}

				bool operator!=(const DataIterator& other) {
					return index_ * sizeof(Type) + offest_ != other.index_ * sizeof(Type) + other.offest_;
				}
			};
		};

		struct API;
	};

	template<num_t BlockSize, num_t SuperBlockSize, num_t SuperBlockCount, num_t DescriptorCount>
	struct MemoryInstance<BlockSize, SuperBlockSize, SuperBlockCount, DescriptorCount>::API {
		static std::vector<FileReference> ListFiles(MemoryInstance* inst) {
			std::vector<FileReference> res;
			for (num_t i = 0; i < DescriptorCount; i++) {
				auto& desc = inst->getDescriptor(i);
				if (desc.attributes.flags & desc.attributes.EX)
					res.push_back(FileReference::fileAt(i, inst));
			}
			return res;
		}
		static std::vector<FileReference> OpenDirectory(FileReference ref) {
			auto& desc = ref.descriptor();
			if (!(desc.attributes.flags & desc.attributes.DR))
				return {};

			std::vector<FileReference> files;

			for (num_t i = 0; i < ref.getAllocatedBlockCount(); i++) {
				BlockType& block = ref.getBlock(i);
				PtrBlockType& pb = reinterpret_cast<PtrBlockType&>(block);
				for (num_t i = 0; i < pb.Size && pb.ptrs[i] != 0; i++)
					files.push_back(FileReference::fileAt(pb.ptrs[i], ref.instance()));
			}

			return files;
		}
	};
}