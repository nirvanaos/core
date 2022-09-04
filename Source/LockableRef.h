/// \file
/// Core weak_ptr implementation.
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
#ifndef NIRVANA_CORE_LOCKABLEREF_H_
#define NIRVANA_CORE_LOCKABLEREF_H_
#pragma once

#include "CoreInterface.h"
#include "TaggedPtr.h"

namespace Nirvana {
namespace Core {

// TODO: Move to a separate file
template <class T>
class LockableRef
{
	LockableRef (const LockableRef&) = delete;
	LockableRef (LockableRef&&) = delete;
	LockableRef& operator = (const LockableRef&) = delete;
	LockableRef& operator = (LockableRef&&) = delete;

public:
	LockableRef () NIRVANA_NOEXCEPT :
		p_ (nullptr)
	{}

	~LockableRef ()
	{
		CoreRef <T> ref;
		ref.p_ = p_.load ();
	}

	LockableRef& operator = (CoreRef <T>&& src) NIRVANA_NOEXCEPT
	{
		CoreRef <T> tmp = std::move (src);
		swap (tmp);
		return *this;
	}

	CoreRef <T> get () const NIRVANA_NOEXCEPT
	{
		CoreRef <T> ref (p_.lock ());
		p_.unlock ();
		return ref;
	}

	void reset () NIRVANA_NOEXCEPT
	{
		CoreRef <T> tmp;
		swap (tmp);
	}

	void swap (CoreRef <T> ref) NIRVANA_NOEXCEPT
	{
		typename LockablePtrT <T, 0>::Ptr from;
		typename LockablePtrT <T, 0>::Ptr to (ref);
		do {
			from = p_.load ();
		} while (!p_.cas (from, to));
		ref.p_ = from;
	}

private:
	mutable LockablePtrT <T, 0> p_;
};

}
}

#endif
