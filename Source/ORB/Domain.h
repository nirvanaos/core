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
#include <CORBA/IOP.h>
#include <CORBA/Proxy/InterfaceMetadata.h>
#include <Nirvana/SimpleList.h>
#include "../AtomicCounter.h"
#include "../MapUnorderedUnstable.h"
#include "../MapUnorderedStable.h"
#include "../UserAllocator.h"
#include "../Chrono.h"
#include "HashOctetSeq.h"
#include "ReferenceLocal.h"
#include <array>

namespace CORBA {
namespace Core {

class ReferenceRemote;
typedef servant_reference <ReferenceRemote> ReferenceRemoteRef;

/// Other domain
class NIRVANA_NOVTABLE Domain : public SyncGC
{
	template <class D> friend class CORBA::servant_reference;

public:
	virtual Internal::IORequest::_ref_type create_request (const IOP::ObjectKey& object_key,
		const Internal::Operation& metadata, unsigned response_flags) = 0;

	/// Domain flag bits
	enum
	{
		/// Domain supports distributed garbage collection
		///
		/// If this flag is set, flags HEARTBEAT_IN and HEARTBEAT_OUT are set also
		GARBAGE_COLLECTION = 0x0001,

		/// Domain requires FT heartbeat ping
		HEARTBEAT_IN = 0x0002,

		/// Domain sends FT heartbeat ping
		HEARTBEAT_OUT = 0x0002
	};

	unsigned flags () const NIRVANA_NOEXCEPT
	{
		return flags_;
	}

	const TimeBase::TimeT& request_latency () const NIRVANA_NOEXCEPT
	{
		return request_latency_;
	}

	void simple_ping () NIRVANA_NOEXCEPT
	{
		last_ping_in_time_ = Nirvana::Core::Chrono::steady_clock ();
	}

	void complex_ping (Internal::IORequest::_ptr_type rq)
	{
		last_ping_in_time_ = Nirvana::Core::Chrono::steady_clock ();
		IDL::Sequence <IOP::ObjectKey> add, del;
		Internal::Type <IDL::Sequence <IOP::ObjectKey> >::unmarshal (rq, add);
		Internal::Type <IDL::Sequence <IOP::ObjectKey> >::unmarshal (rq, del);
		rq->unmarshal_end ();
		complex_ping (add, del);
	}

	void release_owned_objects () NIRVANA_NOEXCEPT
	{
		local_objects_.clear ();
	}

	// DGC-enabled remote reference owned by the current domain
	class RemoteRefKey :
		public IOP::ObjectKey,
		public Nirvana::SimpleList <RemoteRefKey>::Element
	{
	public:
		enum State
		{
			STATE_NEW, // Reference just unmarshalled.
			STATE_ADDITION, // Add request in progress. The request_ field contains reference to the request.
			STATE_CONFIRMED, // The add request completed successfully.
			STATE_DELETION // Delete request in progress. The request_ field contains reference to the request.
		};

		RemoteRefKey (const IOP::ObjectKey& object_key) :
			IOP::ObjectKey (object_key),
			ref_cnt_ (1),
			state_ (STATE_NEW)
		{
		}

		// Add remote reference with this key
		void reference_add () NIRVANA_NOEXCEPT
		{
			if (1 == ++ref_cnt_ && STATE_DELETION == state_) {
				Internal::IORequest::_ref_type rq = std::move (request_);
				rq->wait (std::numeric_limits <uint64_t>::max ());
				state_ = STATE_NEW;
			}
		}

		// Remove remote reference with this key
		// Returns true if reference must be deleted
		bool reference_remove () NIRVANA_NOEXCEPT
		{
			if (!--ref_cnt_) {
				assert (STATE_DELETION != state_);
				if (STATE_ADDITION == state_) {
					Internal::IORequest::_ref_type rq = std::move (request_);
					rq->wait (std::numeric_limits <uint64_t>::max ());
					Any ex;
					if (rq->get_exception (ex))
						state_ = STATE_NEW;
					else
						state_ = STATE_CONFIRMED;
				}
				return STATE_CONFIRMED == state_;
			} else
				return false;
		}

		State state () const NIRVANA_NOEXCEPT
		{
			return state_;
		}

	private:
		unsigned ref_cnt_;
		State state_;
		Internal::IORequest::_ref_type request_;
	};

	RemoteRefKey& on_DGC_reference_unmarshal (const IOP::ObjectKey& object_key)
	{
		auto ins = remote_objects_.emplace (object_key);
		RemoteRefKey& key = const_cast <RemoteRefKey&> (*ins.first);
		if (!ins.second)
			key.reference_add ();
		return key;
	}

	void on_DGC_reference_delete (RemoteRefKey& key) NIRVANA_NOEXCEPT
	{
		if (key.reference_remove ())
			remote_objects_del_.push_back (key);
	}

	void confirm_DGC_references (const ReferenceRemoteRef* begin, const ReferenceRemoteRef* end);

protected:
	Domain (unsigned flags, TimeBase::TimeT request_latency, TimeBase::TimeT heartbeat_interval,
		TimeBase::TimeT heartbeat_timeout);
	~Domain ();

	virtual void _add_ref () NIRVANA_NOEXCEPT override;
	virtual void _remove_ref () NIRVANA_NOEXCEPT override;
	virtual void destroy () NIRVANA_NOEXCEPT = 0;

private:
	void complex_ping (const IDL::Sequence <IOP::ObjectKey>& add, const IDL::Sequence <IOP::ObjectKey>& del)
	{
		static const size_t STATIC_ADD_CNT = 4;
		if (add.size () <= STATIC_ADD_CNT) {
			std::array <ReferenceLocalRef, STATIC_ADD_CNT> refs;
			add_owned_objects (add, refs.data ());
		} else {
			std::vector <ReferenceLocalRef> refs (add.size ());
			add_owned_objects (add, refs.data ());
		}

		for (const IOP::ObjectKey& obj_key : del) {
			local_objects_.erase (obj_key);
		}
	}

	void add_owned_objects (const IDL::Sequence <IOP::ObjectKey>& keys, ReferenceLocalRef* objs);

	void send_reference_changes () NIRVANA_NOEXCEPT
	{
		if (remote_objects_del_.empty ())
			return;
		try {
			Internal::IORequest::_ref_type rq = create_request (IOP::ObjectKey (), op_complex_ping,
				Internal::IOReference::REQUEST_ASYNC);
			marshal_ref_list (remote_objects_del_, rq);
			rq->invoke ();
		} catch (...) {
			// TODO: Log
		}
		last_ping_out_time_ = Nirvana::Core::Chrono::steady_clock ();
	}

	void send_heartbeat ()
	{
		Internal::IORequest::_ref_type rq = create_request (IOP::ObjectKey (), op_heartbeat,
			Internal::IOReference::REQUEST_ASYNC);
		rq->invoke ();
		last_ping_out_time_ = Nirvana::Core::Chrono::steady_clock ();
	}

	static void marshal_ref_list (Nirvana::SimpleList <RemoteRefKey>& list, Internal::IORequest::_ptr_type rq);

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

	unsigned flags_;

	// DGC-enabled local objects owned by this domain
	typedef Nirvana::Core::MapUnorderedUnstable <IOP::ObjectKey, ReferenceLocalRef,
		std::hash <IOP::ObjectKey>, std::equal_to <IOP::ObjectKey>, 
		Nirvana::Core::UserAllocator> LocalObjects;

	LocalObjects local_objects_;

	// DGC-enabled references to the domain objects owned by the current domain
	typedef Nirvana::Core::SetUnorderedStable <RemoteRefKey,
		std::hash <IOP::ObjectKey>, std::equal_to <IOP::ObjectKey>,
		Nirvana::Core::UserAllocator> RemoteObjects;

	// DGC-enabled references to the domain objects owned by the current domain
	RemoteObjects remote_objects_;
	Nirvana::SimpleList <RemoteRefKey> remote_objects_del_;

	TimeBase::TimeT last_ping_in_time_;
	TimeBase::TimeT last_ping_out_time_;
	TimeBase::TimeT request_latency_;
	TimeBase::TimeT heartbeat_interval_;
	TimeBase::TimeT heartbeat_timeout_;

	static const Internal::Operation op_heartbeat;
	static const Internal::Operation op_complex_ping;
};

}
}

#endif
