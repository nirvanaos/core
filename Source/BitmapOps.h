/// \file
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
#ifndef NIRVANA_CORE_BITMAPOPS_H_
#define NIRVANA_CORE_BITMAPOPS_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Nirvana/bitutils.h>
#include <atomic>

namespace Nirvana {
namespace Core {

/// Lock-free bitmap operations
struct BitmapOps
{
	typedef size_t BitmapWord;

	// Atomic decrement free blocks counter if it is not zero.
	static bool acquire (volatile uint16_t* pcnt)
	{
		assert (std::atomic_is_lock_free ((volatile std::atomic <uint16_t>*)pcnt));
		uint16_t cnt = std::atomic_load ((volatile std::atomic <uint16_t>*)pcnt);
		while ((unsigned)(cnt - 1) < (unsigned)0x7FFF) {	// 0 < cnt && cnt <= 0x8000
			if (std::atomic_compare_exchange_strong ((volatile std::atomic <uint16_t>*)pcnt, &cnt, cnt - 1))
				return true;
		}
		return false;
	}

	// Atomic decrement unconditional.
	static void decrement (volatile uint16_t* pcnt)
	{
		std::atomic_fetch_sub ((volatile std::atomic <uint16_t>*)pcnt, 1);
	}

	// Atomic increment free blocks counter.
	static void release (volatile uint16_t* pcnt)
	{
		assert ((int16_t)*pcnt <= 0x7FFF);
		std::atomic_fetch_add ((volatile std::atomic <uint16_t>*)pcnt, 1);
	}

	// Clear rightmost not zero bit and return number of this bit.
	// Return -1 if all bits are zero.
	static int clear_rightmost_one (volatile BitmapWord* pbits)
	{
		assert (std::atomic_is_lock_free ((volatile std::atomic <BitmapWord>*)pbits));
		BitmapWord bits = std::atomic_load ((volatile std::atomic <BitmapWord>*)pbits);
		while (bits) {
			BitmapWord rbits = bits;
			if (std::atomic_compare_exchange_strong ((volatile std::atomic <BitmapWord>*)pbits, &bits, rbits & (rbits - 1)))
				return ntz (rbits);
		}
		return -1;
	}

	static bool bit_clear (volatile BitmapWord* pbits, BitmapWord mask)
	{
		BitmapWord bits = std::atomic_load ((volatile std::atomic <BitmapWord>*)pbits);
		while (bits & mask) {
			BitmapWord rbits = bits & ~mask;
			if (std::atomic_compare_exchange_strong ((volatile std::atomic <BitmapWord>*)pbits, &bits, rbits))
				return true;
		}
		return false;
	}

	static bool bit_set (volatile BitmapWord* pbits, BitmapWord mask)
	{
		assert (!(*pbits & mask));
		BitmapWord old = std::atomic_fetch_or ((volatile std::atomic <BitmapWord>*)pbits, mask);
		return (old & mask) == 0;
	}

	static bool bit_set_check_companion (volatile BitmapWord* pbits, BitmapWord mask, BitmapWord companion_mask)
	{
		BitmapWord bits = std::atomic_load ((volatile std::atomic <BitmapWord>*)pbits);
		assert (!(bits & mask));
		for (;;) {
			if (((bits | mask) & companion_mask) == companion_mask) {
				if (std::atomic_compare_exchange_strong ((volatile std::atomic <BitmapWord>*)pbits, &bits, bits & ~companion_mask))
					return true;
			} else if (std::atomic_compare_exchange_strong ((volatile std::atomic <BitmapWord>*)pbits, &bits, bits | mask))
				return false;
		}
	}
};

}
}

#endif
