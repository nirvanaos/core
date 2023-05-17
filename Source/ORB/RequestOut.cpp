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
#include "RequestOut.h"
#include "OutgoingRequests.h"
#include "DomainRemote.h"
#include "../MemContextCore.h"

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

std::atomic <RequestOut::IdGenType> RequestOut::last_id_;

RequestOut::RequestId RequestOut::get_new_id (IdPolicy id_policy) NIRVANA_NOEXCEPT
{
	IdGenType id;
	switch (id_policy) {
		case IdPolicy::ANY:
			id = last_id_.fetch_add (1, std::memory_order_release) + 1;
			break;

		case IdPolicy::EVEN: {
			IdGenType last = last_id_.load (std::memory_order_acquire);
			do {
				id = last + 2 - (last & 1);
			} while (!last_id_.compare_exchange_weak (last, id));
		} break;

		case IdPolicy::ODD: {
			IdGenType last = last_id_.load (std::memory_order_acquire);
			do {
				id = last + 1 + (last & 1);
			} while (!last_id_.compare_exchange_weak (last, id));
		} break;
	}
	return (RequestId)id;
}

RequestOut::RequestOut (unsigned GIOP_minor, unsigned response_flags,
	Domain& servant_domain, const Internal::Operation& metadata) :
	RequestGIOP (GIOP_minor, response_flags, &servant_domain),
	metadata_ (&metadata),
	id_ (0),
	status_ (Status::IN_PROGRESS),
	request_id_offset_ (0)
{
#if (NIRVANA_DEBUG_ITERATORS != 0)
	SyncContext& sc = SyncContext::current ();
	if (!sc.sync_domain () && !sc.is_free_sync_context ()) {
		// Legacy process has memory context with interlocked access to runtime support.
		// To avoid context switches in iterator debugging during the unmarshaling
		// we have to create a new memory context with the same heap.
		mem_context_ = Nirvana::Core::Ref <MemContext>::create <Nirvana::Core::MemContextCore>
			(std::ref (mem_context_->heap ()));
	}
#endif

	if (metadata.flags & Operation::FLAG_OUT_CPLX)
		response_flags_ |= FLAG_PREUNMARSHAL;

	ExecDomain& ed = ExecDomain::current ();
	if ((response_flags & (1 | IOReference::REQUEST_ASYNC)) == 1)
		deadline_ = ed.deadline ();
	else
		deadline_ = ed.get_request_deadline (!(response_flags & 1));
}

RequestOut::~RequestOut ()
{}

void RequestOut::write_header (const IOP::ObjectKey& object_key, IDL::String& operation, IOP::ServiceContextList& context)
{
	stream_out_->write_message_header (GIOP_minor_, GIOP::MsgType::Request);

	if (GIOP_minor_ <= 1) {
		GIOP::RequestHeader_1_1 hdr;

		// Calculate request id offset
		size_t off = sizeof (GIOP::RequestHeader_1_1) + 4;
		for (const IOP::ServiceContext& ctx : context) {
			off += 8 + ctx.context_data ().size ();
		}
		request_id_offset_ = off;

		hdr.service_context ().swap (context);
		hdr.request_id (id_);
		hdr.response_expected (response_flags_ & RESPONSE_EXPECTED);
		hdr.object_key (object_key);
		hdr.operation ().swap (operation);
		Type <GIOP::RequestHeader_1_1>::marshal_in (hdr, _get_ptr ());
		hdr.service_context ().swap (context);
		hdr.operation ().swap (operation);
	} else {
		GIOP::RequestHeader_1_2 hdr;
		request_id_offset_ = sizeof (GIOP::RequestHeader_1_2);
		hdr.request_id (id_);
		hdr.response_flags ((Octet)(response_flags_ & (RESPONSE_EXPECTED | RESPONSE_DATA)));
		hdr.target ().object_key (object_key);
		hdr.operation ().swap (operation);
		hdr.service_context ().swap (context);
		Type <GIOP::RequestHeader_1_2>::marshal_in (hdr, _get_ptr ());
		hdr.service_context ().swap (context);
		hdr.operation ().swap (operation);

		// In GIOP version 1.2 and 1.3, the Request Body is always aligned on an 8-octet boundary.
		size_t unaligned = stream_out_->size () % 8;
		if (unaligned) {
			Octet zero [8] = { 0 };
			stream_out_->write_c (1, 8 - unaligned, zero);
		}
	}
}

void RequestOut::id (RequestId id)
{
	if (request_id_offset_) {
		size_t offset = request_id_offset_;
		Octet* hdr = (Octet*)stream_out_->header (offset + 4);
		*(uint32_t*)(hdr + offset) = id;
	}
	id_ = id;
}

void RequestOut::set_reply (unsigned status, IOP::ServiceContextList&& context,
	Ref <StreamIn>&& stream)
{
	switch (status_ = (Status)status) {
		case Status::NO_EXCEPTION:
			// If target domain does not support DGC, we make passed DGC references as owned to it.
			if (!marshaled_DGC_references_.empty () && !(target_domain_->flags () & Domain::GARBAGE_COLLECTION))
				static_cast <DomainRemote&> (*target_domain_).add_DGC_objects (marshaled_DGC_references_);

			if (!(response_flags_ & RESPONSE_DATA)) {
				// Data is not expected
				finalize ();
				break;
			}
			stream_in_ = std::move (stream);
			if (FLAG_PREUNMARSHAL & response_flags_) {
				// Preunmarshal data.
				ExecDomain& ed = ExecDomain::current ();
				ed.deadline (deadline_);
				// Memory context must be set by caller.
				assert (ed.mem_context_ptr () == &memory ());
				Ref <RequestLocalBase> pre = Ref <RequestLocalBase>::
					create <RequestLocalImpl <RequestLocalBase> > (&memory (), 3);
				IORequest::_ptr_type rq = pre->_get_ptr ();
				std::vector <Octet> buf;
				buf.resize (3 * sizeof (void*));
				for (const Parameter* param = metadata_->output.p, *end = param + metadata_->output.size;
					param != end; ++param) {
					preunmarshal ((param->type) (), buf, rq);
				}
				if (metadata_->return_type)
					preunmarshal ((metadata_->return_type) (), buf, rq);
				Base::unmarshal_end ();
				pre->invoke (); // Rewind to begin
				preunmarshaled_ = std::move (pre);
			}

			finalize ();
			break;

		case Status::USER_EXCEPTION:
		case Status::SYSTEM_EXCEPTION:
			if (response_flags_ & RESPONSE_EXPECTED)
				stream_in_ = std::move (stream);
			finalize ();
			break;

		default:
			throw UNKNOWN (); // Unexpected reply
	}
}

void RequestOut::preunmarshal (TypeCode::_ptr_type tc, std::vector <Octet> buf, IORequest::_ptr_type out)
{
	size_t cb = tc->n_size ();
	if (buf.size () < cb)
		buf.resize (cb);
	tc->n_construct (buf.data ());
	tc->n_unmarshal (_get_ptr (), 1, buf.data ());
	tc->n_marshal_out (buf.data (), 1, out);
}

bool RequestOut::unmarshal (size_t align, size_t size, void* data)
{
	if (preunmarshaled_)
		return preunmarshaled_->unmarshal (align, size, data);
	else
		return RequestGIOP::unmarshal (align, size, data);
}

bool RequestOut::unmarshal_seq (size_t align, size_t element_size, size_t& element_count, void*& data,
	size_t& allocated_size)
{
	if (preunmarshaled_)
		return preunmarshaled_->unmarshal_seq (align, element_size, element_count, data, allocated_size);
	else
		return RequestGIOP::unmarshal_seq (align, element_size, element_count, data, allocated_size);
}

size_t RequestOut::unmarshal_seq_begin ()
{
	if (preunmarshaled_)
		return preunmarshaled_->unmarshal_seq_begin ();
	else
		return RequestGIOP::unmarshal_seq_begin ();
}

void RequestOut::unmarshal_char (size_t count, Char* data)
{
	if (preunmarshaled_)
		preunmarshaled_->unmarshal_char (count, data);
	else
		RequestGIOP::unmarshal_char (count, data);
}

void RequestOut::unmarshal_string (IDL::String& s)
{
	if (preunmarshaled_)
		preunmarshaled_->unmarshal_string (s);
	else
		RequestGIOP::unmarshal_string (s);
}

void RequestOut::unmarshal_char_seq (IDL::Sequence <Char>& s)
{
	if (preunmarshaled_)
		preunmarshaled_->unmarshal_char_seq (s);
	else
		RequestGIOP::unmarshal_char_seq (s);
}

void RequestOut::unmarshal_wchar (size_t count, WChar* data)
{
	if (preunmarshaled_)
		preunmarshaled_->unmarshal_wchar (count, data);
	else
		RequestGIOP::unmarshal_wchar (count, data);
}

void RequestOut::unmarshal_wstring (IDL::WString& s)
{
	if (preunmarshaled_)
		preunmarshaled_->unmarshal_wstring (s);
	else
		RequestGIOP::unmarshal_wstring (s);
}

void RequestOut::unmarshal_wchar_seq (IDL::Sequence <WChar>& s)
{
	if (preunmarshaled_)
		preunmarshaled_->unmarshal_wchar_seq (s);
	else
		RequestGIOP::unmarshal_wchar_seq (s);
}

Interface::_ref_type RequestOut::unmarshal_interface (const IDL::String& interface_id)
{
	if (preunmarshaled_)
		return preunmarshaled_->unmarshal_interface (interface_id);
	else
		return RequestGIOP::unmarshal_interface (interface_id);
}

TypeCode::_ref_type RequestOut::unmarshal_type_code ()
{
	if (preunmarshaled_)
		return preunmarshaled_->unmarshal_type_code ();
	else
		return RequestGIOP::unmarshal_type_code ();
}

Interface::_ref_type RequestOut::unmarshal_value (const IDL::String& interface_id)
{
	if (preunmarshaled_)
		return preunmarshaled_->unmarshal_value (interface_id);
	else
		return RequestGIOP::unmarshal_value (interface_id);
}

Interface::_ref_type RequestOut::unmarshal_abstract (const IDL::String& interface_id)
{
	if (preunmarshaled_)
		return preunmarshaled_->unmarshal_abstract (interface_id);
	else
		return RequestGIOP::unmarshal_abstract (interface_id);
}

void RequestOut::unmarshal_end ()
{
	if (preunmarshaled_)
		preunmarshaled_->unmarshal_end ();
	else
		RequestGIOP::unmarshal_end ();
}

bool RequestOut::marshal_op ()
{
	return true;
}

void RequestOut::pre_invoke (IdPolicy id_policy)
{
	if (!stream_out_)
		throw BAD_INV_ORDER ();
	Base::invoke ();
	bool response = (response_flags_ & RESPONSE_EXPECTED) != 0;
	if (!response && !marshaled_DGC_references_.empty ()) {
		// For oneway request with DGC references we have to wait response before the references releasing.
		// Offset of the response_flags is immediately after the request_id
		size_t offset = request_id_offset_ + 4;
		Octet* hdr = (Octet*)stream_out_->header (offset + 1);
		assert (0 == hdr [offset]);
		hdr [offset] = 1;
		response = true;
	}
	if (metadata_->context.size != 0) {
		IDL::Sequence <IDL::String> context;
		ExecDomain::current ().get_context (metadata_->context.p, metadata_->context.size, context);
		Type <IDL::Sequence <IDL::String> >::marshal_out (context, _get_ptr ());
	}
	if (response)
		OutgoingRequests::new_request (*this, id_policy);
	else
		OutgoingRequests::new_request_oneway (*this, id_policy);
}

void RequestOut::set_exception (Any& e)
{
	throw BAD_INV_ORDER ();
}

void RequestOut::set_system_exception (const Char* rep_id, uint32_t minor, CompletionStatus completed) NIRVANA_NOEXCEPT
{
	status_ = Status::SYSTEM_EXCEPTION;
	try {
		ImplStatic <StreamOutEncap> sout (true);
		sout.write_string_c (rep_id);
		SystemException::_Data data { minor, completed };
		sout.write_c (alignof (SystemException::_Data), sizeof (SystemException::_Data), &data);
		stream_in_ = Ref <StreamIn>::create <ImplDynamic <StreamInEncapData> > (std::move (sout.data ()));
		finalize ();
	} catch (...) {}
}

void RequestOut::success ()
{
	throw BAD_INV_ORDER ();
}

bool RequestOut::cancel_internal ()
{
	if (!(response_flags_ & RESPONSE_EXPECTED))
		throw BAD_INV_ORDER ();

	if (CORBA::Core::OutgoingRequests::remove_request (id_)) {
		status_ = Status::CANCELLED;
		return true;
	}
	return false;
}

bool RequestOut::get_exception (Any& e)
{
	switch (status_) {
		case Status::SYSTEM_EXCEPTION:
		case Status::USER_EXCEPTION: {
			IDL::String id;
			stream_in_->read_string (id);
			TypeCode::_ptr_type tc;
			if (Status::SYSTEM_EXCEPTION == status_) {
				const ExceptionEntry* ee = SystemException::_get_exception_entry (id);
				if (ee) {
					std::aligned_storage <sizeof (SystemException), alignof (SystemException)>::type se;
					(ee->construct) (&se);
					tc = reinterpret_cast <SystemException&> (se).__type_code ();
				}
			} else {
				for (auto p = metadata_->user_exceptions.p, end = p + metadata_->user_exceptions.size; p != end; ++p) {
					TypeCode::_ptr_type tcp = **p;
					if (RepId::compatible (tcp->id (), id)) {
						tc = tcp;
						break;
					}
				}
			}
			if (tc)
				Type <Any>::unmarshal (tc, _get_ptr (), e);
			else
				e <<= UNKNOWN (Status::SYSTEM_EXCEPTION == status_ ? MAKE_OMG_MINOR (1) : MAKE_OMG_MINOR (2));
			return true;
		}
	}
	return false;
}

bool RequestOut::completed () const NIRVANA_NOEXCEPT
{
	return status_ >= Status::NO_EXCEPTION;
}

bool RequestOut::wait (uint64_t timeout)
{
	throw NO_IMPLEMENT ();
}

}
}
