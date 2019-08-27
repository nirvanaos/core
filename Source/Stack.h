// Lock-free stack implementation
#ifndef NIRVANA_CORE_STACK_H_
#define NIRVANA_CORE_STACK_H_

#include "AtomicPtr.h"
#include "AtomicCounter.h"

namespace Nirvana {
namespace Core {

struct StackElem
{
	void* next;
	RefCounter::UIntType ref_cnt;
};

template <unsigned ALIGN = HEAP_UNIT_CORE>
class Stack
{
	struct Elem
	{
		typedef AtomicPtrT <Elem, ALIGN> Atomic;
		Atomic next;
		RefCounter ref_cnt;

		Elem () :
			next (nullptr)
		{}
	};

public:
	Stack () :
		head_ (nullptr)
	{
	}

	void push (StackElem& node)
	{
		Elem* p = reinterpret_cast <Elem*> (&node);
		new (p) Elem ();
		auto head = head_.load ();
		do
			node.next = head;
		while (!head_.compare_exchange (head, p));
	}

	StackElem* pop ();

private:
	typename Elem::Atomic head_;
};

template <unsigned ALIGN>
StackElem* Stack <ALIGN>::pop ()
{
	for (;;) {
		head_.lock ();
		Elem* p = head_.load ();
		if (p)
			p->ref_cnt.increment ();
		head_.unlock ();
		if (p) {
			if (head_.cas (p, p->next.load ()))
				p->ref_cnt.decrement ();
			if (!p->ref_cnt.decrement ())
				return (StackElem*)p;
		} else
			return nullptr;
	}
}

template <class T>
class StackT :
	public Stack <CORE_OBJECT_ALIGN (T)>
{
public:
	void push (T& node)
	{
		assert ((char*)static_cast <StackElem*> (&node) == (char*)&node); // StackElem must be derived first
		Stack <CORE_OBJECT_ALIGN (T)>::push (static_cast <StackElem&> (node));
	}

	T* pop ()
	{
		return static_cast <T*> (Stack <CORE_OBJECT_ALIGN (T)>::pop ());
	}
};

}
}

#endif
