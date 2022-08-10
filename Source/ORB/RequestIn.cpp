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

using namespace Nirvana;
using namespace Nirvana::Core;
using namespace std;

namespace CORBA {

using namespace Internal;

namespace Core {

RequestIn::RequestIn (const ClientAddress& client, CoreRef <StreamIn>&& in, CoreRef <CodeSetConverterW>&& cscw) :
	Request (move (cscw)),
	key_ (client),
	exec_domain_ (nullptr)
{
	stream_in_ = move (in);
}

RequestIn::~RequestIn ()
{
	// While request in map, exec_domain_ is not nullptr.
	// For the oneway requests, exec_domain_ is always nullptr.
	if (exec_domain_)
		finalize (); // Remove from the map
}

void RequestIn::unmarshal_end ()
{
	if (stream_in_) {
		size_t more_data = !stream_in_->end ();
		stream_in_ = nullptr;
		if (more_data > 7) // 8-byte alignment is ignored
			throw MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
	}
}

void RequestIn::switch_to_reply (GIOP::ReplyStatusType status)
{
	stream_in_ = nullptr;
	if (!stream_out_) {
		unsigned GIOP_minor;
		stream_out_ = create_output (GIOP_minor);
		stream_out_->write_message_header (GIOP::MsgType::Reply, GIOP_minor);
		if (GIOP_minor <= 1) {
			GIOP::ReplyHeader_1_0 hdr;
			// hdr.service_context (move (context_)); TODO: decide
			hdr.request_id (request_id ());
			hdr.reply_status (status);
			Type <GIOP::ReplyHeader_1_0>::marshal_out (hdr, _get_ptr ());
			reply_status_offset_ = stream_out_->size () - 4;
		} else {
			GIOP::ReplyHeader_1_2 hdr;
			// hdr.service_context (move (context_)); TODO: decide
			hdr.request_id (request_id ());
			hdr.reply_status (status);
			reply_status_offset_ = stream_out_->size () + 4;
			Type <GIOP::ReplyHeader_1_2>::marshal_out (hdr, _get_ptr ());
		}
		reply_header_end_ = stream_out_->size ();
	}
}

bool RequestIn::marshal_op ()
{
	switch_to_reply ();
	return (response_flags_ & RESPONSE_DATA) != 0;
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

const IOP::ObjectKey& RequestIn_1_2::object_key () const
{
	switch (header ().target ()._d ()) {
		case GIOP::KeyAddr:
			return header ().target ().object_key ();

		case GIOP::ProfileAddr:
			return key_from_profile (header ().target ().profile ());

		case GIOP::ReferenceAddr:
		{
			const GIOP::IORAddressingInfo& ior = header ().target ().ior ();
			const IOP::TaggedProfileSeq& profiles = ior.ior ().profiles ();
			if (profiles.size () <= ior.selected_profile_index ())
				throw OBJECT_NOT_EXIST ();
			return key_from_profile (profiles [ior.selected_profile_index ()]);
		}
	}

	throw UNKNOWN ();
}

}
}
