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

#include "AtomicCounter.h"
#include "TaggedPtr.h"
#include "SharedObject.h"
#include <assert.h>
#include <Nirvana/throw_exception.h>

namespace Nirvana {
namespace Core {

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
			control_block_->remove_ref ();
	}

	WeakPtr (T* p) :
		control_block_ (new ControlBlock (p))
	{}

	WeakPtr (CoreRef <T>&& ref) :
		control_block_ (new ControlBlock (ref.p_))
	{
		ref.p_ = nullptr;
	}

	WeakPtr (const WeakPtr& src) NIRVANA_NOEXCEPT :
		control_block_ (src.control_block_)
	{
		if (control_block_)
			control_block_->add_ref ();
	}

	WeakPtr (WeakPtr&& src) NIRVANA_NOEXCEPT :
		control_block_ (src.control_block_)
	{
		src.control_block_ = nullptr;
	}

	WeakPtr& operator = (const WeakPtr& src) NIRVANA_NOEXCEPT
	{
		if (control_block_)
			control_block_->remove_ref ();
		if ((control_block_ = src.control_block_))
			control_block_->add_ref ();
		return *this;
	}

	WeakPtr& operator = (WeakPtr&& src) NIRVANA_NOEXCEPT
	{
		if (control_block_)
			control_block_->remove_ref ();
		control_block_ = src.control_block_;
		src.control_block_ = nullptr;
		return *this;
	}

	/// Get strong reference to object.
	CoreRef <T> get () const NIRVANA_NOEXCEPT
	{
		if (control_block_)
			return control_block_->get ();
		else
			return nullptr;
	}

	/// After this call get() for all copies of this weak pointer returns `nullptr` reference.
	void release () NIRVANA_NOEXCEPT
	{
		if (control_block_) {
			control_block_->release ();
			control_block_->remove_ref ();
			control_block_ = nullptr;
		}
	}

	/// Releases the reference to the managed object. After the call `*this` manages no object.
	void reset () NIRVANA_NOEXCEPT
	{
		if (control_block_) {
			control_block_->remove_ref ();
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
	class ControlBlock :
		public SharedObject
	{
	public:
		void add_ref () NIRVANA_NOEXCEPT
		{
			ref_cnt_.increment ();
		}

		void remove_ref () NIRVANA_NOEXCEPT
		{
			if (!ref_cnt_.decrement ())
				delete this;
		}

		ControlBlock (T* p) NIRVANA_NOEXCEPT :
		ptr_ (p)
		{}

		~ControlBlock ()
		{
			reinterpret_cast <CoreRef <T>&> (ptr_).~CoreRef ();
		}

		CoreRef <T> get () NIRVANA_NOEXCEPT
		{
			CoreRef <T> ref (ptr_.lock ());
			ptr_.unlock ();
			return ref;
		}

		void release () NIRVANA_NOEXCEPT
		{
			typedef typename LockablePtrT <T, 0>::Ptr Ptr;
			Ptr p = ptr_.load ();
			if ((T*)p) {
				Ptr nil (nullptr);
				if (ptr_.cas (p, nil))
					reinterpret_cast <CoreRef <T>&> (p).~CoreRef ();
			}
		}

	private:
		LockablePtrT <T, 0> ptr_;
		RefCounter ref_cnt_;
	};

	ControlBlock* control_block_;
};

}
}
#endif
