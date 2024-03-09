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
#ifndef NIRVANA_CORE_TLS_CONTEXT_H_
#define NIRVANA_CORE_TLS_CONTEXT_H_
#pragma once

#include "UserAllocator.h"

namespace Nirvana {

typedef void (*Deleter) (void*);

namespace Core {

/// Thread-local storage context.
class TLS_Context
{
public:
	void set_value (unsigned idx, void* p, Deleter deleter)
	{
		if (!p)
			deleter = nullptr;
		if (entries_.size () <= idx) {
			if (!p)
				return;
			entries_.resize (idx + 1);
		}
		entries_ [idx] = Entry (p, deleter);
	}

	void* get_value (unsigned idx) const noexcept
	{
		// Do not check that index is really allocated, just return nullptr.
		// It is for performance.
		if (entries_.size () <= idx)
			return nullptr;
		else
			return entries_ [idx].ptr ();
	}

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
};

}
}

#endif
