/// \file
/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
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
#ifndef NIRVANA_CORE_USERALLOCATOR_H_
#define NIRVANA_CORE_USERALLOCATOR_H_

#include "user_memory.h"

namespace Nirvana {
namespace Core {

template <class T>
class UserAllocator :
	public std::allocator <T>
{
public:
	static void deallocate (T* p, size_t cnt)
	{
		user_memory ().release (p, cnt * sizeof (T));
	}

	static T* allocate (size_t cnt, void* hint = nullptr, UWord flags = 0)
	{
		return (T*)user_memory ().allocate (hint, cnt * sizeof (T), flags);
	}
};

}
}

#endif
