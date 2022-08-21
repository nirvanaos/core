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
#ifndef NIRVANA_CORE_SKIPLISTWITHPOOL_H_
#define NIRVANA_CORE_SKIPLISTWITHPOOL_H_
#pragma once

#include "SkipList.h"
#include "Stack.h"
#include "AtomicCounter.h"

namespace Nirvana {
namespace Core {

template <class Base>
class SkipListWithPool : public Base
{
	static_assert (sizeof (typename Base::Value) >= sizeof (StackElem), "sizeof (Base::Value) >= sizeof (StackElem)");

	struct Stackable : StackElem
	{
		unsigned level;
	};

public:
	SkipListWithPool (AtomicCounter <false>::IntegralType initial_count) :
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
			SkipListBase::Node* node = (SkipListBase::Node*)se;
			node->level = se->level;
			SkipListBase::deallocate_node (node);
			return true;
		}
		return false;
	}

	Stackable* real_allocate_node ()
	{
		SkipListBase::Node* node = Base::allocate_node ();
		Stackable* se = (Stackable*)node;
		se->level = node->level;
		return se;
	}

	virtual SkipListBase::Node* allocate_node ()
	{
		Stackable* se = stack_.pop ();
		if (!se) {
			assert (false); // Hardly ever, but may be.
			se = real_allocate_node ();
			// This increases the minimum pool size and helps to avoid this situation in the future.
		}
		SkipListBase::Node* node = (SkipListBase::Node*)se;
		node->level = se->level;
		return node;
	}

	virtual void deallocate_node (SkipListBase::Node* node) NIRVANA_NOEXCEPT
	{
		if (purge_count_.decrement_seq () >= 0)
			SkipListBase::deallocate_node (node);
		else {
			purge_count_.increment ();
			Stackable* se = (Stackable*)node;
			se->level = node->level;
			stack_.push (*se);
		}
	}

private:
	Stack <Stackable, core_object_align <typename Base::NodeVal> ()> stack_;
	AtomicCounter <true> purge_count_;
};

}
}

#endif
