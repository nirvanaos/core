/// \file Stack.h
/// Lock-free stack implementation
#ifndef NIRVANA_CORE_STACK_H_
#define NIRVANA_CORE_STACK_H_

#include "TaggedPtr.h"
#include "AtomicCounter.h"

namespace Nirvana {
namespace Core {

/// \struct StackElem
///
/// \brief A stack elements must be derived from StackElem.
///        While element not in stack, the StackElem fields are unused and may contain any values.
///        So this fields may be used for other purposes until the element placed to the stack.

struct StackElem
{
	void* next;
	RefCounter ref_cnt;
};

template <class T, unsigned ALIGN = CORE_OBJECT_ALIGN (T)>
class Stack
{
public:
	Stack () :
		head_ (nullptr)
	{}

	void push (T& elem)
	{
		StackElem* p = &static_cast <StackElem&> (elem);
		::new (&p->ref_cnt) RefCounter (); // Initialize reference counter with 1
		auto head = head_.load ();
		do
			p->next = head;
		while (!head_.compare_exchange (head, &elem));
	}

	T* pop ()
	{
		for (;;) {
			head_.lock ();
			auto p = head_.load ();
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
