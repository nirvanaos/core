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
#ifndef NIRVANA_CORE_THREAD_INL_
#define NIRVANA_CORE_THREAD_INL_

#include "Thread.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

inline
void Thread::exec_domain (ExecDomain& exec_domain) NIRVANA_NOEXCEPT
{
	exec_domain_ = &exec_domain;
	SyncDomain* sd = exec_domain.sync_context ()->sync_domain ();
	if (sd)
		runtime_support_ = &sd->runtime_support ();
	else
		runtime_support_ = &exec_domain.runtime_support ();
}

template <class T> inline
void ExecDomain::Allocator <T>::deallocate (T* p, size_t cnt)
{
	Thread::current ().exec_domain ()->heap_.release (p, cnt * sizeof (T));
}

template <class T> inline
T* ExecDomain::Allocator <T>::allocate (size_t cnt, void* hint, unsigned flags)
{
	return (T*)Thread::current ().exec_domain ()->heap_.allocate (hint, cnt * sizeof (T), flags);
}

}
}

#endif
