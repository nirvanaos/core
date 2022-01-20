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
#ifndef NIRVANA_CORE_OBJECTPOOL_H_
#define NIRVANA_CORE_OBJECTPOOL_H_
#pragma once

#include "Stack.h"
#include "CoreInterface.h"

namespace Nirvana {
namespace Core {

template <class T>
class ObjectPool
{
public:
	ObjectPool (unsigned max_size) :
		max_size_ (max_size)
	{}

	~ObjectPool ()
	{
		while (T* obj = stack_.pop ())
			delete obj;
	}

	CoreRef <T> create ()
	{
		T* obj = stack_.pop ();
		if (obj) {
			--size_;
			return obj;
		} else
			return CoreRef <T>::template create <T> ();
	}

	void release (T& obj) NIRVANA_NOEXCEPT
	{
		if (max_size_ < ++size_) {
			--size_;
			delete& obj;
		} else
			stack_.push (obj);
	}

private:
	Stack <T> stack_;
	unsigned max_size_;
	std::atomic <unsigned> size_;
};

}
}

#endif
