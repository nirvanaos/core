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
#ifndef NIRVANA_ORB_CORE_REQUESTOUT_H_
#define NIRVANA_ORB_CORE_REQUESTOUT_H_
#pragma once

#include "RequestGIOP.h"
#include "RequestLocal.h"
#include "../ExecDomain.h"
#include "../UserObject.h"
#include "../Event.h"

namespace CORBA {
namespace Core {

/// Implements client-side IORequest for GIOP.
class RequestOut :
	public RequestGIOP
{
	static const unsigned FLAG_PREUNMARSHAL = 8;

public:
	RequestOut (unsigned GIOP_minor, unsigned response_flags, const Internal::Operation& metadata);

	~RequestOut ();

	uint32_t id () const NIRVANA_NOEXCEPT
	{
		return id_;
	}

	const Nirvana::DeadlineTime deadline() const NIRVANA_NOEXCEPT
	{
		return deadline_;
	}

	virtual void set_reply (unsigned status, IOP::ServiceContextList&& context,
		Nirvana::Core::CoreRef <StreamIn>&& stream);

	void set_system_exception (const Char* rep_id, uint32_t minor, CompletionStatus completed)
		NIRVANA_NOEXCEPT;

protected:
	void write_header (IOP::ObjectKey& object_key, IDL::String& operation, IOP::ServiceContextList& context);

	virtual bool unmarshal (size_t align, size_t size, void* data) override;
	virtual bool unmarshal_seq(size_t align, size_t element_size, size_t& element_count, void*& data,
		size_t& allocated_size) override;
	virtual size_t unmarshal_seq_begin() override;

	virtual bool marshal_op () override;
	virtual void success () override;
	virtual void set_exception (Any& e) override;
	virtual bool get_exception (Any& e) override;
	virtual bool completed () const NIRVANA_NOEXCEPT override;
	virtual bool wait (uint64_t timeout) override;

	bool cancel_internal ();

	void post_invoke () NIRVANA_NOEXCEPT
	{
		if (!(response_flags_ & Internal::IOReference::REQUEST_ASYNC)) {
			assert (response_flags_ & 3);
			event_.wait ();
		}
	}

private:
	void finalize () NIRVANA_NOEXCEPT
	{
		event_.signal ();
	}

	void preunmarshal(TypeCode::_ptr_type tc, std::vector <Octet> buf, Internal::IORequest::_ptr_type out);

protected:
	Nirvana::DeadlineTime deadline_;
	const Internal::Operation* metadata_;

	uint32_t id_;

	enum class Status
	{
		CANCELLED = -2,
		IN_PROGRESS = -1,
		NO_EXCEPTION,
		USER_EXCEPTION,
		SYSTEM_EXCEPTION,
		LOCATION_FORWARD,
		LOCATION_FORWARD_PERM,
		NEEDS_ADDRESSING_MODE
	} status_;

	Nirvana::Core::Event event_;
	Nirvana::Core::CoreRef <RequestLocalBase> preunmarshaled_;
};

}
}

#endif
