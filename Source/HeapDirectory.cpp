/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
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
#include "HeapDirectory.h"

namespace Nirvana {
namespace Core {

using namespace std;

const UWord HeapDirectoryTraits <0x10000, 11>::block_index_offset_ [HEAP_LEVELS] =
{ // FREE_BLOCK_INDEX_SIZE == 15
	0,  // разделен на 4 части
	4,  // разделен на 2 части
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14
};

const HeapDirectoryBase::BitmapIndex HeapDirectoryTraits <0x10000, 11>::bitmap_index_ [FREE_BLOCK_INDEX_SIZE] =
{ // FREE_BLOCK_INDEX_SIZE == 15
	{0,  0},
	{1,  TOP_BITMAP_WORDS},
	{2,  TOP_BITMAP_WORDS * 3},
	{3,  TOP_BITMAP_WORDS * 7},
	{4,  TOP_BITMAP_WORDS * 15},
	{5,  TOP_BITMAP_WORDS * 31},
	{6,  TOP_BITMAP_WORDS * 63},
	{7,  TOP_BITMAP_WORDS * 127},
	{8,  TOP_BITMAP_WORDS * 255},
	{9,  TOP_BITMAP_WORDS * (511 + 256)},
	{9,  TOP_BITMAP_WORDS * 511},
	{10, TOP_BITMAP_WORDS * (1023 + 512 + 256)},
	{10, TOP_BITMAP_WORDS * (1023 + 512)},
	{10, TOP_BITMAP_WORDS * (1023 + 256)},
	{10, TOP_BITMAP_WORDS * 1023}
};

const UWord HeapDirectoryTraits <0x8000, 10>::block_index_offset_ [HEAP_LEVELS] =
{ // FREE_BLOCK_INDEX_SIZE == 11
	0,  // разделен на 2 части
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10
};

const HeapDirectoryBase::BitmapIndex HeapDirectoryTraits <0x8000, 10>::bitmap_index_ [FREE_BLOCK_INDEX_SIZE] =
{ // FREE_BLOCK_INDEX_SIZE == 11
	{0,  0},
	{1,  TOP_BITMAP_WORDS},
	{2,  TOP_BITMAP_WORDS * 3},
	{3,  TOP_BITMAP_WORDS * 7},
	{4,  TOP_BITMAP_WORDS * 15},
	{5,  TOP_BITMAP_WORDS * 31},
	{6,  TOP_BITMAP_WORDS * 63},
	{7,  TOP_BITMAP_WORDS * 127},
	{8,  TOP_BITMAP_WORDS * 255},
	{9,  TOP_BITMAP_WORDS * (511 + 256)},
	{9,  TOP_BITMAP_WORDS * 511}
};

const UWord HeapDirectoryTraits <0x8000, 11>::block_index_offset_ [HEAP_LEVELS] =
{ // FREE_BLOCK_INDEX_SIZE == 8
	0,  // разделен на 2 части
	2,
	3,
	4,
	5,
	6,
	7,  // 5 верхних уровней объединены
	7,
	7,
	7,
	7
};

const HeapDirectoryBase::BitmapIndex HeapDirectoryTraits <0x8000, 11>::bitmap_index_ [FREE_BLOCK_INDEX_SIZE] =
{ // FREE_BLOCK_INDEX_SIZE == 8
	{4,  TOP_BITMAP_WORDS * 15},
	{5,  TOP_BITMAP_WORDS * 31},
	{6,  TOP_BITMAP_WORDS * 63},
	{7,  TOP_BITMAP_WORDS * 127},
	{8,  TOP_BITMAP_WORDS * 255},
	{9,  TOP_BITMAP_WORDS * 511},
	{10, TOP_BITMAP_WORDS * (1023 + 512)},
	{10, TOP_BITMAP_WORDS * 1023}
};

// TODO: For 0x4000 directory size we can avoid usage of indexes.

const UWord HeapDirectoryTraits <0x4000, 9>::block_index_offset_ [HEAP_LEVELS] =
{ // FREE_BLOCK_INDEX_SIZE == 9
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8
};

const HeapDirectoryBase::BitmapIndex HeapDirectoryTraits <0x4000, 9>::bitmap_index_ [FREE_BLOCK_INDEX_SIZE] =
{ // FREE_BLOCK_INDEX_SIZE == 9
	{0,  0},
	{1,  TOP_BITMAP_WORDS},
	{2,  TOP_BITMAP_WORDS * 3},
	{3,  TOP_BITMAP_WORDS * 7},
	{4,  TOP_BITMAP_WORDS * 15},
	{5,  TOP_BITMAP_WORDS * 31},
	{6,  TOP_BITMAP_WORDS * 63},
	{7,  TOP_BITMAP_WORDS * 127},
	{8,  TOP_BITMAP_WORDS * 255}
};

const UWord HeapDirectoryTraits <0x4000, 11>::block_index_offset_ [HEAP_LEVELS] =
{ // FREE_BLOCK_INDEX_SIZE == 4
	0,
	1,
	2,
	3,  // 8 верхних уровней объединены
	3,
	3,
	3,
	3,
	3,
	3,
	3
};

const HeapDirectoryBase::BitmapIndex HeapDirectoryTraits <0x4000, 11>::bitmap_index_ [FREE_BLOCK_INDEX_SIZE] =
{ // FREE_BLOCK_INDEX_SIZE == 4
	// Можно обойтись вычислениями. Нужен ли такой массив?
	{7,  TOP_BITMAP_WORDS * 127},
	{8,  TOP_BITMAP_WORDS * 255},
	{9,  TOP_BITMAP_WORDS * 511},
	{10, TOP_BITMAP_WORDS * 1023}
};

}
}
