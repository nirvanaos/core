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
#include <Nirvana/SimpleList.h>
#include "../AtomicCounter.h"
#include "../MapUnorderedUnstable.h"
#include "../UserAllocator.h"
#include "../Chrono.h"
#include "../Timer.h"
#include "HashOctetSeq.h"
#include "GarbageCollector.h"
#include <array>

namespace CORBA {
namespace Core {

/// Other domain
class NIRVANA_NOVTABLE Domain : public SyncGC
{
	template <class D> friend class CORBA::servant_reference;

public:
	virtual Internal::IORequest::_ref_type create_request (const IOP::ObjectKey& object_key,
		const Internal::Operation& metadata, unsigned flags) = 0;

	bool DGC_supported () const NIRVANA_NOEXCEPT
	{
		return true; // TODO
	}

	const TimeBase::TimeT& latest_request_in_time () const NIRVANA_NOEXCEPT
	{
		return latest_request_in_time_;
	}

	void request_in () NIRVANA_NOEXCEPT
	{
		latest_request_in_time_ = Nirvana::Core::Chrono::steady_clock ();
	}

	void complex_ping (Internal::IORequest::_ptr_type rq)
	{
		IDL::Sequence <IOP::ObjectKey> add, del;
		Internal::Type <IDL::Sequence <IOP::ObjectKey> >::unmarshal (rq, add);
		Internal::Type <IDL::Sequence <IOP::ObjectKey> >::unmarshal (rq, del);
		complex_ping (add, del);
	}

	void release_owned_objects () NIRVANA_NOEXCEPT
	{
		owned_objects_.clear ();
	}

	void on_DGC_reference_unmarshal (const IOP::ObjectKey& object_key)
	{
		auto ins = remote_objects_.emplace (object_key);
		RemoteRefKey& key = const_cast <RemoteRefKey&> (*ins.first);
		if (ins.second)
			remote_objects_add_.push_back (key);
		else if (1 == key.add_ref ())
			key.remove (); // From remote_objects_del_ list
	}

	void on_DGC_reference_delete (const IOP::ObjectKey& object_key) NIRVANA_NOEXCEPT
	{
		auto f = remote_objects_.find (object_key);
		assert (f != remote_objects_.end ());
		RemoteRefKey& key = const_cast <RemoteRefKey&> (*f);
		if (0 == key.remove_ref ())
			remote_objects_del_.push_back (key);
	}

protected:
	Domain ();
	~Domain ();

	virtual void _add_ref () NIRVANA_NOEXCEPT override;
	virtual void _remove_ref () NIRVANA_NOEXCEPT override;
	virtual void destroy () NIRVANA_NOEXCEPT = 0;

	void complex_ping (const IDL::Sequence <IOP::ObjectKey>& add, const IDL::Sequence <IOP::ObjectKey>& del)
	{
		latest_request_in_time_ = Nirvana::Core::Chrono::steady_clock ();
		static const size_t STATIC_ADD_CNT = 4;
		if (add.size () <= STATIC_ADD_CNT) {
			std::array <Object::_ref_type, STATIC_ADD_CNT> refs;
			add_owned_objects (add, refs.data ());
		} else {
			std::vector <Object::_ref_type> refs (add.size ());
			add_owned_objects (add, refs.data ());
		}

		for (const IOP::ObjectKey& obj_key : del) {
			owned_objects_.erase (obj_key);
		}
	}

	void add_owned_objects (const IDL::Sequence <IOP::ObjectKey>& keys, Object::_ref_type* objs);

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

	// DGC-enabled local objects owned by this domain
	typedef Nirvana::Core::MapUnorderedUnstable <IOP::ObjectKey, Object::_ref_type,
		std::hash <IOP::ObjectKey>, std::equal_to <IOP::ObjectKey>, 
		Nirvana::Core::UserAllocator <std::pair <IOP::ObjectKey, Object::_ref_type> > >
		OwnedObjects;

	OwnedObjects owned_objects_;

	// DGC-enabled remote reference owned by the current domain
	class RemoteRefKey :
		public IOP::ObjectKey,
		public Nirvana::SimpleList <RemoteRefKey>::Element
	{
	public:
		RemoteRefKey (const IOP::ObjectKey& object_key) :
			IOP::ObjectKey (object_key),
			ref_cnt_ (1)
		{}

		unsigned add_ref () NIRVANA_NOEXCEPT
		{
			return ++ref_cnt_;
		}

		unsigned remove_ref () NIRVANA_NOEXCEPT
		{
			assert (ref_cnt_);
			return --ref_cnt_;
		}

	private:
		unsigned ref_cnt_;
	};

	typedef Nirvana::Core::SetUnorderedUnstable <RemoteRefKey,
		std::hash <IOP::ObjectKey>, std::equal_to <IOP::ObjectKey>,
		Nirvana::Core::UserAllocator <RemoteRefKey> >
		RemoteObjects;

	RemoteObjects remote_objects_;
	Nirvana::SimpleList <RemoteRefKey> remote_objects_add_;
	Nirvana::SimpleList <RemoteRefKey> remote_objects_del_;

	class Timer : public Nirvana::Core::Timer
	{
	private:
		virtual void signal () noexcept override;
	};

	TimeBase::TimeT latest_request_in_time_;
};

}
}

#endif
