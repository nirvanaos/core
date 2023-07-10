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
#include "UserAllocator.h"
#include <memory>

namespace Nirvana {

typedef void (*Deleter) (void*);

namespace Core {

/// Thread-local storage.
class TLS
{
	typedef BitmapOps::BitmapWord BitmapWord;
	static const unsigned BW_BITS = sizeof (BitmapWord) * 8;

	/// Limit of the user TLS indexes.
	static const unsigned USER_TLS_INDEXES = 128;

public:
	/// Called on system startup.
	static void initialize () noexcept
	{
		free_count_ = USER_TLS_INDEXES_END;
		std::fill_n (bitmap_, BITMAP_SIZE, ~0);
	}

	/// Get current TLS object.
	/// 
	/// \returns Current TLS reference.
	/// \throws CORBA::NO_IMPLEMENT if current memory context is not user memory context.
	static TLS& current ();

	/// Allocate TLS index.
	///
	/// \returns New TLS index.
	/// \throws CORBA::IMP_LIMIT if the limit of the user TLS indexes is reached.
	static unsigned allocate ();

	/// Release TLS index.
	///
	/// \param idx TLS index.
	/// \throws CORBA::BAD_PARAM if \p is not allocated.
	static void release (unsigned idx);

	/// Set TLS value.
	/// 
	/// \param idx TLS index.
	/// \param p Value.
	/// \param deleter Optional deleter function.
	/// \throws CORBA::BAD_PARAM if \p idx is wrong.
	void set (unsigned idx, void* p, Deleter deleter);

	/// Get TLS value.
	/// 
	/// \param idx TLS index.
	/// \returns TLS value.
	void* get (unsigned idx) const noexcept;

	class Holder
	{
	public:
		TLS& instance ();

	private:
		std::unique_ptr <TLS> p_;
	};

	~TLS ();

private:
	friend class Holder;

	TLS ();

	class Entry
	{
	public:
		Entry () :
			ptr_ (nullptr),
			deleter_ (nullptr)
		{}

		Entry (void* ptr, Deleter deleter) noexcept :
			ptr_ (ptr),
			deleter_ (deleter)
		{}

		Entry (Entry&& src) noexcept :
			ptr_ (src.ptr_),
			deleter_ (src.deleter_)
		{
			src.deleter_ = nullptr;
		}

		Entry& operator = (Entry&& src) noexcept
		{
			destruct ();
			ptr_ = src.ptr_;
			deleter_ = src.deleter_;
			src.deleter_ = nullptr;
			return *this;
		}

		~Entry ()
		{
			destruct ();
		}

		void* ptr () const
		{
			return ptr_;
		}

		void reset () noexcept
		{
			ptr_ = nullptr;
			deleter_ = nullptr;
		}

	private:
		void destruct () noexcept;

	private:
		void* ptr_;
		Deleter deleter_;
	};

	typedef std::vector <Entry, UserAllocator <Entry> > Entries;
	Entries entries_;

	static const size_t BITMAP_SIZE = (USER_TLS_INDEXES + BW_BITS - 1) / BW_BITS;
	static BitmapWord bitmap_ [BITMAP_SIZE];

	static const unsigned USER_TLS_INDEXES_END = BITMAP_SIZE * sizeof (BitmapWord) * 8;

	static uint16_t free_count_;
};

}
}

#endif
