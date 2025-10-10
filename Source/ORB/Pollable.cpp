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
#include "Pollable.h"
#include "ServantProxyLocal.h"

using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

Pollable::Pollable () :
	cur_set_ (nullptr),
	ref_cnt_ (1),
	ready_ (false)
{
	SyncDomain* cur_sd = SyncContext::current ().sync_domain ();
	if (cur_sd)
		sync_domain_ = cur_sd;
	else {
		Heap& heap = Heap::user_heap ();
		sync_domain_ = SyncDomainDyn <SyncDomainCore>::create (heap, std::ref (heap));
	}
}

Pollable::~Pollable ()
{
	assert (&SyncContext::current () == sync_domain_);
}

void Pollable::_add_ref () noexcept
{
	ref_cnt_.increment ();
}

void Pollable::_remove_ref () noexcept
{
	if (0 == ref_cnt_.decrement ()) {
		SyncContext& sc = SyncContext::current ();
		if (&sc == sync_domain_)
			delete this;
		else
			GarbageCollector::schedule (*this, *sync_domain_);
	}
}

bool Pollable::is_ready (ULong timeout)
{
	if (ready_)
		return true;
	else if (!timeout)
		return false;
	else {
		bool ret;
		SYNC_BEGIN (*sync_domain_, nullptr)
			ret = event_.wait (ms2time (timeout), &_sync_frame);
		SYNC_END ();
		return ret;
	}
}

void Pollable::on_complete (Internal::IORequest::_ptr_type reply)
{
	// Called in synchronization domain
	event_.signal_all ();
	ready_ = true;
	if (cur_set_)
		cur_set_->pollable_ready ();
}

TimeBase::TimeT Pollable::ms2time (uint32_t ms) noexcept
{
	switch (ms) {
		case 0:
			return 0;

		case std::numeric_limits <uint32_t>::max ():
			return std::numeric_limits <TimeBase::TimeT>::max ();

		default:
			return (TimeBase::TimeT)ms * TimeBase::MILLISECOND;
	}
}

inline
void PollableSet::pollable_ready ()
{
	SYNC_BEGIN (local2proxy (_this ())->sync_context (), nullptr)
		event_.signal_one ();
	SYNC_END ()
}

}
}
