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

#include "BitmapOps.h"

namespace Nirvana {

typedef void (*Deleter) (void*);

namespace Core {

/// Thread-local storage.
class TLS
{
	typedef BitmapOps::BitmapWord BitmapWord;
	static const unsigned BW_BITS = sizeof (BitmapWord) * 8;
public:
	/// Reserved TLS indexes
	enum
	{
		CORE_TLS_BINDER, ///< Binder context
		CORE_TLS_OBJECT_FACTORY, ///< ObjectFactory stateless creation frame

		CORE_TLS_COUNT
	};

	/// Limit of the user TLS indexes.
	static const unsigned USER_TLS_INDEXES = 64;

	TLS ();
	~TLS ();

	static unsigned allocate ();
	static void release (unsigned idx);

	void set (unsigned idx, void* p, Deleter deleter = nullptr);
	void* get (unsigned idx) NIRVANA_NOEXCEPT;

	static void initialize () NIRVANA_NOEXCEPT
	{
		free_count_ = USER_TLS_INDEXES_END;
		std::fill_n (bitmap_, BITMAP_SIZE, ~0);
	}

	void clear () NIRVANA_NOEXCEPT
	{
		Entries tmp (std::move (entries_));
	}

private:
	class Entry
	{
	public:
		Entry () :
			ptr_ (nullptr),
			deleter_ (nullptr)
		{}

		Entry (void* ptr, Deleter deleter) NIRVANA_NOEXCEPT :
			ptr_ (ptr),
			deleter_ (deleter)
		{}

		Entry (Entry&& src) NIRVANA_NOEXCEPT :
			ptr_ (src.ptr_),
			deleter_ (src.deleter_)
		{
			src.deleter_ = nullptr;
		}

		Entry& operator = (Entry&& src) NIRVANA_NOEXCEPT
		{
			ptr_ = src.ptr_;
			deleter_ = src.deleter_;
			src.deleter_ = nullptr;
			return *this;
		}

		~Entry ()
		{
			if (deleter_ && ptr_) {
				try {
					(*deleter_) (ptr_);
				} catch (...) {
					// TODO: Log
				}
			}
		}

		void* ptr () const
		{
			return ptr_;
		}

	private:
		void* ptr_;
		Deleter deleter_;
	};

	// Do not use UserAllocator here to avoid infinite recursion in Debug configuration.
	// std::vector allocates proxy on construct.
	// We use our vector implementation without proxies.
	typedef std::vector <Entry> Entries;
	Entries entries_;

	static const size_t BITMAP_SIZE = (USER_TLS_INDEXES + BW_BITS - 1) / BW_BITS;
	static BitmapWord bitmap_ [BITMAP_SIZE];

	static const unsigned USER_TLS_INDEXES_END = BITMAP_SIZE * sizeof (BitmapWord) * 8;

	static uint16_t free_count_;
};

}
}

#endif
