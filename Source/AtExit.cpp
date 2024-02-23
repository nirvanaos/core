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
#include "AtExit.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

void AtExitSync::atexit (AtExitFunc f)
{
	entries_.push_back (f);
}

void AtExitSync::execute ()
{
	for (auto it = entries_.rbegin (); it != entries_.rend (); ++it) {
		try {
			(*it) ();
		} catch (...) {
		}
	}
}

void AtExitAsync::atexit (Heap& heap, AtExitFunc f)
{
	size_t size = sizeof (Entry);
	Entry* entry = (Entry*)heap.allocate (nullptr, size, 0);
	entry->func = f;
	entry->prev = last_.load ();
	entry->mem_context = &MemContext::current ();
	for (BackOff bo; true; bo ()) {
		if (last_.compare_exchange_weak (entry->prev, entry))
			break;
	}
}

void AtExitAsync::execute ()
{
	Entry* entry = last_.load ();
	if (entry) {
		ExecDomain& ed = ExecDomain::current ();
		Heap& heap = ed.mem_context_ptr ()->heap ();
		do {
			ed.mem_context_replace (*entry->mem_context);
			try {
				(entry->func) ();
			} catch (...) {
			}
			ed.mem_context_restore ();
			entry->mem_context = nullptr;
			Entry* prev = entry->prev;
			heap.release (entry, sizeof (Entry));
			entry = prev;
		} while (entry);
	}
}

}
}
