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
#include "../pch.h"
#include "Thread.h"
#include "Process.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

Thread::~Thread ()
{}

void Thread::_add_ref () noexcept
{
	ref_cnt_.increment ();
}

void Thread::_remove_ref () noexcept
{
	if (!ref_cnt_.decrement ())
		process_.delete_thread (*this);
}

void Thread::run ()
{
	runnable_->run ();
	runnable_ = nullptr;
	event_.signal ();
}

void Thread::on_crash (const siginfo& signal) noexcept
{
	process_.on_thread_crash (*this, signal);
}

}
}
}
