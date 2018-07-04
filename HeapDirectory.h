#ifndef NIRVANA_CORE_HEAPDIRECTORY_H_
#define NIRVANA_CORE_HEAPDIRECTORY_H_

#include "core.h"
#include <Memory.h>
#include <stddef.h>
#include <nlzntz.h>
#include <algorithm>

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

template <ULong SIZE>
class HeapDirectoryTraitsBase :
	public HeapDirectoryBase
{
public:
	static const UWord UNIT_COUNT = SIZE * 4;

protected:
	// Number of top level blocks.
	static const UWord TOP_LEVEL_BLOCKS = UNIT_COUNT >> (HEAP_LEVELS - 1);

	// Размер верхнего уровня битовой карты в машинных словах.
	static const UWord TOP_BITMAP_WORDS = TOP_LEVEL_BLOCKS / (sizeof (UWord) * 8);

	// Size of bitmap (in words).
	static const UWord BITMAP_SIZE = (~((~0) << HEAP_LEVELS)) * TOP_BITMAP_WORDS;

	// Space available before bitmap for free block index.
	static const UWord FREE_BLOCK_INDEX_MAX = (SIZE - BITMAP_SIZE * sizeof (UWord)) / sizeof (UShort);
};

// There are 3 implementations of HeapDirectoryTraits for directory sizes 0x10000, 0x8000 and 0x4000 respectively.
template <ULong SIZE> class HeapDirectoryTraits;

template <>
class HeapDirectoryTraits <0x10000> :
	public HeapDirectoryTraitsBase <0x10000>
{
public:
	static const UWord FREE_BLOCK_INDEX_SIZE = 15;

	// По размеру блока определает смещение начала поиска в m_free_block_cnt
	static const UWord sm_block_index_offset [HEAP_LEVELS];

	// BitmapIndex По обратному смещению от конца массива m_free_block_cnt определяет
	// уровень и положение области битовой карты.
	static const BitmapIndex sm_bitmap_index [FREE_BLOCK_INDEX_SIZE];
};

template <>
class HeapDirectoryTraits <0x8000> :
	public HeapDirectoryTraitsBase <0x8000>
{
public:
	static const UWord FREE_BLOCK_INDEX_SIZE = 8;

	// По размеру блока определает смещение начала поиска в m_free_block_cnt
	static const UWord sm_block_index_offset [HEAP_LEVELS];

	static const BitmapIndex sm_bitmap_index [FREE_BLOCK_INDEX_SIZE];
};

template <>
class HeapDirectoryTraits <0x4000> :
	public HeapDirectoryTraitsBase <0x4000>
{
public:
	static const UWord FREE_BLOCK_INDEX_SIZE = 4;

	// По размеру блока определает смещение начала поиска в m_free_block_cnt
	static const UWord sm_block_index_offset [HEAP_LEVELS];

	static const BitmapIndex sm_bitmap_index [FREE_BLOCK_INDEX_SIZE];
};

// Heap directory. Used for memory allocation on different levels of memory management.
// Heap directory allocates and deallocates abstract "units" in range (0 <= n < UNIT_COUNT).
// Each unit requires 2 bits of HeapDirectory size.
template <UWord HEAP_DIRECTORY_SIZE>
class HeapDirectory :
	public HeapDirectoryTraits <HEAP_DIRECTORY_SIZE>
{
	typedef HeapDirectoryTraits <HEAP_DIRECTORY_SIZE> Traits;
//	static_assert (Traits::FREE_BLOCK_INDEX_SIZE <= Traits::FREE_BLOCK_INDEX_MAX);

public:
	static void initialize (HeapDirectory <HEAP_DIRECTORY_SIZE>* zero_filled_buf)
	{
		assert (sizeof (HeapDirectory <HEAP_DIRECTORY_SIZE>) <= HEAP_DIRECTORY_SIZE);

		// Bitmap is always aligned for performance.
		assert ((UWord)(&zero_filled_buf->m_bitmap) % sizeof (UWord) == 0);

		// Initialize free blocs count on top level.
		zero_filled_buf->m_free_block_index [Traits::FREE_BLOCK_INDEX_SIZE - 1] = Traits::TOP_LEVEL_BLOCKS;

		// Initialize top level of bitmap by ones.
		::std::fill_n (zero_filled_buf->m_bitmap, Traits::TOP_BITMAP_WORDS, ~0);
	}

	static HeapDirectory <HEAP_DIRECTORY_SIZE>* create (Memory_ptr memory)
	{
		// Reserve memory.
		HeapDirectory* p = reinterpret_cast <HeapDirectory*>(memory->allocate (0, sizeof (HeapDirectory), Memory::RESERVED | Memory::ZERO_INIT));

		// Commit initial part.
		memory->commit (p, reinterpret_cast <Octet*> (p->m_bitmap + TOP_BITMAP_WORDS) - reinterpret_cast <Octet*> (p));

		// Initialize
		initialize (p);

		return p;
	}

	/// <summary>Allocate block.</summary>
	/// <returns>Block offset in units if succeded, otherwise -1.</returns>
	Word allocate (UWord size, Memory_ptr memory = Memory_ptr::nil ());

	bool allocate (UWord begin, UWord end, Memory_ptr memory = Memory_ptr::nil ());

	// Checks that all units in range are allocated.
	bool check_allocated (UWord begin, UWord end, Memory_ptr memory = Memory_ptr::nil ()) const;
	
	void release (UWord begin, UWord end, Memory_ptr memory = Memory_ptr::nil (), bool right_to_left = false);

	bool empty () const
	{
		if (HEAP_DIRECTORY_SIZE < 0x10000) {

			// Верхние уровни объединены
			const UWord* end = m_bitmap + Traits::TOP_BITMAP_WORDS;

			for (const UWord* p = m_bitmap; p < end; ++p)
				if (~*p)
					return false;

			return true;

		} else
			return (Traits::TOP_LEVEL_BLOCKS == m_free_block_index [Traits::FREE_BLOCK_INDEX_SIZE - 1]);
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
		/* The check code.
		#ifdef _DEBUG
		UWord mask = Traits::MAX_BLOCK_SIZE - 1;
		UWord dlevel = 0;
		while ((mask & offset) || (mask >= size)) {
		mask >>= 1;
		++dlevel;
		}
		assert (dlevel == level);
		#endif
		*/
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
		size_t idx = Traits::sm_block_index_offset [Traits::HEAP_LEVELS - 1 - level];
		if (HEAP_DIRECTORY_SIZE > 0x4000) {
			// Add index for splitted levels
			idx += (block_number >> (sizeof (UShort) * 8));
		}
		return m_free_block_index [idx];
	}

	// Массив, содержащий количество свободных блоков на каждом уровне.
	// Если общее количество блоков на уровне > 64K, он разделяется на части,
	// каждой из которых соответствует один элемент массива.
	// Таким образом, поиск производится, в худшем случае, среди 64K бит или 2K слов.
	// Если места в заголовке недостаточно, верхние уровни объединяются.
	// В этом случае, элемент массива содержит суммарное количество свободных блоков
	// на этих уровнях.
	// Массив перевернут - нижние уровни идут первыми.
	// Free block count index.
	UShort m_free_block_index [Traits::FREE_BLOCK_INDEX_SIZE];

	// Битовая карта свободных блоков. Компилятор должен выравнять на границу UWord.
	UWord m_bitmap [Traits::BITMAP_SIZE];
};

template <UWord HEAP_DIRECTORY_SIZE>
Word HeapDirectory <HEAP_DIRECTORY_SIZE>::allocate (UWord size, Memory_ptr memory)
{
	assert (size);
	assert (size <= Traits::MAX_BLOCK_SIZE);

	// Quantize block size
	UWord level = nlz ((ULong)(size - 1)) + Traits::HEAP_LEVELS - 33;
	assert (level < Traits::HEAP_LEVELS);
	UWord block_index_offset = Traits::sm_block_index_offset [Traits::HEAP_LEVELS - 1 - level];

	// Search in free block index
	UShort* free_blocks_ptr = m_free_block_index + block_index_offset;
	Word cnt = Traits::FREE_BLOCK_INDEX_SIZE - block_index_offset;
	while ((cnt--) && !*free_blocks_ptr)
		++free_blocks_ptr;
	if (cnt < 0)
		return -1; // no such blocks

	// Определяем, где искать
	// Search in bitmap
	typename Traits::BitmapIndex bi = Traits::sm_bitmap_index [cnt];

	UWord* bitmap_ptr;

	if (
		(HEAP_DIRECTORY_SIZE < 0x10000) // Верхние уровни объединены
		&&
		!cnt
	) { // Верхние уровни

		// По индексу размера блока уточняем уровень и смещение начала поиска
		if (bi.level > level) {
			bi.level = level;

			bi.bitmap_offset = bitmap_offset (level);
			bitmap_ptr = m_bitmap + bi.bitmap_offset;
		}

		UWord* end = m_bitmap + bitmap_offset_next (bi.bitmap_offset);
		UWord* begin = bitmap_ptr = m_bitmap + bi.bitmap_offset;

		// Поиск в битовой карте. 
		// Верхние уровни всегда находятся в подтвержденной области.
		while (!*bitmap_ptr) {
			if (++bitmap_ptr >= end) {

				if (!bi.level)
					return 0; // Блок не найден

										// Поднимаемся на уровень выше
				--bi.level;
				end = begin;
				begin = bitmap_ptr = m_bitmap + (bi.bitmap_offset = bitmap_offset_prev (bi.bitmap_offset));
			}
		}

	} else { // На остальных уровнях ищем, как обычно

		// Ищем ненулевое слово. Проверка границы не нужна, так как ненулевой счетчик гарантирует,
		// что ненулевое слово есть. 

		bitmap_ptr = m_bitmap + bi.bitmap_offset;

		if (memory) {// Могут попасться неподтвержденные страницы.
			UWord page_size = memory->query (this, Memory::COMMIT_UNIT);
			assert (page_size);

			UWord* page_end = round_down (bitmap_ptr, page_size) + page_size / sizeof (UWord);

			for (;;) {
				while (!memory->is_readable (bitmap_ptr, sizeof (UWord))) {
					bitmap_ptr = page_end;
					page_end += page_size;
				}

				while (!*bitmap_ptr) {
					if (++bitmap_ptr == page_end)
						break;
				}
			}
		} else {
			while (!*bitmap_ptr)
				++bitmap_ptr;
		}

		// Если все работает правильно, ненулевой элемент в битовой карте обязательно будет найден.
		// Следующая проверка введена на случай, если структура данных кучи была испорчена.
		{
			Word end = bi.bitmap_offset + ::std::min (Traits::TOP_LEVEL_BLOCKS << bi.level, (UWord)0x10000) / sizeof (UWord);
			if ((bitmap_ptr - m_bitmap) >= end)
				throw INTERNAL ();
		}
	}

	// По смещению в битовой карте и размеру блока определяем номер (адрес) блока.
	UWord level_bitmap_begin = bitmap_offset (bi.level);

	// Номер блока:
	assert ((UWord)(bitmap_ptr - m_bitmap) >= level_bitmap_begin);
	UWord block_number = (bitmap_ptr - m_bitmap - level_bitmap_begin) * sizeof (UWord) * 8;

	// Сбрасываем крайний справа единичный бит
	UWord bits = *bitmap_ptr;
	assert (bits);
	*bitmap_ptr = bits & bits - 1;
	block_number += ntz (bits); // Номер этого бита прибавляем к номеру блока.
	--*free_blocks_ptr;   // Уменьшаем счетчик свободных блоков.

												// Определяем смещение блока в куче и его размер.
	UWord allocated_size = block_size (bi.level);
	UWord block_offset = block_number * allocated_size;

	// Выделен блок размером allocated_size. Нужен блок размером size.
	// Освобождаем оставшуюся часть.

	try {

		release (block_offset + size, block_offset + allocated_size);

	} catch (...) {
		// Release cb bytes, not allocated_size bytes!
		release (block_offset, block_offset + size);
		throw;
	}

	return block_offset;
}

template <UWord HEAP_DIRECTORY_SIZE>
bool HeapDirectory <HEAP_DIRECTORY_SIZE>::allocate (UWord begin, UWord end, Memory_ptr memory)
{
	assert (begin < end);
	assert (end <= Traits::UNIT_COUNT);

	// Занимаем блок, разбивая его на блоки размером 2^n, смещение которых кратно их размеру.
	UWord allocated_begin = begin;  // Начало занятого пространства.
	UWord allocated_end = allocated_begin;    // Конец занятого пространства.
	while (allocated_end < end) {

		// Ищем минимальный уровень, на который выравнены смещение и размер.
		UWord level = level_align (allocated_end, end - allocated_end);

		// Находим начало битовой карты уровня и номер блока.
		UWord level_bitmap_begin = bitmap_offset (level);
		UWord bl_number = block_number (allocated_end, level);

		UWord* bitmap_ptr;
		UWord mask;
		for (;;) {

			bitmap_ptr = m_bitmap + level_bitmap_begin + bl_number / (sizeof (UWord) * 8);
			mask = (UWord)1 << (bl_number % (sizeof (UWord) * 8));
			// Bitmap page can be not committed.
			if (
				(!memory || memory->is_readable (bitmap_ptr, sizeof (UWord)))
				&&
				(*bitmap_ptr & mask)
				)
				break;  // was found

			if (!level) {

				// No block found. Release allocated.
				release (allocated_begin, allocated_end, Memory_ptr::nil ());

				return false;
			}

			// Level up
			--level;
			bl_number = bl_number >> 1;
			level_bitmap_begin = bitmap_offset_prev (level_bitmap_begin);
		}

		// Clear free bit
		*bitmap_ptr &= ~mask;

		// Decrement free blocks counter
		--free_block_count (level, bl_number);

		UWord block_offset = unit_number (bl_number, level);
		if (allocated_begin < block_offset)
			allocated_begin = block_offset;
		allocated_end = block_offset + block_size (level);
	}

	try { // Release extra space at begin and end
		// Память освобождается изнутри - наружу, чтобы, в случае сбоя
		// (невозможность зафиксировать битовую карту) и последующего освобождения внутренней
		// части, восстановилось исходное состояние.
		release (allocated_begin, begin, memory, true);
		release (end, allocated_end, memory);
	} catch (...) {
		release (begin, end, Memory_ptr::nil ());
		throw;
	}

	return true;
}

template <UWord HEAP_DIRECTORY_SIZE>
void HeapDirectory <HEAP_DIRECTORY_SIZE>::release (UWord begin, UWord end, Memory_ptr memory, bool rtl)
{
	assert (begin <= end);
	assert (end <= Traits::UNIT_COUNT);

	// Освобождаемый блок должен быть разбит на блоки размером 2^n, смещение которых
	// кратно их размеру.
	while (begin < end) {

		// Смещение блока в куче
		UWord level;
		UWord block_begin;
		if (rtl) {
			level = level_align (end, end - begin);
			block_begin = end - block_size (level);
		} else {
			level = level_align (begin, end - begin);
			block_begin = begin;
		}

		// Освобождаем блок со смещением block_begin на уровне level

		// Находим начало битовой карты уровня и номер блока
		UWord level_bitmap_begin = bitmap_offset (level);
		UWord bl_number = block_number (block_begin, level);

		// Определяем адрес слова в битовой карте
		UWord* bitmap_ptr = m_bitmap + level_bitmap_begin + bl_number / (sizeof (UWord) * 8);
		UWord mask = (UWord)1 << (bl_number % (sizeof (UWord) * 8));

		while (level > 0) {

			// Проверяем, есть ли у блока свободный компаньон
			UWord companion_mask = (bl_number & 1) ? mask >> 1 : mask << 1;	// TODO: optimize?

			// Память битовой карты может не быть зафиксирована.
			if (
				(!memory || memory->is_readable (bitmap_ptr, sizeof (UWord)))
				&&
				(*bitmap_ptr & companion_mask)
			) {

				// Есть свободный компаньон, объединяем его с освобождаемым блоком

				// Убираем бит компаньона
				*bitmap_ptr &= ~companion_mask;
				--free_block_count (level, bl_number);

				// Поднимаемся на уровень выше
				--level;
				level_bitmap_begin = bitmap_offset_prev (level_bitmap_begin);
				bl_number >>= 1;
				mask = (UWord)1 << (bl_number % (sizeof (UWord) * 8));
				bitmap_ptr = m_bitmap + level_bitmap_begin + bl_number / (sizeof (UWord) * 8);
			} else
				break;
		}

		// Фиксируем память битовой карты
		if (memory)
			memory->commit (bitmap_ptr, sizeof (UWord));

		// Устанавливаем бит свободного блока

		*bitmap_ptr |= mask;

		// Увеличиваем счетчик свободных блоков
		++free_block_count (level, bl_number);

		// Блок освобожден
		if (rtl)
			end = block_begin;
		else
			begin = block_begin + block_size (level);
	}
}

template <UWord HEAP_DIRECTORY_SIZE>
bool HeapDirectory <HEAP_DIRECTORY_SIZE>::check_allocated (UWord begin, UWord end, Memory_ptr memory) const
{
	UWord page_size;
	if (memory) {
		page_size = memory->query (this, Memory::COMMIT_UNIT);
		assert (page_size);
	}

	// Check for all bits on all levels are 0
	Word level = Traits::HEAP_LEVELS - 1;
	UWord level_bitmap_begin = bitmap_offset (Traits::HEAP_LEVELS - 1);
	for (;;) {

		assert (begin < end);

		const UWord* begin_ptr = m_bitmap + level_bitmap_begin + begin / (sizeof (UWord) * 8);
		UWord begin_mask = (~(UWord)0) << (begin % (sizeof (UWord) * 8));
		const UWord* end_ptr = m_bitmap + level_bitmap_begin + end / (sizeof (UWord) * 8);
		UWord end_mask = ~((~(UWord)0) << (end % (sizeof (UWord) * 8)));

		if (begin_ptr >= end_ptr) {

			if (
				(!memory || memory->is_readable (begin_ptr, sizeof (UWord)))
				&&
				(*begin_ptr & begin_mask & end_mask)
			)
				return false;

		} else if (memory) {

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

				while (begin_ptr < end_ptr && !memory->is_readable (begin_ptr, sizeof (UWord))) {
					begin_ptr = page_end;
					page_end += page_size;
				}
			}

			if (
				((end_ptr < page_end) || memory->is_readable (end_ptr, sizeof (UWord)))
				&&
				(*end_ptr & end_mask)
				)
				return false;

		} else {

			if (*begin_ptr & begin_mask)
				return false;

			while (++begin_ptr < end_ptr)
				if (*begin_ptr)
					return false;

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

#endif
