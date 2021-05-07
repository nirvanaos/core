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
#ifndef NIRVANA_CORE_WAITLIST_H_
#define NIRVANA_CORE_WAITLIST_H_

#include "CoreInterface.h"
#include "CoreObject.h"
#include <exception>

namespace Nirvana {
namespace Core {

class ExecDomain;

class WaitList :
	public CoreObject
{
	template <class T> friend class WaitablePtr;
protected:
	WaitList () :
		state_ (State::INITIAL)
	{}

	void start_construction ();
	void end_construction ();
	void on_exception ();
	void wait_construction ();

	enum State
	{
		INITIAL,
		UNDER_CONSTRUCTION,
		EXCEPTION,
		CONSTRUCTED
	};

	State state () const NIRVANA_NOEXCEPT
	{
		return state_;
	}

private:
	State state_;

	//ExecDomain& worker_;
	ExecDomain* wait_list_;
	std::exception_ptr exception_;
};

template <class T>
class WaitListT :
	public WaitList
{
public:
	template <class ... Args>
	T& construct (Args ... args)
	{
		start_construction ();
		try {
			new (&object_) T (std::forward <Args> (args)...);
		} catch (...) {
			on_exception ();
			throw;
		}
		end_construction ();
		return reinterpret_cast <T&> (object_);
	}

protected:
	~WaitListT ()
	{
		if (state () == State::CONSTRUCTED)
			reinterpret_cast <T&> (object_).~T ();
	}

	T& get () NIRVANA_NOEXCEPT
	{
		return reinterpret_cast <T&> (this->object_);
	}

private:
	typename std::aligned_storage <sizeof (T), alignof (T)>::type object_;
};

template <class T>
class WaitListImpl : public ImplDynamicSync <WaitListT <T> >
{
	typedef ImplDynamicSync <WaitListT <T> > Base;
public:
	typedef CoreRef <WaitListImpl> Ref;

	T& get ()
	{
		if (Base::State::CONSTRUCTED != Base::state ()) {
			Ref ref (this);
			WaitList::wait_construction ();
		}
		return Base::get ();
	}
};

template <class T>
class WaitableRef :
	public WaitListImpl <T>::Ref
{
	typedef typename WaitListImpl <T>::Ref Base;
public:
	WaitableRef () :
		Base (Base::template create <WaitListImpl <T> > ())
	{}
};

}
}

#endif
