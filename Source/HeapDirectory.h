/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#ifndef NIRVANA_CORE_HEAPDIRECTORY_H_
#define NIRVANA_CORE_HEAPDIRECTORY_H_
#pragma once

#include "BitmapOps.h"
#include <Port/Memory.h>

/** \file
The heap implementation.

We use the memory allocation algorithm with binary quantization of the block sizes.
The heap is considered as the sequence of blocks without the gaps between ones.
Each block has size equal a power of 2 and address aligned with it's size.
Each block is allocated or free. When we need to allocate a block with size S,
we search for the free block of size HEAP_UNIT*(2^N) >= S.
The HEAP_UNIT is the minimal block size and always equal a power of 2.
If HEAP_UNIT*(2^N) - S >= HEAP_UNIT, the remaining space divided into smaller blocks and freed for the
further reuse.

So, the allocated memory chunk is always aligned to the nearest power of 2.
All hardware memory management sizes usually equal a power of 2. Thus we guarantee that
if the allocated memory chunk is greater than the half of hardware page size, it will be
aligned at the page boundary. The chunk with the size of half of the cache line will be
aligned with the cache line etc. This provides allocated chunks' 
optimal alignment and performance for any hardware.

HEAP_LEVELS - is a number of different block sizes. The maximal size of the allocated block is:
MAX_BLOCK_SIZE = HEAP_UNIT << (HEAP_LEVELS - 1).

The information about the free block is stored in bitmap.
Bitmap looks like a "pyramid" with a number of levels. Each level corresponds to the block size.
If the block is free, the bit at the corresponding level is set.
Also, we have an array of HEAP_LEVELS integers that stores a number of free blocks at each level.

        +--------+
Level 0 |01111111|          Free blocks count: 7
        +--------+
        +--------+--------+
Level 1 |01000000|00000000| Free blocks count: 1
        +--------+--------+
*/

namespace Nirvana {
namespace Core {

class HeapDirectoryBase
{
	// Copy prohibited.
	HeapDirectoryBase (const HeapDirectoryBase&);
	HeapDirectoryBase operator = (const HeapDirectoryBase&);

public:
	HeapDirectoryBase () {}

protected:
	static size_t commit_unit (const void* p)
	{
		if (Port::Memory::FIXED_COMMIT_UNIT)
			return Port::Memory::FIXED_COMMIT_UNIT;
		else
			return Port::Memory::query (p, Memory::QueryParam::COMMIT_UNIT);
	}

	typedef BitmapOps::BitmapWord BitmapWord;

	struct BitmapIndex
	{
		unsigned level;
		size_t bitmap_offset;
	};

};

/**
Heap directory - the heap control block.
Heap directory contains the free blocks bitmap and the free blocks count array.
\tparam DIRECTORY_SIZE Size of the heap control block (heap directory).
	Must be multiple of PROTECTION_UNIT. The smaller size reduces the memory overhead.
	Because the maximum block size is smaller than the heap size, the bitmap looks like a pyramid
	with a sliced top. This provides a small amount of unused space at top.
	The free blocks count array is placed at this space before the bitmap.
	Reducing the DIRECTORY_SIZE reduces also the size of space for the free blocks count array.
	This can cause not enough space for the HEAP_LEVELS size array.
	In this case we use one counter for several top levels.
	This can increase the time of the free block search.
	There are 3 implementations of HeapDirectoryTraits for directory sizes 0x10000, 0x8000 and 0x4000
	respectively.
	Smaller or larger sizes are hardly ever provide the better result.

\tparam _HEAP_LEVELS Number of the heap levels.
	Defines the maximal allocation block size: MAX_BLOCK_SIZE = HEAP_UNIT << (HEAP_LEVELS - 1).
	The larger blocks are allocated directly by the base memory service.
	The base memory service overhead due to size alignment is about a half of PROTECTION_UNIT
	(hardware page size).
	Therefore, MAX_BLOCK_SIZE should be significantly larger than the page size, and not much less
	than ALLOCATION_UNIT and SHARING_UNIT.
	Larger blocks are rare enough. Thus, allocating large blocks directly through the base service
	can be considered justified.

For Windows host system ALLOCATION_UNIT = 64K, DIRECTORY_SIZE = 64K, and HEAP_LEVELS = 11.
MAX_BLOCK_SIZE = HEAP_UNIT_DEFAULT * 1024.
HEAP_UNIT_DEFAULT = 16 for 32-bit, 32 for 64-bit systems.
So MAX_BLOCK_SIZE is 16K for 32-bit, 32K for 64-bit.

In systems with smaller system ALLOCATION_UNIT can be used DIRECTORY_SIZE = 0x8000 or DIRECTORY_SIZE = 0x4000.

16-bit systems use one global heap with DIRECTORY_SIZE 0x400 or 0x800 (not yet implemented).
*/
template <size_t DIRECTORY_SIZE, unsigned _HEAP_LEVELS>
class HeapDirectoryTraitsBase :
	public HeapDirectoryBase
{
public:
	static const size_t UNIT_COUNT = DIRECTORY_SIZE * 4;
	static const size_t MAX_BLOCK_SIZE = 1 << (_HEAP_LEVELS - 1);

protected:
	static const unsigned HEAP_LEVELS = _HEAP_LEVELS;

	// Number of top level blocks.
	static const size_t TOP_LEVEL_BLOCKS = UNIT_COUNT >> (HEAP_LEVELS - 1);

	// The top bitmap level size in words.
	static const size_t TOP_BITMAP_WORDS = TOP_LEVEL_BLOCKS / (sizeof (BitmapWord) * 8);

	// size_t of bitmap (in words).
	static const size_t BITMAP_SIZE = (~((~(size_t)0) << HEAP_LEVELS)) * TOP_BITMAP_WORDS;

	// Space available before bitmap for free block index.
	static const size_t FREE_BLOCK_INDEX_MAX = (DIRECTORY_SIZE - BITMAP_SIZE * sizeof (BitmapWord)) / sizeof (uint16_t);
};

// There are 3 implementations of HeapDirectoryTraits for directory sizes 0x10000, 0x8000 and 0x4000 respectively.
template <size_t SIZE, unsigned HEAP_LEVELS> class HeapDirectoryTraits;

template <>
class HeapDirectoryTraits <0x10000, 11> :
	public HeapDirectoryTraitsBase <0x10000, 11>
{
public:
	static const size_t FREE_BLOCK_INDEX_SIZE = 15;

	static const bool merged_levels = false;

	// Get search start offset in free_block_count_ from the block size.
	static const size_t block_index_offset_ [HEAP_LEVELS];

	// Get bitmap level and offset from the backward offset from the end of free_block_count_.
	static const BitmapIndex bitmap_index_ [FREE_BLOCK_INDEX_SIZE];
};

template <>
class HeapDirectoryTraits <0x8000, 10> :
	public HeapDirectoryTraitsBase <0x8000, 10>
{
public:
	static const size_t FREE_BLOCK_INDEX_SIZE = 11;

	static const bool merged_levels = false;

	// Get search start offset in free_block_count_ from the block size.
	static const size_t block_index_offset_ [HEAP_LEVELS];

	// Get bitmap level and offset from the backward offset from the end of free_block_count_.
	static const BitmapIndex bitmap_index_ [FREE_BLOCK_INDEX_SIZE];
};

template <>
class HeapDirectoryTraits <0x8000, 11> :
	public HeapDirectoryTraitsBase <0x8000, 11>
{
public:
	static const size_t FREE_BLOCK_INDEX_SIZE = 8;

	static const bool merged_levels = true;

	// Get search start offset in free_block_count_ from the block size.
	static const size_t block_index_offset_ [HEAP_LEVELS];

	// Get bitmap level and offset from the backward offset from the end of free_block_count_.
	static const BitmapIndex bitmap_index_ [FREE_BLOCK_INDEX_SIZE];
};

template <>
class HeapDirectoryTraits <0x4000, 9> :
	public HeapDirectoryTraitsBase <0x4000, 9>
{
public:
	static const size_t FREE_BLOCK_INDEX_SIZE = 9;

	static const bool merged_levels = false;

	// Get search start offset in free_block_count_ from the block size.
	static const size_t block_index_offset_ [HEAP_LEVELS];

	// Get bitmap level and offset from the backward offset from the end of free_block_count_.
	static const BitmapIndex bitmap_index_ [FREE_BLOCK_INDEX_SIZE];
};

template <>
class HeapDirectoryTraits <0x4000, 11> :
	public HeapDirectoryTraitsBase <0x4000, 11>
{
public:
	static const size_t FREE_BLOCK_INDEX_SIZE = 4;

	static const bool merged_levels = true;

	// Get search start offset in free_block_count_ from the block size.
	static const size_t block_index_offset_ [HEAP_LEVELS];

	// Get bitmap level and offset from the backward offset from the end of free_block_count_.
	static const BitmapIndex bitmap_index_ [FREE_BLOCK_INDEX_SIZE];
};

/// \brief Information about the heap space controlled by the HeapDirectory.
/// 
/// Used as optional parameter for commit/decommit allocated memory.
/// Must be NULL if heap allocation/deallocation mustn't cause commit/decommit.
struct HeapInfo
{
	void* heap;         ///< Pointer to heap space.
	size_t unit_size;   ///< Allocation unit size.
	size_t commit_size; ///< Commit unit size.
};

/// \brief Heap directory bitmap implementation details
enum class HeapDirectoryImpl
{
	/// This implementation used for systems without memory protection.
	/// All bitmap memory must be initially committed and zero-filled.
	/// `HeapInfo` parameter is unused and must be `NULL`. `Memory` functions never called.
	PLAIN_MEMORY,

	/// All bitmap memory must be initially committed and zero-filled.
	/// If `HeapInfo` pointer is `NULL` the `Memory` functions will never be called as for PLAIN_MEMORY.
	COMMITTED_BITMAP
};

/// \brief Heap directory. Used for memory allocation on different levels of memory management.
/// 
/// Heap directory allocates and deallocates abstract "units" in range (0 <= n < UNIT_COUNT).
/// Each unit requires 2 bits of HeapDirectory size.
template <size_t DIRECTORY_SIZE, unsigned HEAP_LEVELS, HeapDirectoryImpl IMPL>
class HeapDirectory :
	public HeapDirectoryTraits <DIRECTORY_SIZE, HEAP_LEVELS>
{
	typedef HeapDirectoryTraits <DIRECTORY_SIZE, HEAP_LEVELS> Traits;
	typedef BitmapOps Ops;
	typedef BitmapOps::BitmapWord BitmapWord;
	static_assert (Traits::FREE_BLOCK_INDEX_SIZE <= Traits::FREE_BLOCK_INDEX_MAX, "Traits::FREE_BLOCK_INDEX_SIZE <= Traits::FREE_BLOCK_INDEX_MAX");
public:
	static const HeapDirectoryImpl IMPLEMENTATION = IMPL;

	/// \brief Initializes heap directory.
	/// 
	/// The bitmap buffer must be reserved for protected implementation or allocated for plain implementation.
	HeapDirectory ()
	{
		if (IMPLEMENTATION == HeapDirectoryImpl::COMMITTED_BITMAP)
			Port::Memory::commit (this, sizeof (*this));
		else if (IMPLEMENTATION != HeapDirectoryImpl::PLAIN_MEMORY) // Commit initial part.
			Port::Memory::commit (this, reinterpret_cast <uint8_t*> (bitmap_ + Traits::TOP_BITMAP_WORDS) - reinterpret_cast <uint8_t*> (this));

		// Initialize
		assert (sizeof (HeapDirectory <DIRECTORY_SIZE, HEAP_LEVELS, IMPL>) <= DIRECTORY_SIZE);

		// Bitmap is always aligned for performance.
		assert ((uintptr_t)(&bitmap_) % sizeof (BitmapWord) == 0);

		// Initialize free blocs count on top level.
		free_block_count_ [Traits::FREE_BLOCK_INDEX_SIZE - 1] = Traits::TOP_LEVEL_BLOCKS;

		// Initialize top level of bitmap by ones.
		::std::fill_n (bitmap_, Traits::TOP_BITMAP_WORDS, ~(BitmapWord)0);
	}

	/// \brief Allocate block.
	/// 
	/// \param size      Block size.
	/// \param heap_info HeapInfo pointer.
	/// 
	/// \returns Block offset in allocation units if succeded, otherwise -1.
	ptrdiff_t allocate (size_t size, const HeapInfo* heap_info = nullptr);

	/// \brief Allocate memory range.
	/// 
	/// \param begin     Start allocation range in units.
	/// \param end       End allocation range.
	/// \param heap_info HeapInfo pointer.
	/// 
	/// \returns `true` if blocks were successfully allocated.
	bool allocate (size_t begin, size_t end, const HeapInfo* heap_info = nullptr);

	/// \brief Check that all units in range are allocated.
	/// 
	/// \param begin Start allocation range in units.
	/// \param end   End allocation range.
	/// 
	/// \returns `true` if range is completely allocated. `false` if some units in range are free.
	bool check_allocated (size_t begin, size_t end) const noexcept;

	/// \brief Release memory range.
	/// 
	/// \param begin Start allocation range in units.
	/// \param end   End allocation range.
	/// \param heap_info HeapInfo pointer.
	/// \param right_to_left 
	void release (size_t begin, size_t end,	const HeapInfo* heap_info = nullptr, bool right_to_left = false);

	/// \brief Test for all blocks are free.
	/// 
	/// \returns `true` if there are no allocated blocks.
	bool empty () const noexcept
	{
		if (Traits::merged_levels) {

			// Top levels are merged
			const BitmapWord* end = bitmap_ + Traits::TOP_BITMAP_WORDS;

			for (const BitmapWord* p = bitmap_; p < end; ++p)
				if (~*p)
					return false;

			return true;

		} else
			return (Traits::TOP_LEVEL_BLOCKS == free_block_count_ [Traits::FREE_BLOCK_INDEX_SIZE - 1]);
	}

private:
	// Number of units per block for level.
	static size_t block_size (size_t level) noexcept
	{
		assert (level < Traits::HEAP_LEVELS);
		return Traits::MAX_BLOCK_SIZE >> level;
	}

	static unsigned level_align (size_t offset, size_t size) noexcept
	{
		// Ищем максимальный размер блока <= size, на который выравнен offset
		unsigned level = Traits::HEAP_LEVELS - 1 - ::std::min (ntz (offset | Traits::MAX_BLOCK_SIZE), ilog2_floor (size));
		assert (level < Traits::HEAP_LEVELS);

#ifdef _DEBUG
		{ // The check code.
			size_t mask = Traits::MAX_BLOCK_SIZE - 1;
			unsigned dlevel = 0;
			while ((mask & offset) || (mask >= size)) {
				mask >>= 1;
				++dlevel;
			}
			assert (dlevel == level);
		}
#endif

		return level;
	}

	static size_t block_number (size_t unit, unsigned level) noexcept
	{
		return unit >> (Traits::HEAP_LEVELS - 1 - level);
	}

	static size_t unit_number (size_t block, unsigned level) noexcept
	{
		return block << (Traits::HEAP_LEVELS - 1 - level);
	}

	static size_t bitmap_offset (unsigned level) noexcept
	{
		return (Traits::TOP_BITMAP_WORDS << level) - Traits::TOP_BITMAP_WORDS;
	}

	static size_t bitmap_offset_next (size_t level_bitmap_offset) noexcept
	{
		assert (level_bitmap_offset < Traits::BITMAP_SIZE - Traits::UNIT_COUNT / (sizeof (BitmapWord) * 8));
		return (level_bitmap_offset << 1) + Traits::TOP_BITMAP_WORDS;
	}

	static size_t bitmap_offset_prev (size_t level_bitmap_offset) noexcept
	{
		assert (level_bitmap_offset >= Traits::TOP_BITMAP_WORDS);
		return (level_bitmap_offset - Traits::TOP_BITMAP_WORDS) >> 1;
	}

	uint16_t& free_block_count (size_t level, size_t block_number) noexcept
	{
		size_t idx = Traits::block_index_offset_ [Traits::HEAP_LEVELS - 1 - level];
		if (DIRECTORY_SIZE > 0x4000) {
			// Add index for splitted levels
			idx += (block_number >> (sizeof (uint16_t) * 8));
		}
		return free_block_count_ [idx];
	}

	static BitmapWord companion_mask (BitmapWord mask) noexcept
	{
		constexpr BitmapWord ODD = sizeof (BitmapWord) > 4 ? 0x5555555555555555 : sizeof (BitmapWord) > 2 ? 0x55555555 : 0x5555;
		return mask | ((mask >> 1) & ODD) | ((mask << 1) & ~ODD);
	}

	//! Commit heap block. Does nothing if heap_info == NULL.
	void commit (size_t begin, size_t end, const HeapInfo* heap_info);

private:
	// Free block count index.
	// 
	// This array contains the number of free blocks at each level.
	// If the total block count at a level is > 64K, the level is split into several parts.
	// Each part has the corresponding array element.
	// Thus, in the worst case, the search is performed in 64K bits bitmap.
	// Note that for the 64K bits bitmap, the count of free blocks is <= 32K because the two adjacent
	// bits can not be set - in this case both bits will be reset and one bit at previous level will be set.
	//
	// The array is turned over - the lowest level is first in the array.
	//
	// Because the maximum block size is smaller than the heap size, the bitmap looks like a pyramid with a sliced top.
	// This provides a small amount of unused space at top.
	// The index array is placed at this space before the bitmap.
	uint16_t free_block_count_ [Traits::FREE_BLOCK_INDEX_SIZE];

	// Free blocks bitmap
	BitmapWord bitmap_ [Traits::BITMAP_SIZE];
};

template <size_t DIRECTORY_SIZE, unsigned HEAP_LEVELS, HeapDirectoryImpl IMPL>
ptrdiff_t HeapDirectory <DIRECTORY_SIZE, HEAP_LEVELS, IMPL>::allocate (size_t size, const HeapInfo* heap_info)
{
	assert (size);
	assert (size <= Traits::MAX_BLOCK_SIZE);

	// Quantize block size and calculate the maximal block level
	unsigned level = Traits::HEAP_LEVELS - ilog2_ceil(size) - 1;
	assert (level < Traits::HEAP_LEVELS);
	size_t block_index_offset = Traits::block_index_offset_ [Traits::HEAP_LEVELS - 1 - level];

	typename Traits::BitmapIndex bi;
	BitmapWord* bitmap_ptr;
	int bit_number;

	for (;;) {
		// Search in free block index
		uint16_t* free_blocks_ptr = free_block_count_ + block_index_offset;
		ptrdiff_t cnt = Traits::FREE_BLOCK_INDEX_SIZE - block_index_offset;
		while ((cnt--) && !Ops::acquire (free_blocks_ptr))
			++free_blocks_ptr;
		if (cnt < 0)
			return -1; // no such blocks

		// Decide where to search in the bitmap (level and offset)
		bi = Traits::bitmap_index_ [cnt];

		if (
			Traits::merged_levels // Top levels are merged
			&&
			!cnt
			) { // Top levels

			unsigned merged_levels = bi.level;

			// Refine the search start offset by the block level
			if (bi.level > level) {
				bi.level = level;
				bi.bitmap_offset = bitmap_offset (level);
			}

			BitmapWord* end = bitmap_ + bitmap_offset_next (bi.bitmap_offset);
			BitmapWord* begin = bitmap_ptr = bitmap_ + bi.bitmap_offset;

			// Search in the bitmap
			while ((bit_number = Ops::clear_rightmost_one (bitmap_ptr)) < 0) {
				if (++bitmap_ptr >= end) {

					if (!bi.level) {
						// The free block with required size is not found
						if (level < merged_levels) {
							Ops::release (free_blocks_ptr);
							return -1;
						} else
							goto tryagain;
					}

					// Go level up
					--bi.level;
					end = begin;
					begin = bitmap_ptr = bitmap_ + (bi.bitmap_offset = bitmap_offset_prev (bi.bitmap_offset));
				}
			}

		} else {
			// Levels not merged: find as usual.
			BitmapWord* begin = bitmap_ptr = bitmap_ + bi.bitmap_offset;
			BitmapWord* end = begin + ::std::min ((size_t)Traits::TOP_LEVEL_BLOCKS << bi.level, (size_t)0x10000) / (sizeof (BitmapWord) * 8);

			while ((bit_number = Ops::clear_rightmost_one (bitmap_ptr)) < 0)
				if (++bitmap_ptr == end)
					goto tryagain;

		}

		break;
	tryagain:
		Ops::release (free_blocks_ptr);
	}

	assert (bit_number >= 0);

	// Define the block number by the bitmap offset and level
	size_t level_bitmap_begin = bitmap_offset (bi.level);
	assert ((bitmap_ptr - bitmap_) >= (ptrdiff_t)level_bitmap_begin);
	size_t block_number = (bitmap_ptr - bitmap_ - level_bitmap_begin) * sizeof (BitmapWord) * 8 + bit_number;

	// Define the block start and end offsets
	size_t allocated_size = block_size (bi.level);
	size_t block_offset = block_number * allocated_size;
	size_t allocated_end = block_offset + allocated_size;
	assert (allocated_end <= Traits::UNIT_COUNT);

	// Ensure that memory is committed and writeable.
	commit (block_offset, allocated_end, heap_info);

	// Te block of size = allocated_size (power of 2) is allocated.
	// We need the block of size = size. Release the remaining part.
	try {
		release (block_offset + size, allocated_end);
	} catch (...) {
		// On exception we release size bytes, not allocated_size bytes!
		// If the memory was partially released and there was a failure
		// (it is not possible to commit bitmap, for example), we release the block ending to block_offset + size.
		// It is also the start of the succesfully released part. The free block bits in this part are set as the
		// companions of the releasing block bits.
		// As the result, the source state of the bitmap will be restored.
		release (block_offset, block_offset + size, heap_info);
		throw;
	}

	return block_offset;
}

template <size_t DIRECTORY_SIZE, unsigned HEAP_LEVELS, HeapDirectoryImpl IMPL>
bool HeapDirectory <DIRECTORY_SIZE, HEAP_LEVELS, IMPL>::allocate (size_t begin, size_t end, const HeapInfo* heap_info)
{
	assert (begin < end);
	if (end > Traits::UNIT_COUNT)
		return false;

	// Занимаем блок, разбивая его на блоки размером 2^n, смещение которых кратно их размеру.
	// Allocate block, breaking it into blocks of size 2^n, the offset of which is multiples of their size.
	size_t allocated_begin = begin;         // Begin of the allocated space
	size_t allocated_end = allocated_begin; // End of the allocated space
	while (allocated_end < end) {

		// Search for the minimum level on which offset and size are aligned
		unsigned level = level_align (allocated_end, end - allocated_end);

		// Get begin of the level bitmap and block number within it
		size_t level_bitmap_begin = bitmap_offset (level);
		size_t bl_number = block_number (allocated_end, level);

		bool success = false;
		BitmapWord* bitmap_ptr;
		BitmapWord mask;
		for (;;) {

			bitmap_ptr = bitmap_ + level_bitmap_begin + bl_number / (sizeof (BitmapWord) * 8);
			mask = (BitmapWord)1 << (bl_number % (sizeof (BitmapWord) * 8));
			volatile uint16_t& free_blocks_cnt = free_block_count (level, bl_number);
			// Decrement free blocks counter.
			if (Ops::acquire (&free_blocks_cnt)) {
				if ((success = Ops::bit_clear (bitmap_ptr, mask)))
					break;
				Ops::release (&free_blocks_cnt);
			}

			if (!level)
				break; // Unsuccessfull. Range is not free.

			// Go level up
			--level;
			bl_number = bl_number >> 1;
			level_bitmap_begin = bitmap_offset_prev (level_bitmap_begin);
		}

		if (!success) {
			// Block is not free. Release allocated blocks and return false.
			release (allocated_begin, allocated_end);
			return false;
		}

		size_t block_offset = unit_number (bl_number, level);
		if (allocated_begin > block_offset)
			allocated_begin = block_offset;
		allocated_end = block_offset + block_size (level);
	}

	assert (allocated_begin <= begin && end <= allocated_end);
	assert (allocated_end <= Traits::UNIT_COUNT);

	// Ensure that memory is committed and writeable.
	commit (allocated_begin, allocated_end, heap_info);

	try {
		// Release extra space at begin and end
		// Memory is released from the inside-out so that in case of failure
		// (it is impossible to fix the bitmap) and subsequent release of the internal part,
		// the original state has been restored.
		release (allocated_begin, begin, 0, true);
		release (end, allocated_end, 0, false);
	} catch (...) {
		release (begin, end, heap_info);
		throw;
	}

	return true;
}

template <size_t DIRECTORY_SIZE, unsigned HEAP_LEVELS, HeapDirectoryImpl IMPL> inline
void HeapDirectory <DIRECTORY_SIZE, HEAP_LEVELS, IMPL>::commit (size_t begin, size_t end, const HeapInfo* heap_info)
{
	if (IMPL != HeapDirectoryImpl::PLAIN_MEMORY && heap_info) {
		assert (heap_info->heap && heap_info->unit_size && heap_info->commit_size);
		size_t commit_begin = begin * heap_info->unit_size;
		size_t commit_size = end * heap_info->unit_size - commit_begin;
		try {
			Port::Memory::commit ((uint8_t*)(heap_info->heap) + commit_begin, commit_size);
		} catch (...) {
			release (begin, end);
			throw;
		}
	}
}

template <size_t DIRECTORY_SIZE, unsigned HEAP_LEVELS, HeapDirectoryImpl IMPL>
void HeapDirectory <DIRECTORY_SIZE, HEAP_LEVELS, IMPL>::release (size_t begin, size_t end, const HeapInfo* heap_info, bool rtl)
{
	assert (begin <= end);
	assert (end <= Traits::UNIT_COUNT);

	// Decommit blocks at this level. Note: decommit_level can be negative if commit_size greater than maximal block size.
	int decommit_level = Traits::HEAP_LEVELS;
	if (IMPL != HeapDirectoryImpl::PLAIN_MEMORY && heap_info) {
		assert (heap_info->heap && heap_info->unit_size && heap_info->commit_size);
		decommit_level = Traits::HEAP_LEVELS - 1 - ilog2_floor (heap_info->commit_size / heap_info->unit_size);
	}

	// The released area must be split into blocks of size 2^n, the offset of which is multiples of their size.
	while (begin < end) {

		// The block offset in heap
		unsigned level;
		size_t block_begin;
		size_t block_end;
		if (rtl) {
			level = level_align (end, end - begin);
			block_begin = end - block_size (level);
			block_end = end;
		} else {
			level = level_align (begin, end - begin);
			block_begin = begin;
			block_end = block_begin + block_size (level);
		}

		// Release block with offset block_begin at level

		// Get begin of the level bitmap and block number within it
		size_t level_bitmap_begin = bitmap_offset (level);
		size_t bl_number = block_number (block_begin, level);

		// Calculate the address of the bitmap word
		BitmapWord* bitmap_ptr = bitmap_ + level_bitmap_begin + bl_number / (sizeof (BitmapWord) * 8);
		BitmapWord mask = (BitmapWord)1 << (bl_number % (sizeof (BitmapWord) * 8));
		volatile uint16_t* free_blocks_cnt = &free_block_count (level, bl_number);

		while (level > 0) {

			// Decommit freed memory.
			if (IMPL != HeapDirectoryImpl::PLAIN_MEMORY && heap_info && level == decommit_level)
				Port::Memory::decommit ((uint8_t*)(heap_info->heap) + bl_number * heap_info->commit_size, heap_info->commit_size);

			if (Ops::bit_set_check_companion (bitmap_ptr, mask, companion_mask (mask))) {
				// There is free companion - merge it with the released block
				// Go level up
				Ops::decrement (free_blocks_cnt);
				--level;
				level_bitmap_begin = bitmap_offset_prev (level_bitmap_begin);
				bl_number >>= 1;
				mask = (BitmapWord)1 << (bl_number % (sizeof (BitmapWord) * 8));
				bitmap_ptr = bitmap_ + level_bitmap_begin + bl_number / (sizeof (BitmapWord) * 8);
				free_blocks_cnt = &free_block_count (level, bl_number);
			} else
				break;
		}

		// Increase the counter of the free blocks in advance so that other threads know that
		// the free block will soon appear and start searching.
		// This should reduce the likelihood of false negatives. Although, it can cause additional search loops.
		Ops::release (free_blocks_cnt);

		if (level == 0) {
			if (IMPL != HeapDirectoryImpl::PLAIN_MEMORY && decommit_level <= 0) {
				if (decommit_level == 0) {
					if (heap_info)
						Port::Memory::decommit ((uint8_t*)(heap_info->heap) + bl_number * heap_info->commit_size, heap_info->commit_size);
					if (!Ops::bit_set (bitmap_ptr, mask))
						throw_FREE_MEM ();
				} else {
					unsigned block_size = (unsigned)1 << (unsigned)(-decommit_level);
					BitmapWord companion_mask = (~((~(BitmapWord)0) << block_size)) << round_down ((unsigned)(bl_number % (sizeof (BitmapWord) * 8)), block_size);
					if (Ops::bit_set_check_companion (bitmap_ptr, mask, companion_mask)) {
						if (heap_info)
							try {
								Port::Memory::decommit ((uint8_t*)(heap_info->heap) + (bl_number >> (unsigned)(-decommit_level)) * heap_info->commit_size, heap_info->commit_size);
							} catch (...) {
							}
						if (!Ops::bit_set (bitmap_ptr, companion_mask))
							throw_FREE_MEM ();
					}
				}
			} else
				if (!Ops::bit_set (bitmap_ptr, mask))
					throw_FREE_MEM ();
		}

		// The block is released
		if (rtl)
			end = block_begin;
		else
			begin = block_end;
	}
}

template <size_t DIRECTORY_SIZE, unsigned HEAP_LEVELS, HeapDirectoryImpl IMPL>
bool HeapDirectory <DIRECTORY_SIZE, HEAP_LEVELS, IMPL>::check_allocated (size_t begin, size_t end) const noexcept
{
	if (begin >= Traits::UNIT_COUNT || end > Traits::UNIT_COUNT || end <= begin)
		return false;

	// Check for all bits on all levels are 0
	int level = Traits::HEAP_LEVELS - 1;
	size_t level_bitmap_begin = bitmap_offset (Traits::HEAP_LEVELS - 1);
	for (;;) {

		assert (begin < end);

		const BitmapWord* begin_ptr = bitmap_ + level_bitmap_begin + begin / (sizeof (BitmapWord) * 8);
		BitmapWord begin_mask = (~(BitmapWord)0) << (begin % (sizeof (BitmapWord) * 8));
		const BitmapWord* end_ptr = bitmap_ + level_bitmap_begin + (end - 1) / (sizeof (BitmapWord) * 8);
		BitmapWord end_mask = (~(BitmapWord)0) >> ((sizeof (BitmapWord) * 8) - (((end - 1) % (sizeof (BitmapWord) * 8)) + 1));
		assert (end_ptr < bitmap_ + Traits::BITMAP_SIZE);
		assert (end_mask);
		assert (begin_ptr <= end_ptr);
		assert (begin_mask);

		if (begin_ptr >= end_ptr) {

			if (*begin_ptr & begin_mask & end_mask)
				return false;

		} else {

			if (*begin_ptr & begin_mask)
				return false;

			while (++begin_ptr < end_ptr) {
				if (*begin_ptr)
					return false;
			}

			if (*end_ptr & end_mask)
				return false;
		}

		if (level > 0) {
			// Go to up level
			--level;
			level_bitmap_begin = bitmap_offset_prev (level_bitmap_begin);
			begin /= 2;
			end = (end + 1) / 2;
		} else
			break;
	}

	return true;
}

}
}

#endif
