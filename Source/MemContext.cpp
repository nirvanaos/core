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
#include "MemContextCore.h"
#include "MemContextUser.h"
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

MemContext::MemContext () noexcept :
	deadline_policy_async_ (0),
	deadline_policy_oneway_ (INFINITE_DEADLINE)
{}

MemContext::~MemContext ()
{}

void MemContext::_remove_ref () noexcept
{
	if (!ref_cnt_.decrement ()) {
		
		Ref <Heap> heap (heap_);
		Type type = type_;

		ExecDomain& ed = ExecDomain::current ();
		ed.mem_context_replace (*this);
		if (MC_CORE == type)
			static_cast <MemContextCore*> (this)->~MemContextCore ();
		else
			static_cast <MemContextUser*> (this)->~MemContextUser ();
		ed.mem_context_restore ();

		switch (type_) {
			case MC_CORE:
				heap->release (this, sizeof (MemContextCore));
				break;
			case MC_USER:
				heap->release (this, sizeof (MemContextUser));
				break;
			default:
				BinderMemory::heap ().release (this, sizeof (MemContextUser));
		}
	}
}

}
}
