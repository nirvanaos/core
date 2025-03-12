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
#ifndef NIRVANA_CORE_STACK_H_
#define NIRVANA_CORE_STACK_H_
#pragma once

#include "TaggedPtr.h"
#include "AtomicCounter.h"

namespace Nirvana {
namespace Core {

/// A stack elements must be derived from StackElem.
/// While element is not in stack, the StackElem fields are unused and may contain any values.
/// So this fields may be used for other purposes until the element placed to the stack.
struct StackElem
{
	void* next;
	AtomicCounter <false> ref_cnt;
};

struct StackBaseDummy
{
	static void bottom () noexcept
	{}
};

/// Lock-free stack implementation.
template <class T, unsigned ALIGN = core_object_align (sizeof (T)), class Base = StackBaseDummy>
class Stack : public Base
{
	Stack (const Stack&) = delete;
	Stack& operator = (const Stack&) = delete;

protected:
	typedef LockablePtrT <T, 0, ALIGN> LockablePtr;
	typedef typename LockablePtr::Ptr Ptr;

public:
	Stack () noexcept :
		head_ (nullptr)
	{}

	/// Push object to the stack.
	/// 
	/// \param elem Object.
	void push (T& elem) noexcept
	{
		StackElem* node = &static_cast <StackElem&> (elem);
		// Initialize ref_cnt with 1.
		::new (&(node->ref_cnt)) AtomicCounter <false> (1);
		// While element is attached to the stack, ref_cnt is always > 0.

		// Push element to the stack
		Ptr ptr = &elem;
		Ptr head = head_.load ();
		do
			node->next = head;
		while (!head_.compare_exchange (head, ptr));
	}

	/// Pop object from the stack.
	/// 
	/// \returns The object pointer or `nullptr` if the stack is empty.
	T* pop (bool* last = nullptr) noexcept
	{
		for (;;) {
			// Get head and increment counter on it
			Ptr head = head_.lock ();
			if ((T*)head) {
				StackElem* node = static_cast <StackElem*> ((T*)head);
				node->ref_cnt.increment ();
				head_.unlock ();
				T* next = (T*)(node->next);
				if (head_.cas (head, next)) {
					// Node was detached from stack so we decrement the counter
					if (!next)
						Base::bottom ();
					node->ref_cnt.decrement ();
				}

				// First thread that decrement counter to zero will return detached node
				if (!node->ref_cnt.decrement_seq ())
					return head;
			} else {
				head_.unlock ();
				return nullptr;
			}
		}
	}

private:
	LockablePtrT <T, 0, ALIGN> head_;
};

}
}

#endif
