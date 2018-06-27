#ifndef NIRVANA_CORE_HEAPDIRECTORY_H_
#define NIRVANA_CORE_HEAPDIRECTORY_H_

#include "core.h"

/*
Используется механизм распределения памяти с двоичным квантованием размеров блоков.
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

#define HEAP_PARTS (HEAP_HEADER_SIZE / HEAP_DIRECTORY_SIZE)

// Размер кучи (2 бита заголовка на каждый HEAP_UNIT_MIN)

#define HEAP_PART_SIZE (HEAP_DIRECTORY_SIZE * 4 * HEAP_UNIT_MIN)

// Общий размер кучи

#define HEAP_SIZE (HEAP_PART_SIZE * HEAP_PARTS)

// Heap control block.
class HeapDirectory
{
public:

	void* operator new (size_t cb, void* p)
	{
		return p;
	}

	void operator delete (void*, void*)
	{}

	HeapDirectory ();

	bool empty () const;

	Pointer allocate (Pointer heap, Pointer p, UWord cb, UWord flags);
	void release (Pointer heap, Pointer p, UWord cb);
	bool check_allocated (UWord begin, UWord end);

private:

	Pointer reserve (Pointer heap, UWord cb);

	void release (UWord begin, UWord end, Pointer heap = 0, bool right_to_left = false);

	UWord level_align (UWord offset, UWord size);

	UWord unit_size (UWord level)
	{
		return sm_block_index1 [HEAP_LEVELS - 1 - level].m_block_size;
		/*
		А можно так, нужно посмотреть, что быстрее
		return HEAP_UNIT_MAX >> level;
		*/
	}

	UWord unit_number (UWord offset, UWord level)
	{
		return (offset / HEAP_UNIT_MIN) >> (HEAP_LEVELS - 1 - level);
	}

	UWord unit_offset (UWord number, UWord level)
	{
		return (number * HEAP_UNIT_MIN) << (HEAP_LEVELS - 1 - level);
	}

	UWord bitmap_offset (UWord level)
	{
		return (TOP_BITMAP_WORDS << level) - TOP_BITMAP_WORDS;
	}

	UWord bitmap_offset_next (UWord bitmap_offset)
	{
		return (bitmap_offset << 1) + TOP_BITMAP_WORDS;
	}

	UWord bitmap_offset_prev (UWord bitmap_offset)
	{
		return (bitmap_offset - TOP_BITMAP_WORDS) >> 1;
	}

	UShort& free_unit_count (UWord level, UWord unit_number)
	{
		return m_free_block_index [sm_block_index1 [HEAP_LEVELS - 1 - level].m_block_index_offset
#if (HEAP_DIRECTORY_SIZE > 0x4000)
			// Add index for splitted levels
			+ (unit_number >> (sizeof (UShort) * 8))
#endif
		];
	}

	enum
	{
		// Number of HEAP_UNIT_MAX per one heap partition
		MAX_UNITS_PER_PART = (HEAP_DIRECTORY_SIZE * 4) >> (HEAP_LEVELS - 1),

		// Размер верхнего уровня битовой карты в машинных словах
		TOP_BITMAP_WORDS = MAX_UNITS_PER_PART / (sizeof (UWord) * 8)
	};

	enum
	{
		// Size of bitmap (in words) in one heap partition
		BITMAP_SIZE = (~((~0) << HEAP_LEVELS)) * TOP_BITMAP_WORDS
	};

	enum
	{
		// Space available at top of bitmap for free block index
		FREE_BLOCK_INDEX_MAX = (HEAP_DIRECTORY_SIZE - BITMAP_SIZE * sizeof (UWord)) / sizeof (UShort)
	};

	// Массив, содержащий количество свободных блоков на каждом уровне.
	// Если общее количество блоков на уровне > 64K, он разделяется на части,
	// каждой из которых соответствует один элемент массива.
	// Таким образом, поиск производится, в худшем случае, среди 64K бит или 2K слов.
	// Если места в заголовке недостаточно, верхние уровни объединяются.
	// В этом случае, элемент массива содержит суммарное количество свободных блоков
	// на этих уровнях.
	// Массив перевернут - нижние уровни идут первыми.

	enum
	{

#if (HEAP_DIRECTORY_SIZE == 0x10000)
		FREE_BLOCK_INDEX_SIZE = 4 + 2 + MIN ((HEAP_LEVELS - 2), FREE_BLOCK_INDEX_MAX)
#elif (HEAP_DIRECTORY_SIZE == 0x8000)
		FREE_BLOCK_INDEX_SIZE = 2 + MIN ((HEAP_LEVELS - 1), FREE_BLOCK_INDEX_MAX)
#elif (HEAP_DIRECTORY_SIZE == 0x4000)
		FREE_BLOCK_INDEX_SIZE = MIN (HEAP_LEVELS, FREE_BLOCK_INDEX_MAX)
#else
#error HEAP_DIRECTORY_SIZE is invalid.
#endif
	};

	UShort m_free_block_index [FREE_BLOCK_INDEX_SIZE];

	// Битовая карта свободных блоков
	UWord m_bitmap [BITMAP_SIZE];

	// Alignment
	enum
	{
		ALIGNMENT = HEAP_DIRECTORY_SIZE - FREE_BLOCK_INDEX_SIZE * sizeof (UShort) - BITMAP_SIZE * sizeof (UWord)
	};

#if (ALIGNMENT != 0)
	Octet m_alignment [ALIGNMENT];
#endif
	//Octet m_alignment [HEAP_DIRECTORY_SIZE - FREE_BLOCK_INDEX_SIZE * sizeof (UShort) - BITMAP_SIZE * sizeof (UWord)];

	// Статические таблицы для ускорения поиска

	// BlockIndex1 По размеру блока определает смещение начала поиска в m_free_block_cnt

	struct BlockIndex1
	{
		bool operator < (const BlockIndex1& rhs) const
		{
			return m_block_size < rhs.m_block_size;
		}

		UWord m_block_size;
		UWord m_block_index_offset; // offset in m_free_block_index
	};

	static const BlockIndex1 sm_block_index1 [HEAP_LEVELS];

	// BlockIndex2 По обратному смещению от конца массива m_free_block_cnt определяет
	// уровень и положение области битовой карты.

	struct BlockIndex2
	{
		UWord m_level;
		UWord m_bitmap_offset;
	};

	static const BlockIndex2 sm_block_index2 [FREE_BLOCK_INDEX_SIZE];
};

}

#endif
