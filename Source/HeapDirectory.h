#ifndef NIRVANA_CORE_HEAPDIRECTORY_H_
#define NIRVANA_CORE_HEAPDIRECTORY_H_

#include <Nirvana.h>
#include "../Interface/Memory.h"
#include <stddef.h>
#include <nlzntz.h>
#include <algorithm>
#include <atomic>

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
На месте верхушки расподожен индексный массив, UShort, содержащий информацию о
количестве свободных блоков на различных уровнях.
*/

namespace Nirvana {
namespace Core {

using namespace CORBA;

/*
HEAP_LEVELS - количество уровней (размеров блоков) кучи. Определяет максимальный
размер выделяемого блока HEAP_UNIT_MAX = HEAP_UNIT_MIN << (HEAP_LEVELS - 1).
Блоки большего размера выделяются напрямую базовым сервисом памяти.
Накладные расходы из-за выравнивания для базового
сервиса памяти составляют половину PROTECTION_UNIT (аппаратной страницы) на блок.
Поэтому HEAP_UNIT_MAX должен быть значительно больше размера страницы, и не сильно
меньшим ALLOCATION_UNIT и SHARING_UNIT.
Блоки такого большого размера выделяются достаточно редко.
Кроме того, они часто бывают выравнены по размеру страницы (буферы etc.).
Базовый сервис обычно выделяет каждый блок в отдельном SHARING_UNIT, что уменьшает
накладные расходы при копировании.
Таким образом, выделение достаточно больших блоков непосредственно через базовый сервис
можно считать оправданным.
В данной реализации количество уровней равно 11.
Это дает HEAP_UNIT_MAX = HEAP_UNIT_MIN * 1024, равный 16 (32)K. Для процессоров Intel
это 4 (8) страниц, что вполне достаточно.
В системах с небольшими величинами PROTECTION_UNIT, ALLOCATION_UNIT и SHARING_UNIT можно
попробовать количество уровней 10.
*/

class HeapDirectoryBase
{
	// Copy prohibited.
	HeapDirectoryBase (const HeapDirectoryBase&);
	HeapDirectoryBase operator = (const HeapDirectoryBase&);

public:
	static const UWord HEAP_LEVELS = 11;
	static const UWord MAX_BLOCK_SIZE = 1 << (HEAP_LEVELS - 1);

protected:
	struct BitmapIndex
	{
		UWord level;
		UWord bitmap_offset;
	};
};

/*
DIRECTORY_SIZE - размер управляющего блока кучи. Должен быть кратен PROTECTION_UNIT.
Управляющий блок содержит битовую карту свободных блоков и массив количества
свободных блоков на уровнях. Меньший размер более экономно расходует память,
выделяемую на управление кучей.
Так как максимальный размер блока меньше размера кучи, битовая карта выглядит,
как пирамида со срезанной верхушкой. На месте верхушки расподожен индексный массив,
UShort, содержащий информацию о количестве свободных блоков на различных уровнях.
Уменьшение размера управляющего блока кучи уменьшает размер срезанной верхушки
и оставляет меньше места для индекса. При этом приходится отводить один счетчик свободных
блоков на несколько верхних уровней. Это может увеличить время поиска свободного
блока в куче.
Размер управляющего блока принимается равным 16, 32 или 64 К. Меньшие и большие размеры
вряд ли дадут оптимальный результат.
*/

template <ULong DIRECTORY_SIZE>
class HeapDirectoryTraitsBase :
	public HeapDirectoryBase
{
public:
	static const UWord UNIT_COUNT = DIRECTORY_SIZE * 4;

protected:
	// Number of top level blocks.
	static const UWord TOP_LEVEL_BLOCKS = UNIT_COUNT >> (HEAP_LEVELS - 1);

	// Размер верхнего уровня битовой карты в машинных словах.
	static const UWord TOP_BITMAP_WORDS = TOP_LEVEL_BLOCKS / (sizeof (UWord) * 8);

	// Size of bitmap (in words).
	static const UWord BITMAP_SIZE = (~((~(UWord)0) << HEAP_LEVELS)) * TOP_BITMAP_WORDS;

	// Space available before bitmap for free block index.
	static const UWord FREE_BLOCK_INDEX_MAX = (DIRECTORY_SIZE - BITMAP_SIZE * sizeof (UWord)) / sizeof (UShort);
};

// There are 3 implementations of HeapDirectoryTraits for directory sizes 0x10000, 0x8000 and 0x4000 respectively.
template <ULong SIZE> class HeapDirectoryTraits;

template <>
class HeapDirectoryTraits <0x10000> :
	public HeapDirectoryTraitsBase <0x10000>
{
public:
	static const UWord FREE_BLOCK_INDEX_SIZE = 15;

	// По размеру блока определает смещение начала поиска в free_block_index_
	static const UWord block_index_offset_ [HEAP_LEVELS];

	// BitmapIndex По обратному смещению от конца массива free_block_index_ определяет
	// уровень и положение области битовой карты.
	static const BitmapIndex bitmap_index_ [FREE_BLOCK_INDEX_SIZE];
};

template <>
class HeapDirectoryTraits <0x8000> :
	public HeapDirectoryTraitsBase <0x8000>
{
public:
	static const UWord FREE_BLOCK_INDEX_SIZE = 8;

	// По размеру блока определает смещение начала поиска в free_block_index_
	static const UWord block_index_offset_ [HEAP_LEVELS];

	static const BitmapIndex bitmap_index_ [FREE_BLOCK_INDEX_SIZE];
};

template <>
class HeapDirectoryTraits <0x4000> :
	public HeapDirectoryTraitsBase <0x4000>
{
public:
	static const UWord FREE_BLOCK_INDEX_SIZE = 4;

	// По размеру блока определает смещение начала поиска в free_block_index_
	static const UWord block_index_offset_ [HEAP_LEVELS];

	static const BitmapIndex bitmap_index_ [FREE_BLOCK_INDEX_SIZE];
};

// Multithreaded implementation of HeapDirectory operations.
struct HeapDirectoryOpsLockFree
{
	// Atomic decrement free blocks counter if it is not zero.
	static bool acquire (volatile UShort* pcnt)
	{
		assert (::std::atomic_is_lock_free ((volatile ::std::atomic <UShort>*)pcnt));
		UShort cnt = ::std::atomic_load ((volatile ::std::atomic <UShort>*)pcnt);
		while ((unsigned)(cnt - 1) < (unsigned)0x7FFF) {	// 0 < cnt && cnt <= 0x8000
			if (::std::atomic_compare_exchange_strong ((volatile ::std::atomic <UShort>*)pcnt, &cnt, cnt - 1))
				return true;
		}
		return false;
	}

	// Atomic decrement unconditional.
	static void decrement (volatile UShort* pcnt)
	{
		::std::atomic_fetch_sub ((volatile ::std::atomic <UShort>*)pcnt, 1);
	}

	// Atomic increment free blocks counter.
	static void release (volatile UShort* pcnt)
	{
		assert ((Short)*pcnt <= 0x7FFF);
		::std::atomic_fetch_add ((volatile ::std::atomic <UShort>*)pcnt, 1);
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

/// Heap information
struct HeapInfo
{
	Pointer heap;
	UWord unit_size;
	UWord commit_size;
};

// Heap directory. Used for memory allocation on different levels of memory management.
// Heap directory allocates and deallocates abstract "units" in range (0 <= n < UNIT_COUNT).
// Each unit requires 2 bits of HeapDirectory size.
// USE_EXCEPTION = true provides better performance, but maybe not for all platforms.
template <ULong DIRECTORY_SIZE, bool USE_EXCEPTION>
class HeapDirectory :
	public HeapDirectoryTraits <DIRECTORY_SIZE>
{
	typedef HeapDirectoryTraits <DIRECTORY_SIZE> Traits;
	typedef HeapDirectoryOpsLockFree Ops;
	//	static_assert (Traits::FREE_BLOCK_INDEX_SIZE <= Traits::FREE_BLOCK_INDEX_MAX);

public:
	static void initialize (HeapDirectory <DIRECTORY_SIZE, USE_EXCEPTION>* zero_filled_buf)
	{
		assert (sizeof (HeapDirectory <DIRECTORY_SIZE, USE_EXCEPTION>) <= DIRECTORY_SIZE);

		// Bitmap is always aligned for performance.
		assert ((UWord)(&zero_filled_buf->bitmap_) % sizeof (UWord) == 0);

		// Initialize free blocs count on top level.
		zero_filled_buf->free_block_index_ [Traits::FREE_BLOCK_INDEX_SIZE - 1] = Traits::TOP_LEVEL_BLOCKS;

		// Initialize top level of bitmap by ones.
		::std::fill_n (zero_filled_buf->bitmap_, Traits::TOP_BITMAP_WORDS, ~(UWord)0);
	}

	static void initialize (HeapDirectory <DIRECTORY_SIZE, USE_EXCEPTION>* reserved_buf, Memory_ptr memory)
	{
		// Commit initial part.
		memory->commit (reserved_buf, reinterpret_cast <Octet*> (reserved_buf->bitmap_ + Traits::TOP_BITMAP_WORDS) - reinterpret_cast <Octet*> (reserved_buf));

		// Initialize
		initialize (reserved_buf);
	}

	/// <summary>Allocate block.</summary>
	/// <returns>Block offset in units if succeded, otherwise -1.</returns>
	Word allocate (UWord size, Memory_ptr memory = Memory_ptr::nil (), const HeapInfo* heap_info = 0);

	bool allocate (UWord begin, UWord end, Memory_ptr memory = Memory_ptr::nil (), const HeapInfo* heap_info = 0);

	// Checks that all units in range are allocated.
	bool check_allocated (UWord begin, UWord end, Memory_ptr memory = Memory_ptr::nil ()) const;

	void release (UWord begin, UWord end, Memory_ptr memory = Memory_ptr::nil (),
								const HeapInfo* heap_info = nullptr, bool right_to_left = false);

	bool empty () const
	{
		if (DIRECTORY_SIZE < 0x10000) {

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
	static UWord block_size (UWord level)
	{
		assert (level < Traits::HEAP_LEVELS);
		return Traits::MAX_BLOCK_SIZE >> level;
	}

	static UWord level_align (UWord offset, UWord size)
	{
		// Ищем максимальный размер блока <= size, на который выравнен offset
		UWord level = Traits::HEAP_LEVELS - 1 - ::std::min (ntz (offset | Traits::MAX_BLOCK_SIZE), 31 - nlz ((ULong)size));
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

	static UWord block_number (UWord unit, UWord level)
	{
		return unit >> (Traits::HEAP_LEVELS - 1 - level);
	}

	static UWord unit_number (UWord block, UWord level)
	{
		return block << (Traits::HEAP_LEVELS - 1 - level);
	}

	static UWord bitmap_offset (UWord level)
	{
		return (Traits::TOP_BITMAP_WORDS << level) - Traits::TOP_BITMAP_WORDS;
	}

	static UWord bitmap_offset_next (UWord level_bitmap_offset)
	{
		assert (level_bitmap_offset < Traits::BITMAP_SIZE - Traits::UNIT_COUNT / (sizeof (UWord) * 8));
		return (level_bitmap_offset << 1) + Traits::TOP_BITMAP_WORDS;
	}

	static UWord bitmap_offset_prev (UWord level_bitmap_offset)
	{
		assert (level_bitmap_offset >= Traits::TOP_BITMAP_WORDS);
		return (level_bitmap_offset - Traits::TOP_BITMAP_WORDS) >> 1;
	}

	UShort& free_block_count (UWord level, UWord block_number)
	{
		size_t idx = Traits::block_index_offset_ [Traits::HEAP_LEVELS - 1 - level];
		if (DIRECTORY_SIZE > 0x4000) {
			// Add index for splitted levels
			idx += (block_number >> (sizeof (UShort) * 8));
		}
		return free_block_index_ [idx];
	}

	static UWord companion_mask (UWord mask)
	{
		constexpr UWord ODD = sizeof (UWord) > 4 ? 0x5555555555555555 : 0x55555555;
		return mask | ((mask >> 1) & ODD) | ((mask << 1) & ~ODD);
	}

	void commit (UWord begin, UWord end, Memory_ptr memory, const HeapInfo* heap_info);

private:
	// Массив, содержащий количество свободных блоков на каждом уровне.
	// Если общее количество блоков на уровне > 64K, он разделяется на части,
	// каждой из которых соответствует один элемент массива.
	// Таким образом, поиск производится, в худшем случае, среди 64K бит или 2K слов.
	// Если места в заголовке недостаточно, верхние уровни объединяются.
	// В этом случае, элемент массива содержит суммарное количество свободных блоков
	// на этих уровнях.
	// Заметим, что для массива 64К бит, счетчик свободных блоков (бит) не может превышать
	// величину 32К, так как два соседних бита не могут быть одновременно установлены, в этом
	// случае они обнуляются и устанавливается бит на предыдущем уровне.
	// Массив перевернут - нижние уровни идут первыми.
	// Free block count index.
	UShort free_block_index_ [Traits::FREE_BLOCK_INDEX_SIZE];

	// Битовая карта свободных блоков. Компилятор должен выравнять на границу UWord.
	UWord bitmap_ [Traits::BITMAP_SIZE];
};

template <ULong DIRECTORY_SIZE, bool USE_EXCEPTION>
Word HeapDirectory <DIRECTORY_SIZE, USE_EXCEPTION>::allocate (UWord size, Memory_ptr memory, const HeapInfo* heap_info)
{
	assert (size);
	assert (size <= Traits::MAX_BLOCK_SIZE);

	// Quantize block size
	UWord level = nlz ((ULong)(size - 1)) + Traits::HEAP_LEVELS - 33;
	assert (level < Traits::HEAP_LEVELS);
	UWord block_index_offset = Traits::block_index_offset_ [Traits::HEAP_LEVELS - 1 - level];

	typename Traits::BitmapIndex bi;
	UWord* bitmap_ptr;
	Word bit_number;

	for (;;) {
		// Search in free block index
		UShort* free_blocks_ptr = free_block_index_ + block_index_offset;
		Word cnt = Traits::FREE_BLOCK_INDEX_SIZE - block_index_offset;
		while ((cnt--) && !Ops::acquire (free_blocks_ptr))
			++free_blocks_ptr;
		if (cnt < 0)
			return -1; // no such blocks

		// Определяем, где искать
		// Search in bitmap
		bi = Traits::bitmap_index_ [cnt];

		if (
			(DIRECTORY_SIZE < 0x10000) // Верхние уровни объединены
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

			if (memory) {// Могут попасться неподтвержденные страницы.
				UWord page_size = memory->query (this, Memory::COMMIT_UNIT);
				assert (page_size);

				if (USE_EXCEPTION) {

					for (;;) {
						try {
							while ((bit_number = Ops::clear_rightmost_one (bitmap_ptr)) < 0)
								if (++bitmap_ptr == end)
									goto tryagain;
							break;
						} catch (...) { // MEM_NOT_COMMITTED
							if ((bitmap_ptr = round_up (bitmap_ptr + 1, page_size)) >= end)
								goto tryagain;
						}
					}

				} else {

					for (;;) {
						UWord* page_end = round_up (bitmap_ptr + 1, page_size);
						for (;;) {
							while (!memory->is_readable (bitmap_ptr, sizeof (UWord))) {
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
	UWord level_bitmap_begin = bitmap_offset (bi.level);

	// Номер блока:
	assert ((UWord)(bitmap_ptr - bitmap_) >= level_bitmap_begin);
	UWord block_number = (bitmap_ptr - bitmap_ - level_bitmap_begin) * sizeof (UWord) * 8;
	block_number += bit_number;

	// Определяем смещение блока в куче и его размер.
	UWord allocated_size = block_size (bi.level);
	UWord block_offset = block_number * allocated_size;
	UWord allocated_end = block_offset + allocated_size;
	assert (allocated_end <= Traits::UNIT_COUNT);

	// Выделен блок размером allocated_size. Нужен блок размером size.

	// Commit if need.
	commit (block_offset, allocated_end, memory, heap_info);

	// Освобождаем оставшуюся часть.
	try {
		release (block_offset + size, allocated_end, memory);
	} catch (...) {
		// Release size bytes, not allocated_size bytes!
		// Если память была освобождена частично и произошел сбой (невозможно зафиксировать битоую карту, например)
		// Мы освобождаем блок, заканчивающийся на block_offset + size. Там же начинается успешно освобожденная часть.
		// Биты свободных блоков в этой части будут обнулены, как компаньоны освобождающихся блоков.
		// В результате будет восстановлено исходное состояние битовой карты.
		release (block_offset, block_offset + size, memory, heap_info);
		throw;
	}

	return block_offset;
}

template <ULong DIRECTORY_SIZE, bool USE_EXCEPTION>
bool HeapDirectory <DIRECTORY_SIZE, USE_EXCEPTION>::allocate (UWord begin, UWord end, Memory_ptr memory, const HeapInfo* heap_info)
{
	assert (begin < end);
	if (end > Traits::UNIT_COUNT)
		return false;

	// Занимаем блок, разбивая его на блоки размером 2^n, смещение которых кратно их размеру.
	UWord allocated_begin = begin;  // Начало занятого пространства.
	UWord allocated_end = allocated_begin;    // Конец занятого пространства.
	while (allocated_end < end) {

		// Ищем минимальный уровень, на который выравнены смещение и размер.
		UWord level = level_align (allocated_end, end - allocated_end);

		// Находим начало битовой карты уровня и номер блока.
		UWord level_bitmap_begin = bitmap_offset (level);
		UWord bl_number = block_number (allocated_end, level);

		bool success = false;
		UWord* bitmap_ptr;
		UWord mask;
		for (;;) {

			bitmap_ptr = bitmap_ + level_bitmap_begin + bl_number / (sizeof (UWord) * 8);
			mask = (UWord)1 << (bl_number % (sizeof (UWord) * 8));
			volatile UShort& free_blocks_cnt = free_block_count (level, bl_number);
			// Decrement free blocks counter.
			if (Ops::acquire (&free_blocks_cnt)) {
				if (USE_EXCEPTION) {
					try {
						if (Ops::bit_clear (bitmap_ptr, mask)) {
							success = true; // Block has been allocated
							break;
						}
					} catch (...) { // MEM_NOT_COMMITTED
					}
				} else {
					if (
						(!memory || memory->is_readable (bitmap_ptr, sizeof (UWord)))
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

		UWord block_offset = unit_number (bl_number, level);
		if (allocated_begin > block_offset)
			allocated_begin = block_offset;
		allocated_end = block_offset + block_size (level);
	}

	assert (allocated_begin <= begin && end <= allocated_end);
	assert (allocated_end <= Traits::UNIT_COUNT);

	// Commit if need.
	commit (allocated_begin, allocated_end, memory, heap_info);

	try { // Release extra space at begin and end
		// Память освобождается изнутри - наружу, чтобы, в случае сбоя
		// (невозможность зафиксировать битовую карту) и последующего освобождения внутренней
		// части, восстановилось исходное состояние.
		release (allocated_begin, begin, memory, 0, true);
		release (end, allocated_end, memory, 0, false);
	} catch (...) {
		release (begin, end, memory, heap_info);
		throw;
	}

	return true;
}

template <ULong DIRECTORY_SIZE, bool USE_EXCEPTION>
void HeapDirectory <DIRECTORY_SIZE, USE_EXCEPTION>::commit (UWord begin, UWord end, Memory_ptr memory, const HeapInfo* heap_info)
{
	if (heap_info) {
		assert (memory && heap_info->heap && heap_info->unit_size && heap_info->commit_size);
		UWord commit_size = min (Traits::MAX_BLOCK_SIZE * heap_info->unit_size, heap_info->commit_size);
		UWord commit_begin = round_up (begin * heap_info->unit_size, commit_size);
		UWord commit_end = round_down (end * heap_info->unit_size, commit_size);
		if (commit_begin < commit_end) {
			commit_begin = round_down (commit_begin, heap_info->commit_size);
			commit_end = round_up (commit_end, heap_info->commit_size);
			try {
				memory->commit ((Octet*)(heap_info->heap) + commit_begin, commit_end - commit_begin);
			} catch (...) {
				release (begin, end);
				throw;
			}
		}
	}
}

template <ULong DIRECTORY_SIZE, bool USE_EXCEPTION>
void HeapDirectory <DIRECTORY_SIZE, USE_EXCEPTION>::release (UWord begin, UWord end, Memory_ptr memory, const HeapInfo* heap_info, bool rtl)
{
	assert (begin <= end);
	assert (end <= Traits::UNIT_COUNT);

	// Decommit blocks at this level. Note: decommit_level can be negative if commit_size greater than maximal block size.
	Word decommit_level = Traits::HEAP_LEVELS;
	if (heap_info) {
		assert (memory && heap_info->heap && heap_info->unit_size && heap_info->commit_size);
		decommit_level = Traits::HEAP_LEVELS - 32 + nlz ((ULong)(heap_info->commit_size / heap_info->unit_size));
	}

	// Освобождаемый блок должен быть разбит на блоки размером 2^n, смещение которых
	// кратно их размеру.
	while (begin < end) {

		// Смещение блока в куче
		UWord level;
		UWord block_begin;
		UWord block_end;
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
		UWord level_bitmap_begin = bitmap_offset (level);
		UWord bl_number = block_number (block_begin, level);

		// Определяем адрес слова в битовой карте
		UWord* bitmap_ptr = bitmap_ + level_bitmap_begin + bl_number / (sizeof (UWord) * 8);
		UWord mask = (UWord)1 << (bl_number % (sizeof (UWord) * 8));
		volatile UShort* free_blocks_cnt = &free_block_count (level, bl_number);

		while (level > 0) {

			// Decommit freed memory.
			if (level == decommit_level)
				memory->decommit ((Octet*)(heap_info->heap) + bl_number * heap_info->commit_size, heap_info->commit_size);

			if (USE_EXCEPTION) {

				for (;;) {
					try {
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
							goto prev_level;
						} else
							break;
					} catch (...) { // MEM_NOT_COMMITTED
						memory->commit (bitmap_ptr, sizeof (UWord));
					}
				}
				break;
			prev_level:;

			} else {

				if (memory) // We have to set bit or clear companion bit here. So memory have to be committed anyway.
					memory->commit (bitmap_ptr, sizeof (UWord));

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
			if (decommit_level <= 0) {
				if (decommit_level == 0) {
					memory->decommit ((Octet*)(heap_info->heap) + bl_number * heap_info->commit_size, heap_info->commit_size);
					Ops::bit_set (bitmap_ptr, mask);
				} else {
					UWord block_size = (UWord)1 << (UWord)(-decommit_level);
					UWord companion_mask = (~((~(UWord)0) << block_size)) << round_down ((bl_number % (sizeof (UWord) * 8)), block_size);
					if (Ops::bit_set_check_companion (bitmap_ptr, mask, companion_mask)) {
						try {
							memory->decommit ((Octet*)(heap_info->heap) + (bl_number >> (UWord)(-decommit_level)) * heap_info->commit_size, heap_info->commit_size);
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

template <ULong DIRECTORY_SIZE, bool USE_EXCEPTION>
bool HeapDirectory <DIRECTORY_SIZE, USE_EXCEPTION>::check_allocated (UWord begin, UWord end, Memory_ptr memory) const
{
	if (begin >= Traits::UNIT_COUNT || end > Traits::UNIT_COUNT || end <= begin)
		return false;

	UWord page_size = 0;
	if (memory) {
		page_size = memory->query (this, Memory::COMMIT_UNIT);
		assert (page_size);
	}

	// Check for all bits on all levels are 0
	Word level = Traits::HEAP_LEVELS - 1;
	UWord level_bitmap_begin = bitmap_offset (Traits::HEAP_LEVELS - 1);
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

			if (USE_EXCEPTION) {
				try {
					if (*begin_ptr & begin_mask & end_mask)
						return false;
				} catch (...) { // MEM_NOT_COMMITTED
				}
			} else {
				if (
					(!memory || memory->is_readable (begin_ptr, sizeof (UWord)))
					&&
					(*begin_ptr & begin_mask & end_mask)
					)
					return false;
			}

		} else if (page_size) {

			if (USE_EXCEPTION) {
				try {
					if (*begin_ptr & begin_mask)
						return false;
					++begin_ptr;
				} catch (...) { // MEM_NOT_COMMITTED
					begin_ptr = round_up (begin_ptr + 1, page_size);
				}

				for (;;) {
					try {
						while (begin_ptr < end_ptr) {
							if (*begin_ptr)
								return false;
							++begin_ptr;
						}
						break;
					} catch (...) { // MEM_NOT_COMMITTED
						begin_ptr = round_up (begin_ptr + 1, page_size);
					}
				}

				try {
					if (begin_ptr == end_ptr && (*end_ptr & end_mask))
						return false;
				} catch (...) { // MEM_NOT_COMMITTED
				}

			} else { // Don't use exception

				// End of readable memory.
				const UWord* page_end = round_down (begin_ptr, page_size) + page_size / sizeof (UWord);

				if (memory->is_readable (begin_ptr, sizeof (UWord))) {
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
						if (memory->is_readable (begin_ptr, sizeof (UWord))) {
							page_end = begin_ptr + page_size;
							break;
						} else
							begin_ptr += page_size;
					}
				}

				if (
					(begin_ptr <= end_ptr)
					&&
					((end_ptr < page_end) || memory->is_readable (end_ptr, sizeof (UWord)))
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
