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
#include <stack>

namespace Nirvana {
namespace Core {

class MemContext;

class AtExitAsync
{
public:
	AtExitAsync () noexcept :
		last_ (nullptr)
	{}

	void atexit (Heap& heap, AtExitFunc f);
	void execute () noexcept;

private:
	struct Entry
	{
		Entry* prev;
		Ref <MemContext> mem_context;
		AtExitFunc func;
	};

private:
	std::atomic <Entry*> last_;
};

template <template <class> class Allocator>
class AtExitSync
{
public:
	void atexit (AtExitFunc f)
	{
		entries_.push (f);
	}

	void execute () noexcept
	{
		while (!entries_.empty ()) {
			try {
				(*entries_.top ()) ();
			} catch (...) {
			}
			entries_.pop ();
		}
	}

private:
	std::stack <AtExitFunc, std::deque <AtExitFunc, Allocator <AtExitFunc> > > entries_;
};

}
}

#endif
