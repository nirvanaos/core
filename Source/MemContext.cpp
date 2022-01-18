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
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

StaticallyAllocated <ImplStatic <MemContext>> g_shared_mem_context;

MemContext& MemContext::current ()
{
	Thread* th = Thread::current_ptr ();
	if (th) {
		ExecDomain* ed = th->exec_domain ();
		if (ed)
			return ed->mem_context ();
	}
	return g_shared_mem_context;
}

bool MemContext::is_current (MemContext* context)
{
	Thread* th = Thread::current_ptr ();
	if (th) {
		ExecDomain* ed = th->exec_domain ();
		if (ed)
			return ed->mem_context_ptr () == context;
	}
	return &g_shared_mem_context == context;
}

MemContext::MemContext ()
{}

MemContext::~MemContext ()
{}

RuntimeProxy::_ref_type MemContext::runtime_proxy_get (const void* obj)
{
	return nullptr;
}

void MemContext::runtime_proxy_remove (const void* obj)
{
}

void MemContext::on_object_construct (MemContextObject& obj)
{}

void MemContext::on_object_destruct (MemContextObject& obj)
{}

}
}
