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
#include "RequestIn.h"
#include "IncomingRequests.h"
#include <CORBA/IIOP.h>
#include "ServantProxyObject.h"
#include "DomainRemote.h"
#include "tagged_seq.h"

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

RequestIn::RequestIn (const DomainAddress& client, unsigned GIOP_minor) :
	RequestGIOP (GIOP_minor, 0, nullptr),
	key_ (client),
	map_iterator_ (nullptr),
	sync_domain_ (nullptr),
	reply_status_offset_ (0),
	reply_header_end_ (0),
	cancelled_ (false),
	has_context_ (false)
{
}

void RequestIn::initialize (Ref <StreamIn>&& in)
{
	stream_in_ = std::move (in);

	switch (GIOP_minor_) {
		case 0: {
			typedef GIOP::RequestHeader_1_0 Hdr;
			Hdr hdr;
			Type <Hdr>::unmarshal (_get_ptr (), hdr);
			key_.request_id = hdr.request_id ();
			object_key_ = std::move (hdr.object_key ());
			operation_ = std::move (hdr.operation ());
			service_context_ = std::move (hdr.service_context ());
			response_flags_ = hdr.response_expected () ? (RESPONSE_EXPECTED | RESPONSE_DATA) : 0;
		} break;

		case 1: {
			typedef GIOP::RequestHeader_1_1 Hdr;
			Hdr hdr;
			Type <Hdr>::unmarshal (_get_ptr (), hdr);
			key_.request_id = hdr.request_id ();
			object_key_ = std::move (hdr.object_key ());
			operation_ = std::move (hdr.operation ());
			service_context_ = std::move (hdr.service_context ());
			response_flags_ = hdr.response_expected () ? (RESPONSE_EXPECTED | RESPONSE_DATA) : 0;
		} break;

		default: {
			typedef GIOP::RequestHeader_1_2 Hdr;
			Hdr hdr;
			Type <Hdr>::unmarshal (_get_ptr (), hdr);
			key_.request_id = hdr.request_id ();
			operation_ = std::move (hdr.operation ());
			service_context_ = std::move (hdr.service_context ());
			response_flags_ = hdr.response_flags ();
			if ((response_flags_ & (RESPONSE_EXPECTED | RESPONSE_DATA)) == RESPONSE_DATA)
				throw INV_FLAG ();

			switch (hdr.target ()._d ()) {
				case GIOP::KeyAddr:
					object_key_ = std::move (hdr.target ().object_key ());
					break;

				case GIOP::ProfileAddr:
					get_object_key (hdr.target ().profile ());
					break;

				case GIOP::ReferenceAddr: {
					const GIOP::IORAddressingInfo& ior = hdr.target ().ior ();
					const IOP::TaggedProfileSeq& profiles = ior.ior ().profiles ();
					if (profiles.size () <= ior.selected_profile_index ())
						throw OBJECT_NOT_EXIST ();
					get_object_key (profiles [ior.selected_profile_index ()]);
				} break;
			}

			// In GIOP version 1.2 and 1.3, the Request Body is always aligned on an 8-octet boundary.
			size_t unaligned = stream_in_->position() % 8;
			if (unaligned)
				stream_in_->read(1, 8 - unaligned, nullptr);
		}
	}
	sort (service_context_);
}

void RequestIn::get_object_key (const IOP::TaggedProfile& profile)
{
	if (IOP::TAG_INTERNET_IOP == profile.tag ()) {
		ImplStatic <StreamInEncap> stm (ref (profile.profile_data ()));

		// Skip IIOP version
		stm.read (alignof (IIOP::Version), sizeof (IIOP::Version), nullptr);

		// Skip host name
		size_t host_len = stm.read_size ();
		stm.read (1, host_len + 1, nullptr);

		// Skip port number
		stm.read (2, 2, nullptr);

		stm.read_seq (object_key_);
	}
	throw OBJECT_NOT_EXIST ();
}

RequestIn::~RequestIn ()
{}

void RequestIn::_add_ref () NIRVANA_NOEXCEPT
{
	RequestGIOP::_add_ref ();
}

void RequestIn::_remove_ref () NIRVANA_NOEXCEPT
{
	RequestGIOP::_remove_ref ();
}

MemContext* RequestIn::memory () const NIRVANA_NOEXCEPT
{
	return &RequestGIOP::memory ();
}

void RequestIn::switch_to_reply (GIOP::ReplyStatusType status)
{
	if (!stream_out_) {
		// Leave the object synchronization domain, if any.
		ExecDomain::current ().leave_sync_domain ();
		stream_in_ = nullptr;
		if (response_flags_ & RESPONSE_EXPECTED) {
			stream_out_ = create_output ();
			stream_out_->write_message_header (GIOP_minor_, GIOP::MsgType::Reply);

			// Before marshaling the reply header we must set RESPONSE_DATA flag
			// to prevent bypass.
			unsigned flags = response_flags_;
			response_flags_ |= RESPONSE_DATA;
			try {
				if (GIOP_minor_ <= 1) {
					GIOP::ReplyHeader_1_0 hdr;
					// hdr.service_context (move (context_)); TODO: decide
					hdr.request_id (request_id ());
					hdr.reply_status (status);
					Type <GIOP::ReplyHeader_1_0>::marshal_out (hdr, _get_ptr ());
					reply_header_end_ = stream_out_->size ();
					reply_status_offset_ = reply_header_end_ - 4;
				} else {
					GIOP::ReplyHeader_1_2 hdr;
					// hdr.service_context (move (context_)); TODO: decide
					hdr.request_id (request_id ());
					hdr.reply_status (status);
					reply_status_offset_ = stream_out_->size () + 4;
					Type <GIOP::ReplyHeader_1_2>::marshal_out (hdr, _get_ptr ());
					reply_header_end_ = stream_out_->size ();
				}
			} catch (...) {
				response_flags_ = flags;
				throw;
			}
			response_flags_ = flags;
		}
	}
}

bool RequestIn::marshal_op ()
{
	switch_to_reply ();
	return (response_flags_ & RESPONSE_DATA) != 0;
}

void RequestIn::serve (const ServantProxyBase& proxy)
{
	if (is_cancelled ())
		return;

	IOReference::OperationIndex op = proxy.find_operation (operation ());

	SyncContext* sync_context = &proxy.get_sync_context (op);
	SyncDomain* sync_domain = sync_context->sync_domain ();
	MemContext* mem;
	if (sync_domain)
		mem = &sync_domain->mem_context ();
	else
		mem = memory ();

	const Operation& op_md = proxy.operation_metadata (op);
	if (sync_domain && (op_md.flags & Operation::FLAG_IN_CPLX)) {
		// Do not enter sync domain immediately.
		// We enter to free sync context now and will enter sync_domain_
		// in unmarshal_end () when all input objects unmarshaled.
		sync_domain_ = sync_domain;
		sync_context = &g_core_free_sync_context;
	}

	has_context_ = op_md.context.size != 0;

	// We don't need to handle exceptions here, because invoke () does not throw exceptions.
	Nirvana::Core::Synchronized _sync_frame (*sync_context, mem);
	if (!is_cancelled ())
		proxy.invoke (op, _get_ptr ());
}

void RequestIn::unmarshal_end ()
{
	RequestGIOP::unmarshal_end ();

	if (has_context_) {
		IDL::Sequence <IDL::String> context;
		Type <IDL::Sequence <IDL::String> >::unmarshal (_get_ptr (), context);
		ExecDomain::current ().set_context (context);
	}

	// Here we must enter the target sync domain, if any.
	SyncDomain* sd;
	if ((sd = sync_domain_)) {
		sync_domain_ = nullptr;
		ExecDomain::current ().schedule_call_no_push_mem (*sd);
	}
}

void RequestIn::success ()
{
	switch_to_reply ();
}

void RequestIn::set_exception (Any& e)
{
	TypeCode::_ptr_type type = e.type ();
	if (type->kind () != TCKind::tk_except)
		throw BAD_PARAM (MAKE_OMG_MINOR (21));

	GIOP::ReplyStatusType status = e.is_system_exception () ?
		GIOP::ReplyStatusType::SYSTEM_EXCEPTION
		:
		GIOP::ReplyStatusType::USER_EXCEPTION;

	if (!stream_out_)
		switch_to_reply (status);
	else {
		assert (reply_header_end_);
		stream_out_->rewind (reply_header_end_);
		*(GIOP::ReplyStatusType*)((Octet*)stream_out_->header (reply_header_end_) + reply_status_offset_) = status;
	}
	unsigned rf = response_flags_;
	if (rf & RESPONSE_EXPECTED) {
		response_flags_ |= RESPONSE_DATA; // To marshal exception data.
		try {
			stream_out_->write_string_c (type->id ());
			type->n_marshal_out (e.data (), 1, _get_ptr ());
		} catch (...) {}
		response_flags_ = rf;
	}
}

bool RequestIn::finalize () NIRVANA_NOEXCEPT
{
	if (map_iterator_) {
		bool ret = IncomingRequests::finalize (map_iterator_);
		map_iterator_ = nullptr;
		return ret;
	}

	return false;
}

void RequestIn::cancel () NIRVANA_NOEXCEPT
{
	// Oneway requests (response_flags_ == 0) are not cancellable.
	if (response_flags_ && !cancelled_.exchange (true, std::memory_order_release)) {
		response_flags_ = 0; // Prevent marshaling
		if (map_iterator_) {
			IncomingRequests::release_iterator (map_iterator_);
			map_iterator_ = nullptr;
		}
	}
}

bool RequestIn::is_cancelled () const NIRVANA_NOEXCEPT
{
	return cancelled_.load (std::memory_order_acquire);
}

void RequestIn::invoke ()
{
	throw BAD_OPERATION ();
}

bool RequestIn::get_exception (Any& e)
{
	return false;
}

void RequestIn::post_send_success () NIRVANA_NOEXCEPT
{
	if (!marshaled_DGC_references_.empty ()) {
		assert (target_domain_);
		if (!(target_domain_->flags () & Domain::GARBAGE_COLLECTION))
			static_cast <DomainRemote&> (*target_domain_).add_DGC_objects (marshaled_DGC_references_);
		else {
			// For DGC-enabled target domain we need to delay request release
			try {
				new (&delayed_release_timer_) DelayedReleaseTimer (std::ref (*this));
			} catch (...) {
				return;
			}
			_add_ref ();
			try {
				((DelayedReleaseTimer*)&delayed_release_timer_)->set_relative (target_domain_->request_latency (), 0);
			} catch (...) {
				((DelayedReleaseTimer*)&delayed_release_timer_)->DelayedReleaseTimer::~DelayedReleaseTimer ();
				_remove_ref ();
				return;
			}
		}
	}
}

inline void RequestIn::delayed_release () NIRVANA_NOEXCEPT
{
	((DelayedReleaseTimer*)&delayed_release_timer_)->DelayedReleaseTimer::~DelayedReleaseTimer ();
	_remove_ref ();
}

void RequestIn::DelayedReleaseTimer::signal () noexcept
{
	try {
		DeadlineTime deadline =
			PROXY_GC_DEADLINE == INFINITE_DEADLINE ?
			INFINITE_DEADLINE : Chrono::make_deadline (PROXY_GC_DEADLINE);
		ExecDomain::async_call <DelayedRelease> (deadline, g_core_free_sync_context, nullptr, std::ref (request_));
	} catch (...) {
		assert (false);
		try {
			set_relative (TimeBase::SECOND * 1, 0);
		} catch (...) {
			assert (false);
		}
	}
}

void RequestIn::DelayedRelease::run ()
{
	request_.delayed_release ();
}

}
}
