#ifndef NIRVANA_CORE_SKIPLISTWITHPOOL_H_
#define NIRVANA_CORE_SKIPLISTWITHPOOL_H_

#include "SkipList.h"
#include "Stack.h"
#include "AtomicCounter.h"

namespace Nirvana {
namespace Core {

template <class Base>
class SkipListWithPool : public Base
{
	static_assert (sizeof (Base::Value) >= sizeof (StackElem), "sizeof (Base::Value) >= sizeof (StackElem)");

	struct Stackable : StackElem
	{
		unsigned level;
	};

public:
	SkipListWithPool (AtomicCounter::UIntType initial_count) :
		purge_count_ (0)
	{
		while (initial_count--)
			push_node ();
	}

	~SkipListWithPool ()
	{
		while (pop_node ());
	}

	void create_item ()
	{
		push_node ();
	}

	void delete_item ()
	{
		if (!pop_node ())
			purge_count_.increment ();
	}

private:
	void push_node ()
	{
		stack_.push (*real_allocate_node ());
	}

	bool pop_node () NIRVANA_NOEXCEPT
	{
		Stackable* se = stack_.pop ();
		if (se) {
			item_count_cur_.decrement ();
			unsigned level = se->level;
			SkipListBase::delete_node (new (se) SkipListBase::Node (level));
			return true;
		}
		return false;
	}

	Stackable* real_allocate_node ()
	{
		SkipListBase::NodeBase* nb = Base::allocate_node ();
		Stackable* se = (Stackable*)nb
		se->level = nb->level;
		item_count_cur_.increment ();
		return se;
	}

	virtual SkipListBase::NodeBase* allocate_node ()
	{
		Stackable* se = stack_.pop ();
		if (!se) {
			assert (false); // Hardly ever, but may be.
			se = real_allocate_node ();
			// This increases the minimum pool size and helps to avoid this situation in the future.
		}
		SkipListBase::NodeBase* nb = (SkipListBase::NodeBase*)se;
		nb->level = se->level;
		return nb;
	}

	virtual void deallocate_node (SkipListBase::NodeBase* node) NIRVANA_NOEXCEPT
	{
		if (purge_count_.decrement () >= 0)
			SkipListBase::deallocate_node (node);
		else {
			purge_count_.increment ();
			Stackable* se = (Stackable*)node;
			se->level = node->level;
			stack_.push (*se);
		}
	}

	void push_node (Stackable& se) NIRVANA_NOEXCEPT
	{
		Stackable* se = reinterpret_cast <Stackable*> (node);
		se->level = level;
		stack_.push (*se);
	}

private:
	Stack <Stackable> stack_;
	AtomicCounter <true> purge_count_;
};

}
}

#endif
