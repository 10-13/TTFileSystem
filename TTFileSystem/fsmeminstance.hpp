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

		constexpr const static num_t TotalSize = sizeof(Primitives::Header) + DescriptorCount * sizeof(Primitives::Descriptor) + SuperBlockCount * sizeof(SuperBlockType);
		constexpr const static num_t BlockCount = SuperBlockSize * SuperBlockCount;
		
		constexpr const static num_t SuperBlocksOffset = sizeof(Primitives::Header) + DescriptorCount * sizeof(Primitives::Descriptor);
		constexpr const static num_t BlockOffset = offsetof(SuperBlockType, data);
		constexpr const static num_t DescriptorsOffset = sizeof(Primitives::Header);
	private:
		byte_t* data_;

		template<typename T>
		T* getOffsetedPtr(num_t offset, num_t index)
		{
			return *(T*)(data_ + offset + index * sizeof(T));
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

		BlockType& getBlock(num_t global_index)
		{
			if (global_index >= BlockCount)
				throw new std::out_of_range("Accessing non instance data.");
			return *getOffsetedPtr<BlockType>(SuperBlocksOffset + sizeof(SuperBlockType) * (global_index / SuperBlockSize) + BlockOffset, global_index % SuperBlockSize);
		}

		byte_t* transfer()
		{
			auto tmp = data_;
			data_ = nullptr;
			return tmp_;
		}

	public:
		MemoryInstance()
		{
			data_ = (byte_t*)malloc(TotalSize);
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
	};
}