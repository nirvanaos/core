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
#ifndef NIRVANA_CORE_TLS_H_
#define NIRVANA_CORE_TLS_H_
#pragma once

#include "MemContextUser.h"
#include "BitmapOps.h"

namespace Nirvana {

typedef void (*Deleter) (void*);

namespace Core {

/// Thread-local storage.
class TLS
{
	typedef BitmapOps::BitmapWord BitmapWord;
	static const unsigned BW_BITS = sizeof (BitmapWord) * 8;

	/// Limit of the user TLS indexes.
	static const unsigned USER_TLS_INDEXES_MIN = 128;

	static const size_t BITMAP_SIZE = (USER_TLS_INDEXES_MIN + BW_BITS - 1) / BW_BITS;
	static const unsigned USER_TLS_INDEXES_END = BITMAP_SIZE * sizeof (BitmapWord) * 8;

public:
	/// Called on system startup.
	static void initialize () noexcept
	{
		free_count_ = USER_TLS_INDEXES_END;
		std::fill_n (bitmap_, BITMAP_SIZE, ~0);
	}

	/// Allocate TLS index.
	///
	/// \returns New TLS index.
	/// \param deleter Optional deleter function.
	/// \throws CORBA::IMP_LIMIT if the limit of the user TLS indexes is reached.
	static unsigned allocate (Deleter deleter)
	{
		if (BitmapOps::acquire (&free_count_)) {
			for (BitmapWord* p = bitmap_;;) {
				int bit;
				do {
					bit = BitmapOps::clear_rightmost_one (p);
					if (bit >= 0) {
						unsigned idx = (unsigned)((p - bitmap_) * sizeof (BitmapWord) * 8) + bit;
						deleters_ [idx] = deleter;
						return idx;
					}
				} while (std::end (bitmap_) != ++p);
			}
		} else
			throw_IMP_LIMIT ();
	}

	/// Release TLS index.
	///
	/// \param idx TLS index.
	/// \throws CORBA::BAD_PARAM if \p is not allocated.
	static void release (unsigned idx)
	{
		if (idx >= USER_TLS_INDEXES_END)
			throw_BAD_PARAM ();
		size_t i = idx / BW_BITS;
		BitmapWord mask = (BitmapWord)1 << (idx % BW_BITS);
		if (BitmapOps::bit_set (bitmap_ + i, mask))
			BitmapOps::release (&free_count_);
		else
			throw_BAD_PARAM ();
	}

	/// Set TLS value.
	/// 
	/// \param idx TLS index.
	/// \param p Value.
	/// \throws CORBA::BAD_PARAM if \p idx is wrong.
	static void set (unsigned idx, void* p)
	{
		{
			if (idx >= USER_TLS_INDEXES_END)
				throw_BAD_PARAM ();
			size_t i = idx / BW_BITS;
			BitmapWord mask = (BitmapWord)1 << (idx % BW_BITS);
			if (bitmap_ [i] & mask)
				throw_BAD_PARAM (); // This index is free

			MemContextUser::current ().tls ().set_value (idx, p, deleters_ [idx]);
		}
	}

	/// Get TLS value.
	/// 
	/// \param idx TLS index.
	/// \returns TLS value.
	static void* get (unsigned idx) noexcept
	{
		MemContextUser* mc = MemContext::current ().user_context ();
		if (mc) {
			TLS_Context* ctx = mc->tls_ptr ();
			if (ctx)
				return ctx->get_value (idx);
		}
		return nullptr;
	}

private:
	static BitmapWord bitmap_ [BITMAP_SIZE];
	static Deleter deleters_ [USER_TLS_INDEXES_END];
	static uint16_t free_count_;
};

}
}

#endif
