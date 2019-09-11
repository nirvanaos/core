// Lock-free stack implementation
#ifndef NIRVANA_CORE_STACK_H_
#define NIRVANA_CORE_STACK_H_

#include "TaggedPtr.h"
#include "AtomicCounter.h"

namespace Nirvana {
namespace Core {

//! \struct StackElem
//!
//! \brief A stack elements must be derived from StackElem.
//!        While element not in stack, the StackElem fields are unused and may contain any values.
//!        So this fields may be used for other purposes until the element placed to the stack.

struct StackElem
{
	void* next;
	RefCounter::UIntType ref_cnt;
};

template <class T, unsigned ALIGN = CORE_OBJECT_ALIGN (T)>
class Stack
{
	struct Node
	{
		T* next;
		RefCounter ref_cnt;
	};

public:
	Stack () :
		head_ (nullptr)
	{}

	void push (T& elem)
	{
		Node* p = &get_node (elem);
		new (p) Node ();
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
				get_node (*p).ref_cnt.increment ();
			head_.unlock ();
			if (p) {
				Node& node = get_node (*p);
				if (head_.cas (p, node.next))
					node.ref_cnt.decrement ();
				if (!node.ref_cnt.decrement ())
					return p;
			} else
				return nullptr;
		}
	}

private:
	static Node& get_node (T& elem)
	{
		return reinterpret_cast <Node&> (static_cast <StackElem&> (elem));
	}

private:
	LockablePtrT <T, 0, ALIGN> head_;
};

}
}

#endif
