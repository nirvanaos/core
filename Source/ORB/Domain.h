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
#ifndef NIRVANA_ORB_CORE_DOMAIN_H_
#define NIRVANA_ORB_CORE_DOMAIN_H_
#pragma once

#include <CORBA/CORBA.h>
#include <Nirvana/CoreDomains.h>
#include "../AtomicCounter.h"
#include "../MapUnorderedUnstable.h"
#include "../UserAllocator.h"
#include "../Chrono.h"
#include "HashOctetSeq.h"
#include "GarbageCollector.h"
#include "POA_Root.h"
#include <array>

namespace CORBA {
namespace Core {

class NIRVANA_NOVTABLE Domain : public SyncGC
{
	template <class D> friend class CORBA::servant_reference;

public:
	virtual Internal::IORequest::_ref_type create_request (const IOP::ObjectKey& object_key,
		const Internal::Operation& metadata, unsigned flags) = 0;

	const TimeBase::TimeT& last_ping_time () const NIRVANA_NOEXCEPT
	{
		return last_ping_time_;
	}

	void simple_ping () NIRVANA_NOEXCEPT
	{
		last_ping_time_ = Nirvana::Core::Chrono::steady_clock ();
	}

	void complex_ping (const Nirvana::Core::ObjectKeys& add, const Nirvana::Core::ObjectKeys& del)
	{
		last_ping_time_ = Nirvana::Core::Chrono::steady_clock ();
		static const size_t STATIC_ADD_CNT = 4;
		if (add.size () <= STATIC_ADD_CNT) {
			std::array <Object::_ref_type, STATIC_ADD_CNT> refs;
			PortableServer::Core::POA_Root::get_DGC_objects (add, refs.data ());
			Object::_ref_type* objs = refs.data ();
			for (const IOP::ObjectKey& obj_key : add) {
				if (*objs)
					owned_objects_.emplace (obj_key, std::move (*objs));
				++objs;
			}
		} else {
			std::vector <Object::_ref_type> refs;
			Object::_ref_type* objs = refs.data ();
			for (const IOP::ObjectKey& obj_key : add) {
				owned_objects_.emplace (obj_key, std::move (*objs));
				++objs;
			}
		}

		for (const IOP::ObjectKey& obj_key : del) {
			owned_objects_.erase (obj_key);
		}
	}

	void release_owned_objects () NIRVANA_NOEXCEPT
	{
		owned_objects_.clear ();
	}

protected:
	Domain ();
	~Domain ();

	virtual void _add_ref () NIRVANA_NOEXCEPT override;
	virtual void _remove_ref () NIRVANA_NOEXCEPT override;
	virtual void destroy () NIRVANA_NOEXCEPT = 0;

private:
	class RefCnt : public Nirvana::Core::AtomicCounter <false>
	{
	public:
		RefCnt () :
			Nirvana::Core::AtomicCounter <false> (1)
		{}

		RefCnt (const RefCnt&) = delete;
		RefCnt& operator = (const RefCnt&) = delete;
	};

	RefCnt ref_cnt_;

	// DGC-enabled objects owned by this domain
	typedef Nirvana::Core::MapUnorderedUnstable <IOP::ObjectKey, Object::_ref_type,
		std::hash <IOP::ObjectKey>, std::equal_to <IOP::ObjectKey>, 
		Nirvana::Core::UserAllocator <std::pair <IOP::ObjectKey, Object::_ref_type> > >
		OwnedObjects;

	OwnedObjects owned_objects_;
	TimeBase::TimeT last_ping_time_;
};

}
}

#endif
