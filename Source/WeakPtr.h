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
#ifndef NIRVANA_CORE_WEAKPTR_H_
#define NIRVANA_CORE_WEAKPTR_H_
#pragma once

#include "CoreInterface.h"
#include "SharedObject.h"

namespace Nirvana {
namespace Core {

struct ControlBlock
{
	AtomicCounter <false> ref_cnt_, ref_cnt_weak_;
};

/// Shared implementation of a core object.
/// \tparam T object class.
template <class T>
class ImplShared final :
	public T,
	public SharedObject
{
protected:
	template <class> friend class CoreRef;

	template <class ... Args>
	ImplShared (Args ... args) :
		T (std::forward <Args> (args)...),
		ref_cnt_weak_ (0)
	{}

	~ImplShared ()
	{}

	void _add_ref () NIRVANA_NOEXCEPT
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () NIRVANA_NOEXCEPT
	{
		if (!ref_cnt_.decrement ()) {
			if (0 == ref_cnt_weak_)
				delete this;
			else {
				static_cast <T*> (this)->~T ();
				size_t cb_rel = round_down (sizeof (*this), HEAP_UNIT_DEFAULT);
				if (cb_rel)
					g_shared_mem_context->heap ().release (this, cb_rel);
			}
		}
	}

	void _add_ref_weak ()
	{
		ref_cnt_weak_.increment ();
	}

	void _remove_ref_weak ()
	{
		if (!ref_cnt_weak_.decrement_seq ()) {
			if (ref_cnt_.is_zero ())
				g_shared_mem_context->heap ().release ((uint8_t*)this + sizeof (T), sizeof (*this) - sizeof (T));
		}
	}

	CoreRef <ImplShared <T> > _get_ref ()
	{
		CoreRef <ImplShared <T> > ret;
		if (!ref_cnt_.is_zero ())
			ret = this;
		return ret;
	}

private:
	RefCounter ref_cnt_;
	AtomicCounter <false> ref_cnt_weak_;
};

/// Weak object reference.
/// 
/// \typeparam T The object type.
template <class T>
class WeakPtr
{
public:
	WeakPtr () NIRVANA_NOEXCEPT :
		control_block_ (nullptr)
	{}

	~WeakPtr ()
	{
		if (control_block_)
			control_block_->_remove_ref_weak ();
	}

	WeakPtr (CoreRef <ImplShared <T> > p) :
		control_block_ (p)
	{
		if (p)
			p->_add_ref_weak ();
	}

	WeakPtr (const WeakPtr& src) NIRVANA_NOEXCEPT :
		control_block_ (src.control_block_)
	{
		if (control_block_)
			control_block_->_add_ref_weak ();
	}

	WeakPtr (WeakPtr&& src) NIRVANA_NOEXCEPT :
		control_block_ (src.control_block_)
	{
		src.control_block_ = nullptr;
	}

	WeakPtr& operator = (const WeakPtr& src) NIRVANA_NOEXCEPT
	{
		if (control_block_ != src.control_block_) {
			if (control_block_)
				control_block_->_remove_ref_weak ();
			if ((control_block_ = src.control_block_))
				control_block_->_add_ref_weak ();
		}
		return *this;
	}

	WeakPtr& operator = (WeakPtr&& src) NIRVANA_NOEXCEPT
	{
		if (control_block_)
			control_block_->_remove_ref_weak ();
		control_block_ = src.control_block_;
		src.control_block_ = nullptr;
		return *this;
	}

	/// Get strong reference to object.
	CoreRef <ImplShared <T> > lock () const NIRVANA_NOEXCEPT
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
