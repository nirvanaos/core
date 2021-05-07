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
#ifndef NIRVANA_CORE_COREINTERFACE_H_
#define NIRVANA_CORE_COREINTERFACE_H_

#include "AtomicCounter.h"
#include <assert.h>
#include <Nirvana/throw_exception.h>

namespace Nirvana {
namespace Core {

template <class> class CoreRef;

/// Core interface.
class NIRVANA_NOVTABLE CoreInterface
{
protected:
	template <class> friend class CoreRef;

	// TODO: NIRVANA_NOEXCEPT?
	virtual void _add_ref () = 0;
	virtual void _remove_ref () = 0;
};

template <class T> class CoreRef;

/// Dynamic implementation of a core object.
/// \tparam T object class.
template <class T>
class ImplDynamic :
	public T
{
protected:
	template <class> friend class CoreRef;

	template <class ... Args>
	ImplDynamic (Args ... args) :
		T (std::forward <Args> (args)...)
	{}

	~ImplDynamic ()
	{}

	void _add_ref ()
	{
		ref_cnt_.increment ();
	}

	void _remove_ref ()
	{
		if (!ref_cnt_.decrement ())
			delete this;
	}

private:
	RefCounter ref_cnt_;
};

/// Dynamic implementation of a core object for usage in synchronized scenarios.
/// \tparam T object class.
template <class T>
class ImplDynamicSync :
	public T
{
protected:
	template <class> friend class CoreRef;

	template <class ... Args>
	ImplDynamicSync (Args ... args) :
		T (std::forward <Args> (args)...),
		ref_cnt_ (1)
	{}

	~ImplDynamicSync ()
	{}

	void _add_ref ()
	{
		++ref_cnt_;
	}

	void _remove_ref ()
	{
		assert (ref_cnt_);
		if (!--ref_cnt_)
			delete this;
	}

private:
	unsigned ref_cnt_;
};

/// Static or stack implementation of a core object.
/// \tparam T object class.
template <class T>
class ImplStatic :
	public T
{
public:
	template <class ... Args>
	ImplStatic (Args ... args) :
		T (std::forward <Args> (args)...)
	{}

private:
	void _add_ref ()
	{}

	void _remove_ref ()
	{}
};

/// Special implementation of a core object.
/// \tparam T object class.
template <class T>
class ImplNoAddRef :
	public T
{
protected:
	template <class> friend class CoreRef;

	template <class ... Args>
	ImplNoAddRef (Args ... args) :
		T (std::forward <Args> (args)...)
	{}

	~ImplNoAddRef ()
	{}

	void _add_ref ()
	{
		assert (false);
		throw_INTERNAL ();
	}

	void _remove_ref ()
	{
		this->~ImplNoAddRef <T> ();
	}
};

/// Core smart pointer.
/// \tparam T object or core interface class.
///         Note that T haven't to derive from CoreInterface, but can.
template <class T>
class CoreRef
{
	template <class T1> friend class CoreRef;
public:
	CoreRef () NIRVANA_NOEXCEPT :
		p_ (nullptr)
	{}

	CoreRef (nullptr_t) NIRVANA_NOEXCEPT :
		p_ (nullptr)
	{}

	/// Increments reference counter unlike I_var.
	CoreRef (T* p) NIRVANA_NOEXCEPT :
		p_ (p)
	{
		if (p_)
			p_->_add_ref ();
	}

	template <class T1>
	CoreRef (const CoreRef <T1>& src) NIRVANA_NOEXCEPT :
		CoreRef (src.p_)
	{
		if (p_)
			p_->_add_ref ();
	}

	template <class T1>
	CoreRef (CoreRef <T1>&& src) NIRVANA_NOEXCEPT :
		p_ (src.p_)
	{
		src.p_ = nullptr;
	}

	/// Creates an object.
	/// \tparam Impl Object implementation class.
	template <class Impl, class ... Args>
	static CoreRef create (Args ... args)
	{
		CoreRef v;
		v.p_ = new Impl (std::forward <Args> (args)...);
		return v;
	}

	~CoreRef () NIRVANA_NOEXCEPT
	{
		if (p_)
			p_->_remove_ref ();
	}

	/// Increments reference counter unlike I_var.
	CoreRef& operator = (T* p) NIRVANA_NOEXCEPT
	{
		if (p_ != p) {
			reset (p);
			if (p)
				p->_add_ref ();
		}
		return *this;
	}

	template <class T1>
	CoreRef& operator = (const CoreRef <T1>& src) NIRVANA_NOEXCEPT
	{
		T* p = src.p_;
		if (p_ != p) {
			reset (p);
			if (p)
				p->_add_ref ();
		}
		return *this;
	}

	template <class T1>
	CoreRef& operator = (CoreRef <T1>&& src) NIRVANA_NOEXCEPT
	{
		if (this != &src) {
			reset (src.p_);
			src.p_ = nullptr;
		}
		return *this;
	}

	T* operator -> () const NIRVANA_NOEXCEPT
	{
		return p_;
	}

	operator T* () const NIRVANA_NOEXCEPT
	{
		return p_;
	}

	T& operator * () const NIRVANA_NOEXCEPT
	{
		assert (p_);
		return *p_;
	}

	void reset () NIRVANA_NOEXCEPT
	{
		if (p_) {
			T* tmp = p_;
			p_ = nullptr;
			tmp->_remove_ref ();
		}
	}

private:
	void reset (T* p) NIRVANA_NOEXCEPT
	{
		if (p != p_) {
			T* tmp = p_;
			p_ = p;
			if (tmp)
				tmp->_remove_ref ();
		}
	}

private:
	T* p_;
};

}
}

#endif
