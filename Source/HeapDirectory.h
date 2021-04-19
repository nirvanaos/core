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

#include <Nirvana/Nirvana.h>
#include <Port/Memory.h>
#include <stddef.h>
#include <algorithm>
#include <atomic>

#ifdef _WIN32

#define HEAP_DIR_TRY __try
#define HEAP_DIR_CATCH __except (1) // EXCEPTION_EXECUTE_HANDLER

#else

// For POSIX we can implement GP handling based on signals and longjmp
#define HEAP_DIR_TRY try
#define HEAP_DIR_CATCH catch (...)

#endif

/*
Используется алгоритм распределения памяти с двоичным квантованием размеров блоков.
Куча рассматривается, как совокупность блоков размером, кратным степени 2,
расположенных по адресам, кратным размеру блока. При выделении блока размером S
ищется свободный блок размером m << n >= S, где m - минимальный размер блока.
Если запрошенный размер блока меньше, оставшееся в блоке свободное пространство
разделяется на несколько меньших блоков, размеры которых также кратны степени 2.
Поскольку все аппаратные параметры управления памятью обычно кратны степени 2,
это гарантирует, что блок, по размеру кратный странице, будет выравнен на границу
страницы, блок, кратный размеру линейки кэша, будет выравнен на ее границу и т. п.
Таким образом, обеспечивается оптимальное выравнивание выделяемых блоков для любой
архитектуры и максимальная производительность.

Информация о свободных блоках хранится в битовой карте. Битовая карта представляет
собой "пирамиду", в которой каждому блоку каждого уровня (размера) соответствует
один бит. Этот бит установлен, если блок свободен. Так как максимальный размер блока
меньше размера кучи, битовая карта выглядит, как пирамида со срезанной верхушкой.
На месте верхушки расподожен индексный массив, uint16_t, содержащий информацию о
количестве свободных блоков на различных уровнях.
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

	struct BitmapIndex
	{
		UWord  level;
		size_t bitmap_offset;
	};

};

/*
DIRECTORY_SIZE - размер управляющего блока кучи. Должен быть кратен PROTECTION_UNIT.
Управляющий блок содержит битовую карту свободных блоков и массив количества
свободных блоков на уровнях. Меньший размер более экономно расходует память,
выделяемую на управление кучей.
Так как максимальный размер блока меньше размера кучи, битовая карта выглядит,
как пирамида со срезанной верхушкой. На месте верхушки расположен индексный массив,
uint16_t, содержащий информацию о количестве свободных блоков на различных уровнях.
Уменьшение размера управляющего блока кучи уменьшает размер срезанной верхушки
и оставляет меньше места для индекса. При этом приходится отводить один счетчик свободных
блоков на несколько верхних уровней. Это может увеличить время поиска свободного
блока в куче.
Размер управляющего блока принимается равным 16, 32 или 64 К. Меньшие и большие размеры
вряд ли дадут оптимальный результат.
Для 16 битных систем размер управляющего блока единственной общей кучи должен 4 или 2 К (пока не реализовано).

HEAP_LEVELS - количество уровней (размеров блоков) кучи. Определяет максимальный
размер выделяемого блока MAX_BLOCK_SIZE = HEAP_UNIT << (HEAP_LEVELS - 1).
Блоки большего размера выделяются напрямую базовым сервисом памяти.
Накладные расходы из-за выравнивания для базового
сервиса памяти составляют половину PROTECTION_UNIT (аппаратной страницы) на блок.
Поэтому MAX_BLOCK_SIZE должен быть значительно больше размера страницы, и не сильно
меньшим Memory::ALLOCATION_UNIT и Memory::SHARING_UNIT.
Блоки такого большого размера выделяются достаточно редко.
Кроме того, они часто бывают выравнены по размеру страницы (буферы etc.).
Базовый сервис обычно выделяет каждый блок в отдельном SHARING_UNIT, что уменьшает
накладные расходы при копировании.
Таким образом, выделение достаточно больших блоков непосредственно через базовый сервис
можно считать оправданным.
For Windows host Memory::ALLOCATION_UNIT = 64K, DIRECTORY_SIZE = 64K and HEAP_LEVELS = 11.
Это дает MAX_BLOCK_SIZE = HEAP_UNIT_DEFAULT * 1024, равный 16K.
В системах с небольшими величинами PROTECTION_UNIT, ALLOCATION_UNIT и SHARING_UNIT можно
использовать DIRECTORY_SIZE = 0x8000 или DIRECTORY_SIZE = 0x4000.
*/

template <size_t DIRECTORY_SIZE, UWord _HEAP_LEVELS>
class HeapDirectoryTraitsBase :
	public HeapDirectoryBase
{
public:
	static const size_t UNIT_COUNT = DIRECTORY_SIZE * 4;
	static const size_t MAX_BLOCK_SIZE = 1 << (_HEAP_LEVELS - 1);

protected:
	static const UWord HEAP_LEVELS = _HEAP_LEVELS;

	// Number of top level blocks.
	static const size_t TOP_LEVEL_BLOCKS = UNIT_COUNT >> (HEAP_LEVELS - 1);

	// Размер верхнего уровня битовой карты в машинных словах.
	static const size_t TOP_BITMAP_WORDS = TOP_LEVEL_BLOCKS / (sizeof (UWord) * 8);

	// size_t of bitmap (in words).
	static const size_t BITMAP_SIZE = (~((~(size_t)0) << HEAP_LEVELS)) * TOP_BITMAP_WORDS;

	// Space available before bitmap for free block index.
	static const size_t FREE_BLOCK_INDEX_MAX = (DIRECTORY_SIZE - BITMAP_SIZE * sizeof (UWord)) / sizeof (uint16_t);
};

// There are 3 implementations of HeapDirectoryTraits for directory sizes 0x10000, 0x8000 and 0x4000 respectively.
template <size_t SIZE, UWord HEAP_LEVELS> class HeapDirectoryTraits;

template <>
class HeapDirectoryTraits <0x10000, 11> :
	public HeapDirectoryTraitsBase <0x10000, 11>
{
public:
	static const size_t FREE_BLOCK_INDEX_SIZE = 15;

	static const bool merged_levels = false;

	// По размеру блока определает смещение начала поиска в free_block_index_
	static const size_t block_index_offset_ [HEAP_LEVELS];

	// По обратному смещению от конца массива free_block_index_ определяет
	// уровень и положение области битовой карты.
	static const BitmapIndex bitmap_index_ [FREE_BLOCK_INDEX_SIZE];
};

template <>
class HeapDirectoryTraits <0x8000, 10> :
	public HeapDirectoryTraitsBase <0x8000, 10>
{
public:
	static const size_t FREE_BLOCK_INDEX_SIZE = 11;

	static const bool merged_levels = false;

	// По размеру блока определает смещение начала поиска в free_block_index_
	static const size_t block_index_offset_ [HEAP_LEVELS];

	// По обратному смещению от конца массива free_block_index_ определяет
	// уровень и положение области битовой карты.
	static const BitmapIndex bitmap_index_ [FREE_BLOCK_INDEX_SIZE];
};

template <>
class HeapDirectoryTraits <0x8000, 11> :
	public HeapDirectoryTraitsBase <0x8000, 11>
{
public:
	static const size_t FREE_BLOCK_INDEX_SIZE = 8;

	static const bool merged_levels = true;

	// По размеру блока определает смещение начала поиска в free_block_index_
	static const size_t block_index_offset_ [HEAP_LEVELS];

	// По обратному смещению от конца массива free_block_index_ определяет
	// уровень и положение области битовой карты.
	static const BitmapIndex bitmap_index_ [FREE_BLOCK_INDEX_SIZE];
};

template <>
class HeapDirectoryTraits <0x4000, 9> :
	public HeapDirectoryTraitsBase <0x4000, 9>
{
public:
	static const size_t FREE_BLOCK_INDEX_SIZE = 9;

	static const bool merged_levels = false;

	// По размеру блока определает смещение начала поиска в free_block_index_
	static const size_t block_index_offset_ [HEAP_LEVELS];

	// По обратному смещению от конца массива free_block_index_ определяет
	// уровень и положение области битовой карты.
	static const BitmapIndex bitmap_index_ [FREE_BLOCK_INDEX_SIZE];
};

template <>
class HeapDirectoryTraits <0x4000, 11> :
	public HeapDirectoryTraitsBase <0x4000, 11>
{
public:
	static const size_t FREE_BLOCK_INDEX_SIZE = 4;

	static const bool merged_levels = true;

	// По размеру блока определает смещение начала поиска в free_block_index_
	static const size_t block_index_offset_ [HEAP_LEVELS];

	// По обратному смещению от конца массива free_block_index_ определяет
	// уровень и положение области битовой карты.
	static const BitmapIndex bitmap_index_ [FREE_BLOCK_INDEX_SIZE];
};

// Multithreaded implementation of HeapDirectory operations.
struct HeapDirectoryOpsLockFree
{
	// Atomic decrement free blocks counter if it is not zero.
	static bool acquire (volatile uint16_t* pcnt)
	{
		assert (::std::atomic_is_lock_free ((volatile ::std::atomic <uint16_t>*)pcnt));
		uint16_t cnt = ::std::atomic_load ((volatile ::std::atomic <uint16_t>*)pcnt);
		while ((unsigned)(cnt - 1) < (unsigned)0x7FFF) {	// 0 < cnt && cnt <= 0x8000
			if (::std::atomic_compare_exchange_strong ((volatile ::std::atomic <uint16_t>*)pcnt, &cnt, cnt - 1))
				return true;
		}
		return false;
	}

	// Atomic decrement unconditional.
	static void decrement (volatile uint16_t* pcnt)
	{
		::std::atomic_fetch_sub ((volatile ::std::atomic <uint16_t>*)pcnt, 1);
	}

	// Atomic increment free blocks counter.
	static void release (volatile uint16_t* pcnt)
	{
		assert ((int16_t)*pcnt <= 0x7FFF);
		::std::atomic_fetch_add ((volatile ::std::atomic <uint16_t>*)pcnt, 1);
	}

	// Clear rightmost not zero bit and return number of this bit.
	// Return -1 if all bits are zero.
	static Word clear_rightmost_one (volatile UWord* pbits)
	{
		assert (::std::atomic_is_lock_free ((volatile ::std::atomic <UWord>*)pbits));
		UWord bits = ::std::atomic_load ((volatile ::std::atomic <UWord>*)pbits);
		while (bits) {
			UWord rbits = bits;
			if (::std::atomic_compare_exchange_strong ((volatile ::std::atomic <UWord>*)pbits, &bits, rbits & (rbits - 1)))
				return ntz (rbits);
		}
		return -1;
	}

	static bool bit_clear (volatile UWord* pbits, UWord mask)
	{
		UWord bits = ::std::atomic_load ((volatile ::std::atomic <UWord>*)pbits);
		while (bits & mask) {
			UWord rbits = bits & ~mask;
			if (::std::atomic_compare_exchange_strong ((volatile ::std::atomic <UWord>*)pbits, &bits, rbits))
				return true;
		}
		return false;
	}

	static void bit_set (volatile UWord* pbits, UWord mask)
	{
		assert (!(*pbits & mask));
		::std::atomic_fetch_or ((volatile ::std::atomic <UWord>*)pbits, mask);
	}

	static bool bit_set_check_companion (volatile UWord* pbits, UWord mask, UWord companion_mask)
	{
		UWord bits = ::std::atomic_load ((volatile ::std::atomic <UWord>*)pbits);
		assert (!(bits & mask));
		for (;;) {
			if (((bits | mask) & companion_mask) == companion_mask) {
				if (::std::atomic_compare_exchange_strong ((volatile ::std::atomic <UWord>*)pbits, &bits, bits & ~companion_mask))
					return true;
			} else if (::std::atomic_compare_exchange_strong ((volatile ::std::atomic <UWord>*)pbits, &bits, bits | mask))
				return false;
		}
	}
};

//! Information about the heap space controlled by the HeapDirectory.
//! Used as optional parameter for commit/decommit allocated memory.
//! Must be NULL if heap allocation/deallocation mustn't cause commit/decommit.
struct HeapInfo
{
	void* heap;
	size_t unit_size;
	size_t commit_size;
};

/// Heap directory bitmap implementation details
/// 
/// Heap bitmaps are often sparse (filled with zeroes).
/// Some virtual memory systems, such as Linux, use single zeroed physical page for all zero memory
/// blocks and then eventually do COW (copy on write) behavior on the page when we are changing the contents.
/// So the physical page is allocated only when we write to page.
/// For the such systems we may use HeapDirectoryImpl::COMMITTED_BITMAP and it won't produce the redundant
/// memory allocations.
/// But Windows does not use this behaviour. In Windows, the physical page allocated on the first access
/// the page, read or write. So, to conserve the physical memory in Windows we use HeapDirectoryImpl::RESERVED_BITMAP_WITH_EXCEPTIONS.
enum class HeapDirectoryImpl
{
	//! This implementation used for systems without memory protection.
	//! All bitmap memory must be initially committed and zero-filled.
	//! `HeapInfo` parameter is unused and must be `NULL`. `Memory` functions never called.
	PLAIN_MEMORY,

	//! All bitmap memory must be initially committed and zero-filled.
	//! If `HeapInfo` pointer is `NULL` the `Memory` functions will never be called as for PLAIN_MEMORY.
	//! This implementation provides the best performance but can't save physical pages for bitmap.
	COMMITTED_BITMAP,

	//! Bitmap memory must be initially reserved. HeapDirectory will commit it as needed.
	//! HeapDirectory uses `is_readable()` method to detect uncommitted pages.
	//! Extremely slow.
	RESERVED_BITMAP,

	//! Bitmap memory must be initially reserved. HeapDirectory will commit it as needed.
	//! HeapDirectory catches GP exception to detect uncommitted pages.
	//! This provides better performance than `RESERVED_BITMAP`, but maybe not for all platforms.
	RESERVED_BITMAP_WITH_EXCEPTIONS
};

//! Heap directory. Used for memory allocation on different levels of memory management.
//! Heap directory allocates and deallocates abstract "units" in range (0 <= n < UNIT_COUNT).
//! Each unit requires 2 bits of HeapDirectory size.
template <size_t DIRECTORY_SIZE, UWord HEAP_LEVELS, HeapDirectoryImpl IMPL>
class HeapDirectory :
	public HeapDirectoryTraits <DIRECTORY_SIZE, HEAP_LEVELS>
{
	typedef HeapDirectoryTraits <DIRECTORY_SIZE, HEAP_LEVELS> Traits;
	typedef HeapDirectoryOpsLockFree Ops;
	static_assert (Traits::FREE_BLOCK_INDEX_SIZE <= Traits::FREE_BLOCK_INDEX_MAX, "Traits::FREE_BLOCK_INDEX_SIZE <= Traits::FREE_BLOCK_INDEX_MAX");
public:
	static const HeapDirectoryImpl IMPLEMENTATION = IMPL;

	//! \brief Initializes heap directory.
	//! The bitmap buffer must be reserved for protected implementation or allocated for plain implementation.

	HeapDirectory ()
	{
		if (IMPL > HeapDirectoryImpl::COMMITTED_BITMAP) // Commit initial part.
			Port::Memory::commit (this, reinterpret_cast <uint8_t*> (bitmap_ + Traits::TOP_BITMAP_WORDS) - reinterpret_cast <uint8_t*> (this));

		// Initialize
		assert (sizeof (HeapDirectory <DIRECTORY_SIZE, HEAP_LEVELS, IMPL>) <= DIRECTORY_SIZE);

		// Bitmap is always aligned for performance.
		assert ((uintptr_t)(&bitmap_) % sizeof (UWord) == 0);

		// Initialize free blocs count on top level.
		free_block_index_ [Traits::FREE_BLOCK_INDEX_SIZE - 1] = Traits::TOP_LEVEL_BLOCKS;

		// Initialize top level of bitmap by ones.
		::std::fill_n (bitmap_, Traits::TOP_BITMAP_WORDS, ~(UWord)0);
	}

	/// <summary>Allocate block.</summary>
	/// <returns>Block offset in units if succeded, otherwise -1.</returns>
	Word allocate (size_t size, const HeapInfo* heap_info = nullptr);

	bool allocate (size_t begin, size_t end, const HeapInfo* heap_info = nullptr);

	// Checks that all units in range are allocated.
	bool check_allocated (size_t begin, size_t end) const;

	void release (size_t begin, size_t end,	const HeapInfo* heap_info = nullptr, bool right_to_left = false);

	bool empty () const
	{
		if (Traits::merged_levels) {

			// Верхние уровни объединены
			const UWord* end = bitmap_ + Traits::TOP_BITMAP_WORDS;

			for (const UWord* p = bitmap_; p < end; ++p)
				if (~*p)
					return false;

			return true;

		} else
			return (Traits::TOP_LEVEL_BLOCKS == free_block_index_ [Traits::FREE_BLOCK_INDEX_SIZE - 1]);
	}

private:
	// Number of units per block, for level.
	static size_t block_size (size_t level)
	{
		assert (level < Traits::HEAP_LEVELS);
		return Traits::MAX_BLOCK_SIZE >> level;
	}

	static UWord level_align (size_t offset, size_t size)
	{
		// Ищем максимальный размер блока <= size, на который выравнен offset
		size_t level = Traits::HEAP_LEVELS - 1 - ::std::min (ntz (offset | Traits::MAX_BLOCK_SIZE), 31 - nlz ((uint32_t)size));
		assert (level < Traits::HEAP_LEVELS);

#ifdef _DEBUG
		{ // The check code.
			UWord mask = Traits::MAX_BLOCK_SIZE - 1;
			UWord dlevel = 0;
			while ((mask & offset) || (mask >= size)) {
				mask >>= 1;
				++dlevel;
			}
			assert (dlevel == level);
		}
#endif

		return level;
	}

	static size_t block_number (size_t unit, UWord level)
	{
		return unit >> (Traits::HEAP_LEVELS - 1 - level);
	}

	static size_t unit_number (size_t block, UWord level)
	{
		return block << (Traits::HEAP_LEVELS - 1 - level);
	}

	static size_t bitmap_offset (UWord level)
	{
		return (Traits::TOP_BITMAP_WORDS << level) - Traits::TOP_BITMAP_WORDS;
	}

	static size_t bitmap_offset_next (size_t level_bitmap_offset)
	{
		assert (level_bitmap_offset < Traits::BITMAP_SIZE - Traits::UNIT_COUNT / (sizeof (UWord) * 8));
		return (level_bitmap_offset << 1) + Traits::TOP_BITMAP_WORDS;
	}

	static size_t bitmap_offset_prev (size_t level_bitmap_offset)
	{
		assert (level_bitmap_offset >= Traits::TOP_BITMAP_WORDS);
		return (level_bitmap_offset - Traits::TOP_BITMAP_WORDS) >> 1;
	}

	uint16_t& free_block_count (UWord level, size_t block_number)
	{
		size_t idx = Traits::block_index_offset_ [Traits::HEAP_LEVELS - 1 - level];
		if (DIRECTORY_SIZE > 0x4000) {
			// Add index for splitted levels
			idx += (block_number >> (sizeof (uint16_t) * 8));
		}
		return free_block_index_ [idx];
	}

	static UWord companion_mask (UWord mask)
	{
		constexpr UWord ODD = sizeof (UWord) > 4 ? 0x5555555555555555 : 0x55555555;
		return mask | ((mask >> 1) & ODD) | ((mask << 1) & ~ODD);
	}

	//! Commit heap block. Does nothing if heap_info == NULL.
	void commit (size_t begin, size_t end, const HeapInfo* heap_info);

	Word reserved_scan (UWord*& bitmap_ptr, UWord* const end) const
	{
		size_t page_size = HeapDirectoryBase::commit_unit (this);
		assert (page_size);
		Word bit_number = -1;
		for (;;) {
			HEAP_DIR_TRY {
				while ((bit_number = Ops::clear_rightmost_one (bitmap_ptr)) < 0) {
					if (++bitmap_ptr == end)
						break;
				}
				break;
			} HEAP_DIR_CATCH {
				if ((bitmap_ptr = round_up (bitmap_ptr + 1, page_size)) >= end) {
					break;
				}
			}
		}
		return bit_number;
	}

	static bool reserved_bit_clear (UWord* const bitmap_ptr, const UWord mask)
	{
		HEAP_DIR_TRY {
			return Ops::bit_clear (bitmap_ptr, mask);
		} HEAP_DIR_CATCH {
			return false;
		}
	}

	static bool reserved_bit_set_check_companion (UWord* bitmap_ptr, const UWord mask, const UWord companion_mask)
	{
		for (;;) {
			HEAP_DIR_TRY{
				if (Ops::bit_set_check_companion (bitmap_ptr, mask, companion_mask))
					return true;
				else
					break;
			} HEAP_DIR_CATCH{
				Port::Memory::commit (bitmap_ptr, sizeof (UWord));
			}
		}
		return false;
	}

private:
	// Массив, содержащий количество свободных блоков на каждом уровне.
	// Если общее количество блоков на уровне > 64K, он разделяется на части,
	// каждой из которых соответствует один элемент массива.
	// Таким образом, поиск производится, в худшем случае, среди 64K бит или 2K слов.
	// Заметим, что для массива 64К бит, счетчик свободных блоков (бит) не может превышать
	// величину 32К, так как два соседних бита не могут быть одновременно установлены, в этом
	// случае они обнуляются и устанавливается бит на предыдущем уровне.
	// Массив перевернут - нижние уровни идут первыми.
	// Free block count index.
	uint16_t free_block_index_ [Traits::FREE_BLOCK_INDEX_SIZE];

	// Битовая карта свободных блоков. Компилятор должен выравнять на границу UWord.
	UWord bitmap_ [Traits::BITMAP_SIZE];
};

template <size_t DIRECTORY_SIZE, UWord HEAP_LEVELS, HeapDirectoryImpl IMPL>
Word HeapDirectory <DIRECTORY_SIZE, HEAP_LEVELS, IMPL>::allocate (size_t size, const HeapInfo* heap_info)
{
	assert (size);
	assert (size <= Traits::MAX_BLOCK_SIZE);

	// Quantize block size
	UWord level = nlz ((uint32_t)(size - 1)) + Traits::HEAP_LEVELS - 33;
	assert (level < Traits::HEAP_LEVELS);
	size_t block_index_offset = Traits::block_index_offset_ [Traits::HEAP_LEVELS - 1 - level];

	typename Traits::BitmapIndex bi;
	UWord* bitmap_ptr;
	Word bit_number;

	for (;;) {
		// Search in free block index
		uint16_t* free_blocks_ptr = free_block_index_ + block_index_offset;
		ptrdiff_t cnt = Traits::FREE_BLOCK_INDEX_SIZE - block_index_offset;
		while ((cnt--) && !Ops::acquire (free_blocks_ptr))
			++free_blocks_ptr;
		if (cnt < 0)
			return -1; // no such blocks

		// Определяем, где искать
		// Search in bitmap
		bi = Traits::bitmap_index_ [cnt];

		if (
			Traits::merged_levels // Верхние уровни объединены
			&&
			!cnt
			) { // Верхние уровни

			UWord merged_levels = bi.level;

			// По индексу размера блока уточняем уровень и смещение начала поиска
			if (bi.level > level) {
				bi.level = level;
				bi.bitmap_offset = bitmap_offset (level);
			}

			UWord* end = bitmap_ + bitmap_offset_next (bi.bitmap_offset);
			UWord* begin = bitmap_ptr = bitmap_ + bi.bitmap_offset;

			// Поиск в битовой карте. 
			// Верхние уровни всегда находятся в подтвержденной области.
			while ((bit_number = Ops::clear_rightmost_one (bitmap_ptr)) < 0) {
				if (++bitmap_ptr >= end) {

					if (!bi.level) {
						// Блок требуемого размера не найден.
						if (level < merged_levels) {
							Ops::release (free_blocks_ptr);
							return -1;
						} else
							goto tryagain;
					}

					// Поднимаемся на уровень выше
					--bi.level;
					end = begin;
					begin = bitmap_ptr = bitmap_ + (bi.bitmap_offset = bitmap_offset_prev (bi.bitmap_offset));
				}
			}

		} else { // На остальных уровнях ищем, как обычно

			UWord* begin = bitmap_ptr = bitmap_ + bi.bitmap_offset;
			UWord* end = begin + ::std::min (Traits::TOP_LEVEL_BLOCKS << bi.level, (UWord)0x10000) / (sizeof (UWord) * 8);

			if (HeapDirectoryImpl::COMMITTED_BITMAP < IMPL) {// Могут попасться неподтвержденные страницы.

				if (HeapDirectoryImpl::RESERVED_BITMAP_WITH_EXCEPTIONS == IMPL) {

					if ((bit_number = reserved_scan (bitmap_ptr, end)) < 0)
						goto tryagain;

				} else {

					size_t page_size = HeapDirectoryBase::commit_unit (this);
					assert (page_size);
					for (;;) {
						UWord* page_end = round_up (bitmap_ptr + 1, page_size);
						for (;;) {
							while (!Port::Memory::is_readable (bitmap_ptr, sizeof (UWord))) {
								if ((bitmap_ptr = page_end) >= end)
									goto tryagain;
								else
									page_end += page_size;
							}

							while ((bit_number = Ops::clear_rightmost_one (bitmap_ptr)) < 0) {
								if (++bitmap_ptr == end)
									goto tryagain;

								if (bitmap_ptr == page_end)
									goto next_page;
							}
							break;
						next_page:;
						}
						break;
					}
				}

			} else {
				while ((bit_number = Ops::clear_rightmost_one (bitmap_ptr)) < 0)
					if (++bitmap_ptr == end)
						goto tryagain;
			}

		}

		break;
	tryagain:
		Ops::release (free_blocks_ptr);
	}

	assert (bit_number >= 0);

	// По смещению в битовой карте и размеру блока определяем номер (адрес) блока.
	size_t level_bitmap_begin = bitmap_offset (bi.level);

	// Номер блока:
	assert ((bitmap_ptr - bitmap_) >= (ptrdiff_t)level_bitmap_begin);
	size_t block_number = (bitmap_ptr - bitmap_ - level_bitmap_begin) * sizeof (UWord) * 8;
	block_number += bit_number;

	// Определяем смещение блока в куче и его размер.
	size_t allocated_size = block_size (bi.level);
	size_t block_offset = block_number * allocated_size;
	size_t allocated_end = block_offset + allocated_size;
	assert (allocated_end <= Traits::UNIT_COUNT);

	// Выделен блок размером allocated_size. Нужен блок размером size.

	// Ensure that memory is committed and writeable.
	commit (block_offset, allocated_end, heap_info);

	// Освобождаем оставшуюся часть.
	try {
		release (block_offset + size, allocated_end);
	} catch (...) {
		// Release size bytes, not allocated_size bytes!
		// Если память была освобождена частично и произошел сбой (невозможно зафиксировать битоую карту, например)
		// Мы освобождаем блок, заканчивающийся на block_offset + size. Там же начинается успешно освобожденная часть.
		// Биты свободных блоков в этой части будут обнулены, как компаньоны освобождающихся блоков.
		// В результате будет восстановлено исходное состояние битовой карты.
		release (block_offset, block_offset + size, heap_info);
		throw;
	}

	return block_offset;
}

template <size_t DIRECTORY_SIZE, UWord HEAP_LEVELS, HeapDirectoryImpl IMPL>
bool HeapDirectory <DIRECTORY_SIZE, HEAP_LEVELS, IMPL>::allocate (size_t begin, size_t end, const HeapInfo* heap_info)
{
	assert (begin < end);
	if (end > Traits::UNIT_COUNT)
		return false;

	// Занимаем блок, разбивая его на блоки размером 2^n, смещение которых кратно их размеру.
	size_t allocated_begin = begin;  // Начало занятого пространства.
	size_t allocated_end = allocated_begin;    // Конец занятого пространства.
	while (allocated_end < end) {

		// Ищем минимальный уровень, на который выравнены смещение и размер.
		UWord level = level_align (allocated_end, end - allocated_end);

		// Находим начало битовой карты уровня и номер блока.
		size_t level_bitmap_begin = bitmap_offset (level);
		size_t bl_number = block_number (allocated_end, level);

		bool success = false;
		UWord* bitmap_ptr;
		UWord mask;
		for (;;) {

			bitmap_ptr = bitmap_ + level_bitmap_begin + bl_number / (sizeof (UWord) * 8);
			mask = (UWord)1 << (bl_number % (sizeof (UWord) * 8));
			volatile uint16_t& free_blocks_cnt = free_block_count (level, bl_number);
			// Decrement free blocks counter.
			if (Ops::acquire (&free_blocks_cnt)) {
				if (HeapDirectoryImpl::RESERVED_BITMAP_WITH_EXCEPTIONS == IMPL) {
					if ((success = reserved_bit_clear (bitmap_ptr, mask)))
						break;
				} else {
					if (
						(HeapDirectoryImpl::COMMITTED_BITMAP >= IMPL || Port::Memory::is_readable (bitmap_ptr, sizeof (UWord)))
						&&
						Ops::bit_clear (bitmap_ptr, mask)
						) {
						success = true; // Block has been allocated
						break;
					}
				}
				Ops::release (&free_blocks_cnt);
			}

			if (!level)
				break; // Unsuccessfull. Range is not free.

			// Level up
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

	try { // Release extra space at begin and end
		// Память освобождается изнутри - наружу, чтобы, в случае сбоя
		// (невозможность зафиксировать битовую карту) и последующего освобождения внутренней
		// части, восстановилось исходное состояние.
		release (allocated_begin, begin, 0, true);
		release (end, allocated_end, 0, false);
	} catch (...) {
		release (begin, end, heap_info);
		throw;
	}

	return true;
}

template <size_t DIRECTORY_SIZE, UWord HEAP_LEVELS, HeapDirectoryImpl IMPL> inline
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

template <size_t DIRECTORY_SIZE, UWord HEAP_LEVELS, HeapDirectoryImpl IMPL>
void HeapDirectory <DIRECTORY_SIZE, HEAP_LEVELS, IMPL>::release (size_t begin, size_t end, const HeapInfo* heap_info, bool rtl)
{
	assert (begin <= end);
	assert (end <= Traits::UNIT_COUNT);

	// Decommit blocks at this level. Note: decommit_level can be negative if commit_size greater than maximal block size.
	Word decommit_level = Traits::HEAP_LEVELS;
	if (IMPL != HeapDirectoryImpl::PLAIN_MEMORY && heap_info) {
		assert (heap_info->heap && heap_info->unit_size && heap_info->commit_size);
		decommit_level = Traits::HEAP_LEVELS - 32 + nlz ((uint32_t)(heap_info->commit_size / heap_info->unit_size));
	}

	// Освобождаемый блок должен быть разбит на блоки размером 2^n, смещение которых
	// кратно их размеру.
	while (begin < end) {

		// Смещение блока в куче
		UWord level;
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

		// Освобождаем блок со смещением block_begin на уровне level

		// Находим начало битовой карты уровня и номер блока
		size_t level_bitmap_begin = bitmap_offset (level);
		size_t bl_number = block_number (block_begin, level);

		// Определяем адрес слова в битовой карте
		UWord* bitmap_ptr = bitmap_ + level_bitmap_begin + bl_number / (sizeof (UWord) * 8);
		UWord mask = (UWord)1 << (bl_number % (sizeof (UWord) * 8));
		volatile uint16_t* free_blocks_cnt = &free_block_count (level, bl_number);

		while (level > 0) {

			// Decommit freed memory.
			if (IMPL != HeapDirectoryImpl::PLAIN_MEMORY && level == decommit_level)
				Port::Memory::decommit ((uint8_t*)(heap_info->heap) + bl_number * heap_info->commit_size, heap_info->commit_size);

			if (HeapDirectoryImpl::RESERVED_BITMAP_WITH_EXCEPTIONS == IMPL) {

				if (reserved_bit_set_check_companion (bitmap_ptr, mask, companion_mask (mask))) {
					// Есть свободный компаньон, объединяем его с освобождаемым блоком
					// Поднимаемся на уровень выше
					Ops::decrement (free_blocks_cnt);
					--level;
					level_bitmap_begin = bitmap_offset_prev (level_bitmap_begin);
					bl_number >>= 1;
					mask = (UWord)1 << (bl_number % (sizeof (UWord) * 8));
					bitmap_ptr = bitmap_ + level_bitmap_begin + bl_number / (sizeof (UWord) * 8);
					free_blocks_cnt = &free_block_count (level, bl_number);
				} else
					break;

			} else {

				if (HeapDirectoryImpl::COMMITTED_BITMAP < IMPL) // We have to set bit or clear companion bit here. So memory have to be committed anyway.
					Port::Memory::commit (bitmap_ptr, sizeof (UWord));

				if (Ops::bit_set_check_companion (bitmap_ptr, mask, companion_mask (mask))) {
					// Есть свободный компаньон, объединяем его с освобождаемым блоком
					// Поднимаемся на уровень выше
					Ops::decrement (free_blocks_cnt);
					--level;
					level_bitmap_begin = bitmap_offset_prev (level_bitmap_begin);
					bl_number >>= 1;
					mask = (UWord)1 << (bl_number % (sizeof (UWord) * 8));
					bitmap_ptr = bitmap_ + level_bitmap_begin + bl_number / (sizeof (UWord) * 8);
					free_blocks_cnt = &free_block_count (level, bl_number);
				} else
					break;
			}
		}

		// Заранее увеличиваем счетчик свободных блоков, чтобы другие потоки знали,
		// что свободный блок скоро появится и начинали поиск. Это должно уменьшить вероятность
		// ложных отказов. Хотя может вызвать дополительные циклы поиска.
		Ops::release (free_blocks_cnt);

		if (level == 0) {
			if (IMPL != HeapDirectoryImpl::PLAIN_MEMORY && decommit_level <= 0) {
				if (decommit_level == 0) {
					Port::Memory::decommit ((uint8_t*)(heap_info->heap) + bl_number * heap_info->commit_size, heap_info->commit_size);
					Ops::bit_set (bitmap_ptr, mask);
				} else {
					UWord block_size = (UWord)1 << (UWord)(-decommit_level);
					UWord companion_mask = (~((~(UWord)0) << block_size)) << round_down ((bl_number % (sizeof (UWord) * 8)), block_size);
					if (Ops::bit_set_check_companion (bitmap_ptr, mask, companion_mask)) {
						try {
							Port::Memory::decommit ((uint8_t*)(heap_info->heap) + (bl_number >> (UWord)(-decommit_level)) * heap_info->commit_size, heap_info->commit_size);
						} catch (...) {
						}
						Ops::bit_set (bitmap_ptr, companion_mask);
					}
				}
			} else
				Ops::bit_set (bitmap_ptr, mask);
		}

		// Блок освобожден
		if (rtl)
			end = block_begin;
		else
			begin = block_end;
	}
}

template <size_t DIRECTORY_SIZE, UWord HEAP_LEVELS, HeapDirectoryImpl IMPL>
bool HeapDirectory <DIRECTORY_SIZE, HEAP_LEVELS, IMPL>::check_allocated (size_t begin, size_t end) const
{
	if (begin >= Traits::UNIT_COUNT || end > Traits::UNIT_COUNT || end <= begin)
		return false;

	uintptr_t page_size = 0;
	if (HeapDirectoryImpl::COMMITTED_BITMAP < IMPL) {
		page_size = HeapDirectoryBase::commit_unit (this);
		assert (page_size);
	}

	// Check for all bits on all levels are 0
	Word level = Traits::HEAP_LEVELS - 1;
	size_t level_bitmap_begin = bitmap_offset (Traits::HEAP_LEVELS - 1);
	for (;;) {

		assert (begin < end);

		const UWord* begin_ptr = bitmap_ + level_bitmap_begin + begin / (sizeof (UWord) * 8);
		UWord begin_mask = (~(UWord)0) << (begin % (sizeof (UWord) * 8));
		const UWord* end_ptr = bitmap_ + level_bitmap_begin + (end - 1) / (sizeof (UWord) * 8);
		UWord end_mask = (~(UWord)0) >> ((sizeof (UWord) * 8) - (((end - 1) % (sizeof (UWord) * 8)) + 1));
		assert (end_ptr < bitmap_ + Traits::BITMAP_SIZE);
		assert (end_mask);
		assert (begin_ptr <= end_ptr);
		assert (begin_mask);

		if (begin_ptr >= end_ptr) {

			if (HeapDirectoryImpl::RESERVED_BITMAP_WITH_EXCEPTIONS == IMPL) {
				HEAP_DIR_TRY {
					if (*begin_ptr & begin_mask & end_mask)
						return false;
				} HEAP_DIR_CATCH {
					;
				}
			} else {
				if (
					(HeapDirectoryImpl::COMMITTED_BITMAP >= IMPL || Port::Memory::is_readable (begin_ptr, sizeof (UWord)))
					&&
					(*begin_ptr & begin_mask & end_mask)
					)
					return false;
			}

		} else if (HeapDirectoryImpl::COMMITTED_BITMAP < IMPL) {
			assert (page_size);

			if (HeapDirectoryImpl::RESERVED_BITMAP_WITH_EXCEPTIONS == IMPL) {
				HEAP_DIR_TRY {
					if (*begin_ptr & begin_mask)
						return false;
					++begin_ptr;
				} HEAP_DIR_CATCH {
					begin_ptr = round_up (begin_ptr + 1, page_size);
				}

				for (;;) {
					HEAP_DIR_TRY {
						while (begin_ptr < end_ptr) {
							if (*begin_ptr)
								return false;
							++begin_ptr;
						}
					break;
					} HEAP_DIR_CATCH {
						begin_ptr = round_up (begin_ptr + 1, page_size);
					}
				}

				HEAP_DIR_TRY {
					if (begin_ptr == end_ptr && (*end_ptr & end_mask))
						return false;
				} HEAP_DIR_CATCH{
					;
				}

			} else { // Don't use exception

				// End of readable memory.
				const UWord* page_end = round_down (begin_ptr, page_size) + page_size / sizeof (UWord);

				if (Port::Memory::is_readable (begin_ptr, sizeof (UWord))) {
					if (*begin_ptr & begin_mask)
						return false;
					++begin_ptr;
				} else
					begin_ptr = page_end;

				while (begin_ptr < end_ptr) {
					for (const UWord* end = ::std::min (end_ptr, page_end); begin_ptr < end; ++begin_ptr)
						if (*begin_ptr)
							return false;

					while (begin_ptr < end_ptr) {
						if (Port::Memory::is_readable (begin_ptr, sizeof (UWord))) {
							page_end = begin_ptr + page_size;
							break;
						} else
							begin_ptr += page_size;
					}
				}

				if (
					(begin_ptr <= end_ptr)
					&&
					((end_ptr < page_end) || Port::Memory::is_readable (end_ptr, sizeof (UWord)))
					&&
					(*end_ptr & end_mask)
					)
					return false;

			}

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
