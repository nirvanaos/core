/// \file Stack.h
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

#include "TaggedPtr.h"
#include "AtomicCounter.h"

namespace Nirvana {
namespace Core {

/// \struct StackElem
///
/// \brief A stack elements must be derived from StackElem.
///        While element is not in stack, the StackElem fields are unused and may contain any values.
///        So this fields may be used for other purposes until the element placed to the stack.
struct StackElem
{
	void* next;
	RefCounter ref_cnt;
};

/// Lock-free stack implementation.
template <class T, unsigned ALIGN = CORE_OBJECT_ALIGN (T)>
class Stack
{
	Stack (const Stack&) = delete;
	Stack& operator = (const Stack&) = delete;
public:
	Stack () NIRVANA_NOEXCEPT :
		head_ (nullptr)
	{}

	void push (T& elem) NIRVANA_NOEXCEPT
	{
		StackElem* p = &static_cast <StackElem&> (elem);
		::new (&p->ref_cnt) RefCounter (); // Initialize reference counter with 1
		auto head = head_.load ();
		do
			p->next = head;
		while (!head_.compare_exchange (head, &elem));
	}

	T* pop () NIRVANA_NOEXCEPT
	{
		for (;;) {
			head_.lock ();
			T* p = head_.load ();
			if (p)
				static_cast <StackElem&> (*p).ref_cnt.increment ();
			head_.unlock ();
			if (p) {
				StackElem& node = static_cast <StackElem&> (*p);
				if (head_.cas (p, (T*)node.next))
					node.ref_cnt.decrement ();
				if (!node.ref_cnt.decrement ())
					return p;
			} else
				return nullptr;
		}
	}

private:
	LockablePtrT <T, 0, ALIGN> head_;
};

}
}

#endif
