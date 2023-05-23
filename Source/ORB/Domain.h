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

	class DGC_RefKey;

private:
	// DGC request
	class DGC_Request : public Nirvana::Core::UserObjectSyncRefCnt <DGC_Request>
	{
	public:
		DGC_Request (Domain& domain);

		void add (DGC_RefKey& key)
		{
			keys_.push_back (&key);
			++add_cnt_;
		}

		void del (DGC_RefKey& key)
		{
			keys_.push_back (&key);
		}

		void invoke ();

		void complete (bool no_throw = false);

	private:
		Domain& domain_;
		Internal::IORequest::_ref_type request_;
		std::vector <DGC_RefKey*, Nirvana::Core::UserAllocator <DGC_RefKey*> > keys_;
		size_t add_cnt_;

		static const Internal::Operation operation_;
	};

	typedef Nirvana::Core::Ref <DGC_Request> DGC_RequestRef;

public:
	// DGC-enabled remote reference owned by the current domain
	class DGC_RefKey :
		public IOP::ObjectKey,
		public Nirvana::SimpleList <DGC_RefKey>::Element
	{
	public:
		DGC_RefKey (const IOP::ObjectKey& object_key) :
			IOP::ObjectKey (object_key),
			reference_cnt_ (1),
			added_ (false)
		{
		}

		// Add remote reference with this key
		void reference_add () noexcept
		{
			++reference_cnt_;
		}

		// Remove remote reference with this key
		unsigned reference_remove () noexcept
		{
			return --reference_cnt_;
		}

		unsigned reference_cnt () const noexcept
		{
			return reference_cnt_;
		}

		bool unconfirmed () const noexcept
		{
			return !added_ || request_;
		}

		bool added () const noexcept
		{
			return added_;
		}

		void complete_deletion () noexcept
		{
			if (request_ && added_)
				request_->complete (true);
		}

		void complete_addition () noexcept
		{
			if (request_ && !added_)
				request_->complete (true);
		}

		void request_completed () noexcept
		{
			assert (request_);
			request_ = nullptr;
			added_ = !added_;
		}

		void request_failed () noexcept
		{
			assert (request_);
			request_ = nullptr;
			if (!added_)
				exception_ = std::current_exception ();
		}

		DGC_Request* request () const noexcept
		{
			return request_;
		}

		void request (DGC_Request& rq) noexcept
		{
			exception_ = nullptr;
			request_ = &rq;
		}

		void check ()
		{
			if (exception_)
				std::rethrow_exception (exception_);
		}

	private:
		unsigned reference_cnt_;
		bool added_;
		DGC_RequestRef request_;
		std::exception_ptr exception_;
	};

	DGC_RefKey& on_DGC_reference_unmarshal (const IOP::ObjectKey& object_key)
	{
		bool first = remote_objects_.empty ();
		auto ins = remote_objects_.emplace (object_key);
		if (first)
			_add_ref ();
		DGC_RefKey& key = const_cast <DGC_RefKey&> (*ins.first);
		if (!ins.second)
			key.reference_add ();
		return key;
	}

	void on_DGC_reference_delete (DGC_RefKey& key) noexcept
	{
		if (0 == key.reference_remove ()) {
			if (key.added () || key.request ()) {
				remote_objects_del_.push_back (key);
				schedule_del ();
			} else
				erase_remote_key (key);
		}
	}

	static void confirm_DGC_references (size_t cnt, CORBA::Core::ReferenceRemoteRef* refs);

protected:
	Domain (unsigned flags, TimeBase::TimeT request_latency, TimeBase::TimeT heartbeat_interval,
		TimeBase::TimeT heartbeat_timeout);
	~Domain ();

	virtual void _add_ref () NIRVANA_NOEXCEPT override;
	virtual void _remove_ref () NIRVANA_NOEXCEPT override;
	virtual void destroy () NIRVANA_NOEXCEPT = 0;

private:
	void confirm_DGC_ref_start (const ReferenceRemoteRef* begin, const ReferenceRemoteRef* end);
	void confirm_DGC_ref_finish (const ReferenceRemoteRef* begin, const ReferenceRemoteRef* end, bool no_throw = false);

	void complex_ping (IDL::Sequence <IOP::ObjectKey>& add, const IDL::Sequence <IOP::ObjectKey>& del)
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
			if (local_objects_.erase (obj_key) && local_objects_.empty ())
				_remove_ref ();
		}
	}

	void add_owned_objects (IDL::Sequence <IOP::ObjectKey>& keys, ReferenceLocalRef* objs);

	void send_heartbeat ()
	{
		Internal::IORequest::_ref_type rq = create_request (IOP::ObjectKey (), op_heartbeat_,
			Internal::IOReference::REQUEST_ASYNC);
		rq->invoke ();
		last_ping_out_time_ = Nirvana::Core::Chrono::steady_clock ();
	}

	void schedule_del () noexcept;
	void send_del ();
	void append_del (DGC_Request& rq);
	void erase_remote_key (DGC_RefKey& key) noexcept;

private:
	class GC : public Nirvana::Core::Runnable
	{
	public:
		GC (Domain& domain) :
			domain_ (&domain)
		{
		}

	private:
		virtual void run () override;

	private:
		servant_reference <Domain> domain_;
	};

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
	typedef Nirvana::Core::SetUnorderedStable <DGC_RefKey,
		std::hash <IOP::ObjectKey>, std::equal_to <IOP::ObjectKey>,
		Nirvana::Core::UserAllocator> RemoteObjects;

	// DGC-enabled references to the domain objects owned by the current domain
	RemoteObjects remote_objects_;
	Nirvana::SimpleList <DGC_RefKey> remote_objects_del_;

	TimeBase::TimeT last_ping_in_time_;
	TimeBase::TimeT last_ping_out_time_;
	TimeBase::TimeT request_latency_;
	TimeBase::TimeT heartbeat_interval_;
	TimeBase::TimeT heartbeat_timeout_;

	bool DGC_scheduled_;

	static const Internal::Operation op_heartbeat_;
};

}
}

#endif
