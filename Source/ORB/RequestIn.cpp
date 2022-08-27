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
#include "RequestEncapIn.h"
#include "IIOP.h"
#include "ServantProxyBase.h"

using namespace Nirvana;
using namespace Nirvana::Core;
using namespace std;

namespace CORBA {

using namespace Internal;

namespace Core {

RequestIn::RequestIn (const DomainAddress& client, unsigned GIOP_minor, CoreRef <StreamIn>&& in) :
	RequestGIOP (GIOP_minor, false),
	key_ (client),
	GIOP_minor_ (GIOP_minor),
	exec_domain_ (nullptr),
	sync_domain_ (nullptr)
{
	stream_in_ = move (in);

	switch (GIOP_minor) {
		case 0: {
			typedef GIOP::RequestHeader_1_0 Hdr;
			Hdr hdr;
			Type <Hdr>::unmarshal (_get_ptr (), hdr);
			key_.request_id = hdr.request_id ();
			object_key_.unmarshal (hdr.object_key ());
			operation_ = move (hdr.operation ());
			service_context_ = move (hdr.service_context ());
			response_flags_ = hdr.response_expected () ? (RESPONSE_EXPECTED | RESPONSE_DATA) : 0;
		} break;

		case 1: {
			typedef GIOP::RequestHeader_1_1 Hdr;
			Hdr hdr;
			Type <Hdr>::unmarshal (_get_ptr (), hdr);
			key_.request_id = hdr.request_id ();
			object_key_.unmarshal (hdr.object_key ());
			operation_ = move (hdr.operation ());
			service_context_ = move (hdr.service_context ());
			response_flags_ = hdr.response_expected () ? (RESPONSE_EXPECTED | RESPONSE_DATA) : 0;
		} break;

		default: {
			typedef GIOP::RequestHeader_1_2 Hdr;
			Hdr hdr;
			Type <Hdr>::unmarshal (_get_ptr (), hdr);
			key_.request_id = hdr.request_id ();
			operation_ = move (hdr.operation ());
			service_context_ = move (hdr.service_context ());
			response_flags_ = hdr.response_flags ();
			if ((response_flags_ & (RESPONSE_EXPECTED | RESPONSE_DATA)) == RESPONSE_DATA)
				throw INV_FLAG ();

			switch (hdr.target ()._d ()) {
				case GIOP::KeyAddr:
					object_key_.unmarshal (hdr.target ().object_key ());
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
		}
	}
}

void RequestIn::get_object_key (const IOP::TaggedProfile& profile)
{
	if (IOP::TAG_INTERNET_IOP == profile.tag ()) {
		ImplStatic <StreamInEncap> s (ref (profile.profile_data ()));

		s.read (alignof (IIOP::Version), sizeof (IIOP::Version), nullptr);
		size_t host_len = s.read_size ();
		s.read (1, host_len + 1, nullptr);
		s.read (2, 2, nullptr);
		object_key_.unmarshal (s);
	}
	throw OBJECT_NOT_EXIST ();
}

RequestIn::~RequestIn ()
{
	// While request in map, exec_domain_ is not nullptr.
	// For the oneway requests, exec_domain_ is always nullptr.
	if (exec_domain_)
		finalize (); // Remove from the map
}

void RequestIn::switch_to_reply (GIOP::ReplyStatusType status)
{
	// Leave the object synchronization domain, if any.
	ExecDomain::current ().leave_sync_domain ();

	stream_in_ = nullptr;
	if (!stream_out_) {
		stream_out_ = create_output ();
		stream_out_->write_message_header (GIOP_minor_, GIOP::MsgType::Reply);
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
	}
}

bool RequestIn::marshal_op ()
{
	switch_to_reply ();
	return (response_flags_ & RESPONSE_DATA) != 0;
}

void RequestIn::serve_request (ServantProxyBase& proxy)
{
	ExecDomain& ed = ExecDomain::current ();
	if (response_flags_)
		exec_domain_ = &ed; // Save ED pointer for cancel
	IOReference::OperationIndex op_idx = proxy.find_operation (operation ());
	sync_domain_ = proxy.get_sync_context (op_idx).sync_domain ();
	ed.mem_context_push (&sync_domain_->mem_context ());
	proxy.invoke (op_idx, _get_ptr ());
	ed.mem_context_pop ();
	ed.leave_sync_domain ();
}

void RequestIn::unmarshal_end ()
{
	RequestGIOP::unmarshal_end ();
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
	if (e.type ()->kind () != TCKind::tk_except)
		throw BAD_PARAM (MAKE_OMG_MINOR (21));

	GIOP::ReplyStatusType status = e.is_system_exception () ?
		GIOP::ReplyStatusType::SYSTEM_EXCEPTION
		:
		GIOP::ReplyStatusType::USER_EXCEPTION;

	if (!stream_out_)
		switch_to_reply (status);
	else {
		stream_out_->rewind (reply_header_end_);
		*(GIOP::ReplyStatusType*)((Octet*)stream_out_->header (reply_header_end_) + reply_status_offset_) = status;
	}
	if (response_flags_)
		response_flags_ |= RESPONSE_DATA; // To marshal Any.
	Type <Any>::marshal_out (e, _get_ptr ());
}

bool RequestIn::finalize ()
{
	if (response_flags_ && atomic_exchange ((volatile atomic <ExecDomain*>*) & exec_domain_, nullptr))
		return IncomingRequests::finalize (key_);

	assert (!exec_domain_);
	return false;
}

void RequestIn::cancel ()
{
	response_flags_ = 0;
	ExecDomain* ed = atomic_exchange ((volatile atomic <ExecDomain*>*)&exec_domain_, nullptr);
	// TODO: We must call ed->abort () here but it is not implemented yet.
	// if (ed)
	//   ed->abort ();
}

void RequestIn::invoke ()
{
	throw BAD_OPERATION ();
}

bool RequestIn::is_exception () const NIRVANA_NOEXCEPT
{
	return false;
}

bool RequestIn::completed () const NIRVANA_NOEXCEPT
{
	return false;
}

bool RequestIn::wait (uint64_t)
{
	throw BAD_OPERATION ();
}

}
}
