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
#ifndef NIRVANA_CORE_ATEXIT_H_
#define NIRVANA_CORE_ATEXIT_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include "Heap.h"
#include <atomic>

namespace Nirvana {
namespace Core {

class AtExit
{
public:
	AtExit () :
		last_ (nullptr)
	{}

	void atexit (Heap& heap, AtExitFunc f)
	{
		size_t size = sizeof (Entry);
		Entry* entry = (Entry*)heap.allocate (nullptr, size, 0);
		entry->func = f;
		entry->prev = last_.load ();
		while (!last_.compare_exchange_weak (entry->prev, entry))
			;
	}

	void execute (Heap& heap)
	{
		for (Entry* entry = last_.load (); entry;) {
			Entry* prev = entry->prev;
			heap.release (entry, sizeof (Entry));
			entry = prev;
		}
	}

private:
	struct Entry
	{
		Entry* prev;
		AtExitFunc func;
	};

private:
	std::atomic <Entry*> last_;
};

}
}

#endif