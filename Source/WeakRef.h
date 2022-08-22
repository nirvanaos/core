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
#ifndef NIRVANA_CORE_WEAKREF_H_
#define NIRVANA_CORE_WEAKREF_H_
#pragma once

#include "CoreInterface.h"
#include "SharedObject.h"

namespace Nirvana {
namespace Core {

template <class T> class WeakRef;

/// Shared implementation of a core object.
/// \tparam T object class.
template <class T>
class ImplShared final :
	public T,
	public SharedObject
{
private:
	template <class> friend class CoreRef;

	template <class ... Args>
	ImplShared (Args ... args) :
		T (std::forward <Args> (args)...)
	{}

	void _add_ref () NIRVANA_NOEXCEPT
	{
		control_block_.ref_cnt.increment ();
	}

	void _remove_ref () NIRVANA_NOEXCEPT
	{
		if (!control_block_.ref_cnt.decrement ()) {
			if (control_block_.ref_cnt_weak.is_zero ())
				delete this;
			else {
				static_cast <T*> (this)->~T ();
				size_t cb_rel = sizeof (T) / HEAP_UNIT_DEFAULT * HEAP_UNIT_DEFAULT;
				if (cb_rel)
					g_shared_mem_context->heap ().release (this, cb_rel);
			}
		}
	}

	friend class WeakRef <T>;

	void _add_ref_weak () NIRVANA_NOEXCEPT
	{
		control_block_.ref_cnt_weak.increment ();
	}

	void _remove_ref_weak () NIRVANA_NOEXCEPT
	{
		if (!control_block_.ref_cnt_weak.decrement_seq ()) {
			if (control_block_.ref_cnt.is_zero ())
				g_shared_mem_context->heap ().release ((uint8_t*)this + sizeof (T), sizeof (*this) - sizeof (T));
		}
	}

	CoreRef <ImplShared <T> > _get_ref () NIRVANA_NOEXCEPT
	{
		CoreRef <ImplShared <T> > ret;
		if (!control_block_.ref_cnt.is_zero ())
			ret = this;
		return ret;
	}

private:
	struct ControlBlock
	{
		RefCounter ref_cnt;
		AtomicCounter <false> ref_cnt_weak;

		ControlBlock () :
			ref_cnt_weak (0)
		{}

		ControlBlock (const ControlBlock&) :
			ref_cnt_weak (0)
		{}

		ControlBlock (ControlBlock&&) :
			ref_cnt_weak (0)
		{}

		ControlBlock& operator = (const ControlBlock&)
		{
			return *this;
		}

		ControlBlock& operator = (ControlBlock&&)
		{
			return *this;
		}
	}
	control_block_;
};

/// Shared object reference.
/// 
/// \typeparam T The object type.
template <class T>
using SharedRef = CoreRef <ImplShared <T> >;

/// Weak object reference.
/// 
/// \typeparam T The object type.
template <class T>
class WeakRef
{
public:
	WeakRef () NIRVANA_NOEXCEPT :
		control_block_ (nullptr)
	{}

	~WeakRef ()
	{
		if (control_block_)
			control_block_->_remove_ref_weak ();
	}

	WeakRef (SharedRef <T> p) :
		control_block_ (p)
	{
		if (p)
			p->_add_ref_weak ();
	}

	WeakRef (const WeakRef& src) NIRVANA_NOEXCEPT :
		control_block_ (src.control_block_)
	{
		if (control_block_)
			control_block_->_add_ref_weak ();
	}

	WeakRef (WeakRef&& src) NIRVANA_NOEXCEPT :
		control_block_ (src.control_block_)
	{
		src.control_block_ = nullptr;
	}

	WeakRef& operator = (const WeakRef& src) NIRVANA_NOEXCEPT
	{
		if (control_block_ != src.control_block_) {
			if (control_block_)
				control_block_->_remove_ref_weak ();
			if ((control_block_ = src.control_block_))
				control_block_->_add_ref_weak ();
		}
		return *this;
	}

	WeakRef& operator = (WeakRef&& src) NIRVANA_NOEXCEPT
	{
		if (control_block_)
			control_block_->_remove_ref_weak ();
		control_block_ = src.control_block_;
		src.control_block_ = nullptr;
		return *this;
	}

	/// Get strong reference to object.
	SharedRef <T> lock () const NIRVANA_NOEXCEPT
	{
		if (control_block_)
			return control_block_->_get_ref ();
		else
			return nullptr;
	}

	/// Releases the reference to the managed object. After the call `*this` manages no object.
	void reset () NIRVANA_NOEXCEPT
	{
		if (control_block_) {
			control_block_->_remove_ref_weak ();
			control_block_ = nullptr;
		}
	}

	bool operator ! () const NIRVANA_NOEXCEPT
	{
		return !control_block_;
	}

	operator bool () const NIRVANA_NOEXCEPT
	{
		return control_block_ != nullptr;
	}

private:
	ImplShared <T>* control_block_;
};

}
}

#endif
