// Lock-free stack implementation
#ifndef NIRVANA_CORE_STACK_H_
#define NIRVANA_CORE_STACK_H_

#include "AtomicPtr.h"

namespace Nirvana {
namespace Core {

struct StackElem
{
	void* next;
};

template <unsigned ALIGN = HEAP_UNIT_CORE>
class Stack
{
	struct Elem
	{
		typedef AtomicPtrT <Elem, ALIGN> Atomic;
		typedef TaggedPtrT <Elem, ALIGN> Tagged;
		Atomic next;
	};

public:
	Stack ()
	{
		head_.next = nullptr;
	}

	void push (StackElem& node)
	{
		Elem* p = reinterpret_cast <Elem*> (&node);
		auto head = head_.next.load ();
		do
			node.next = head;
		while (!head_.next.compare_exchange (head, p));
	}

	StackElem* pop ();

private:
	Elem head_;
};

template <unsigned ALIGN>
StackElem* Stack <ALIGN>::pop ()
{
	// Try mark the node
	bool marked = false;
	Elem* prev = &head_;
	prev->next.lock ();
	for (;;) {
		Elem* node = prev->next.load ();
		if (node) {
			typename Elem::Tagged next = node->next.load ();
			for (;;) {
				if (next.is_marked ())
					break;
				if (node->next.compare_exchange (next, next.marked ())) {
					marked = true;
					break;
				}
			}
			if (marked)
				break;
			node->next.lock ();
			prev->next.unlock ();
			prev = node;
		}
	}
	prev->next.unlock ();
	if (marked) {
		// Remove the first acceptable marked node
		Elem* prev_prev = nullptr;
		prev = &head_;
		prev->next.lock ();
		Elem* node = prev->next.load ();
		assert (node);
		for (;;) {
			typename Elem::Tagged next = node->next.load ();
			if (next.is_marked ()) {
				// Try to remove
				prev->next.unlock ();
				if (prev->next.cas (node, next.unmarked ()))
					break; // Success
				prev->next.lock ();
			}
			if (prev_prev)
				prev_prev->next.unlock ();
			prev_prev = prev;
			if (!(node = next)) {
				// Restart
				prev_prev->next.unlock ();
				prev_prev = nullptr;
				prev = &head_;
				prev->next.lock ();
				node = prev->next.load ();
			}
		}
		if (prev_prev)
			prev_prev->next.unlock ();

		return reinterpret_cast <StackElem*> (node);
	} else
		return nullptr;
}

template <class T>
class StackT :
	public Stack <std::max ((unsigned)HEAP_UNIT_CORE, (unsigned)(1 << log2_ceil (sizeof (T))))>
{
public:
	void push (T& node)
	{
		assert ((char*)static_cast <StackElem*> (&node) == (char*)&node); // StackElem must be derived first
		Stack <std::max ((unsigned)HEAP_UNIT_CORE, (unsigned)(1 << log2_ceil (sizeof (T))))>::push (static_cast <StackElem&> (node));
	}

	T* pop ()
	{
		return static_cast <T*> (Stack <std::max ((unsigned)HEAP_UNIT_CORE, (unsigned)(1 << log2_ceil (sizeof (T))))>::pop ());
	}
};

}
}

#endif
