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
#include "TLS.h"

using namespace std;

namespace Nirvana {
namespace Core {

TLS::BitmapWord TLS::bitmap_ [BITMAP_SIZE];
uint16_t TLS::free_count_;

void TLS::Entry::destruct () NIRVANA_NOEXCEPT
{
	if (deleter_ && ptr_) {
		try {
			(*deleter_) (ptr_);
		} catch (...) {
			// TODO: Log
		}
	}
}

TLS::TLS ()
{}

TLS::~TLS ()
{}

unsigned TLS::allocate ()
{
	if (BitmapOps::acquire (&free_count_)) {
		for (BitmapWord* p = bitmap_;;) {
			int bit;
			do {
				bit = BitmapOps::clear_rightmost_one (p);
				if (bit >= 0)
					return (unsigned)((p - bitmap_) * sizeof (BitmapWord) * 8) + bit + CORE_TLS_COUNT;
			} while (end (bitmap_) != ++p);
		}
	} else
		throw_IMP_LIMIT ();
}

void TLS::release (unsigned idx)
{
	if (idx < CORE_TLS_COUNT || idx >= CORE_TLS_COUNT + USER_TLS_INDEXES_END)
		throw_BAD_PARAM ();
	idx -= CORE_TLS_COUNT;
	size_t i = idx / BW_BITS;
	BitmapWord mask = (BitmapWord)1 << (idx % BW_BITS);
	if (BitmapOps::bit_set (bitmap_ + i, mask))
		BitmapOps::release (&free_count_);
	else
		throw_BAD_PARAM ();
}

void TLS::set (unsigned idx, void* p, Deleter deleter)
{
	if (idx >= CORE_TLS_COUNT + USER_TLS_INDEXES_END)
		throw_BAD_PARAM ();
	if (!p)
		deleter = nullptr;
	if (entries_.size () <= idx) {
		if (!p)
			return;
		entries_.resize (idx + 1);
	}
	entries_ [idx] = Entry (p, deleter);
}

void* TLS::get (unsigned idx) NIRVANA_NOEXCEPT
{
	if (entries_.size () <= idx)
		return nullptr;
	else
		return entries_ [idx].ptr ();
}

}
}
