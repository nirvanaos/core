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

	/// Atomic decrement free blocks counter if it is not zero.
	/// 
	/// \param pcnt The free blocks counter pointer.
	/// 
	/// \returns `true` if the counter was decremented.
	static bool acquire (volatile uint16_t* pcnt) noexcept;

	/// Atomic decrement unconditional.
	/// 
	/// \param pcnt The free blocks counter pointer.
	static void decrement (volatile uint16_t* pcnt) noexcept
	{
		std::atomic_fetch_sub ((volatile std::atomic <uint16_t>*)pcnt, 1);
	}

	/// Atomic increment free blocks counter.
	/// 
	/// \param pcnt The free blocks counter pointer.
	static void release (volatile uint16_t* pcnt) noexcept
	{
		assert ((int16_t)*pcnt <= 0x7FFF);
		std::atomic_fetch_add ((volatile std::atomic <uint16_t>*)pcnt, 1);
	}

	/// Clear rightmost not zero bit and return number of this bit.
	/// 
	/// \param pbits The bitmap word pointer.
	/// 
	/// \returns Zero based bit number. -1 if all bits are zero.
	static int clear_rightmost_one (volatile BitmapWord* pbits) noexcept;

	/// Clear bitmap bit if it is not zero.
	/// 
	/// \param pbits The bitmap word pointer.
	/// \param mask The mask with exactly one bit set.
	/// 
	/// \returns `true` if the bit was zero and has been set. Otherwise `false`.
	/// 
	static bool bit_clear (volatile BitmapWord* pbits, BitmapWord mask) noexcept;

	/// Set the bitmap bits.
	///
	/// \param pbits The bitmap word pointer.
	/// \param mask The mask with bits to set.
	/// 
	/// \returns `true` if all mask bits were zero before.
	static bool bit_set (volatile BitmapWord* pbits, BitmapWord mask) noexcept
	{
		assert (!(*pbits & mask));
		BitmapWord old = std::atomic_fetch_or ((volatile std::atomic <BitmapWord>*)pbits, mask);
		return (old & mask) == 0;
	}

	/// \brief Set the bitmap bits or clear the companion bits
	///
	/// \param pbits The bitmap word pointer.
	/// \param mask The bits to set.
	/// \param companion_mask The companion bits.
	/// 
	/// If all bits from companion_mask are set, clear them and return `true`.
	/// Otherwise set mask bits and return `false`.
	/// 
	/// \throws CORBA::FREE_MEM if some of mask bits is already set.
	static bool bit_set_check_companion (volatile BitmapWord* pbits, BitmapWord mask,
		BitmapWord companion_mask) noexcept;
};

}
}

#endif
