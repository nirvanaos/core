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

#include "UserAllocator.h"

namespace Nirvana {

typedef void (*Deleter) (void*);

namespace Core {

/// Thread-local storage.
class TLS
{
	typedef size_t BitmapWord;
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

	static unsigned alloc ();
	static void free (unsigned idx) NIRVANA_NOEXCEPT;

	void set (unsigned idx, void* p, Deleter deleter = nullptr);
	void* get (unsigned idx);

protected:
	void clear () NIRVANA_NOEXCEPT
	{
		Entries tmp (std::move (entries_));
	}

private:
	class Entry
	{
	public:
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
		}

		~Entry ()
		{
			if (deleter_ && ptr_)
				(*deleter_) (ptr_);
		}

	private:
		void* ptr_;
		Deleter deleter_;
	};

	typedef std::vector <Entry, UserAllocator <Entry> > Entries;
	Entries entries_;

	static const size_t BITMAP_SIZE = (USER_TLS_INDEXES + BW_BITS - 1) / BW_BITS;
	static BitmapWord bitmap_ [BITMAP_SIZE];

	static const unsigned USER_TLS_INDEXES_END = BITMAP_SIZE * sizeof (BitmapWord) * 8;
};

}
}

#endif
