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
#include "pch.h"
#include "MemContext.h"
#include "ExecDomain.h"
#include "BinderMemory.h"

namespace Nirvana {
namespace Core {

bool MemContext::is_current (const MemContext* context) noexcept
{
	Thread* th = Thread::current_ptr ();
	if (th) {
		ExecDomain* ed = th->exec_domain ();
		if (ed)
			return ed->mem_context_ptr () == context;
	}
	return !context;
}

Ref <Heap> MemContext::create_heap ()
{
	return sizeof (void*) > 2 ? Ref <Heap>::create <ImplDynamic <HeapUser> > () :
		Ref <Heap> (&Heap::shared_heap ());
}

Ref <MemContext> MemContext::create (Ref <Heap>&& heap, bool class_library_init)
{
	Heap& allocate_from = class_library_init ? BinderMemory::heap () : *heap;
	size_t cb = sizeof (MemContext);
	return CreateRef (new (allocate_from.allocate (nullptr, cb, 0)) MemContext (std::move (heap), class_library_init));
}

MemContext& MemContext::current ()
{
	return ExecDomain::current ().mem_context ();
}

MemContext* MemContext::current_ptr () noexcept
{
	return ExecDomain::current ().mem_context_ptr ();
}

inline
MemContext::MemContext (Ref <Heap>&& heap, bool class_library_init) noexcept :
	heap_ (std::move (heap)),
	ref_cnt_ (1),
	class_library_init_ (class_library_init)
{}

inline
MemContext::~MemContext ()
{}

class MemContext::Replacer
{
public:
	Replacer (MemContext& mc) :
		exec_domain_ (ExecDomain::current ())
	{
		// Increment reference counter to prevent recursive deletion
		mc.ref_cnt_.increment ();
		if (exec_domain_.mem_context_stack_empty ()) {
			// If context stack is empty, mem_context_push definitely won't throw an exception.
			pop_ = true;
			exec_domain_.mem_context_push (&mc);
		} else {
			pop_ = false;
			ref_ = &mc;
			exec_domain_.mem_context_swap (ref_);
		}
	}

	~Replacer ()
	{
		if (pop_)
			exec_domain_.mem_context_pop ();
		else
			exec_domain_.mem_context_swap (ref_);
	}

private:
	ExecDomain& exec_domain_;
	Ref <MemContext> ref_;
	bool pop_;
};

void MemContext::_remove_ref () noexcept
{
	if (!ref_cnt_.decrement_seq ()) {

		// Hold heap reference
		Ref <Heap> heap;
		if (class_library_init_)
			heap = &BinderMemory::heap ();
		else
			heap = heap_;

		{
			// Replace memory context and call destructor
			Replacer replace (*this);
			this->~MemContext ();
		}

		// Release memory
		heap->release (this, sizeof (MemContext));
	}
}

}
}
