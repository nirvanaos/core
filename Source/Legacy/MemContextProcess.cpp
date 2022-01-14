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
#include "MemContextProcess.h"
#include "../HeapDynamic.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

class HeapDynamic : public Nirvana::Core::HeapDynamic
{
	typedef Nirvana::Core::HeapDynamic Base;
public:
	HeapDynamic (uint16_t allocation_unit, SimpleList <Nirvana::Core::HeapDynamic>& list, std::mutex& m) :
		Base (allocation_unit, list),
		mutex_ (&m)
	{}

	~HeapDynamic ()
	{
		if (mutex_) {
			mutex_->lock ();
			SimpleListElem::remove ();
			mutex_->unlock ();
		}
	}

	std::mutex* mutex_;
};

MemContextProcess::MemContextProcess ()
{}

MemContextProcess::~MemContextProcess ()
{
	for (SimpleList <Nirvana::Core::HeapDynamic>::iterator it = user_heap_list_.begin (); it != user_heap_list_.end (); ++it) {
		static_cast <HeapDynamic&> (*it).mutex_ = nullptr;
	}
}

RuntimeProxy::_ref_type MemContextProcess::runtime_proxy_get (const void* obj)
{ // TODO Replace with own mutex!
	std::lock_guard <std::mutex> lock (mutex_);
	return Base::runtime_proxy_get (obj);
}

void MemContextProcess::runtime_proxy_remove (const void* obj)
{ // TODO Replace with own mutex!
	std::lock_guard <std::mutex> lock (mutex_);
	Base::runtime_proxy_remove (obj);
}

Memory::_ref_type MemContextProcess::create_heap (uint16_t granularity)
{
	std::lock_guard <std::mutex> lock (mutex_);
	return CORBA::make_pseudo <HeapDynamic> (granularity, std::ref (user_heap_list_), std::ref (mutex_));
}

}
}
}
