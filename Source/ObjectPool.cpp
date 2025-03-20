/// \file
/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2025 Igor Popov.
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
#include "ObjectPool.h"

namespace Nirvana {
namespace Core {

ObjectPoolBase* ObjectPoolBase::pool_list_;
ObjectPoolBase::Timer* ObjectPoolBase::timer_;

ObjectPoolBase::ObjectPoolBase (unsigned min_size) noexcept :
	next_ (pool_list_),
	min_size_ (min_size),
	shrink_ ATOMIC_FLAG_INIT
{
	pool_list_ = this;
}

void ObjectPoolBase::on_pop () noexcept
{
	if (cur_size_.decrement_seq () <= min_size_)
		shrink_.clear ();
}

void ObjectPoolBase::Timer::run (const TimeBase::TimeT&) noexcept
{
	for (ObjectPoolBase* p = pool_list_; p; p = p->next_) {
		p->shrink ();
	}
}

}
}
