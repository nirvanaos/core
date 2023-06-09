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
class NIRVANA_NOVTABLE RequestOut : public RequestGIOP
{
	typedef RequestGIOP Base;
	static const unsigned FLAG_PREUNMARSHAL = 8;

public:
	// Request id generator.
#if ATOMIC_LONG_LOCK_FREE
	typedef long IdGenType;
#elif ATOMIC_INT_LOCK_FREE
	typedef int IdGenType;
#elif ATOMIC_LLONG_LOCK_FREE
	typedef long long IdGenType;
#elif ATOMIC_SHORT_LOCK_FREE
	typedef short IdGenType;
#else
#error Platform does not meet the minimal atomic requirements.
#endif

	// IdGen may have different sizes. We need 32-bit max, if possible.
	typedef std::conditional_t <(sizeof (IdGenType) <= 4), IdGenType, uint32_t> RequestId;

	enum class IdPolicy
	{
		ANY,
		EVEN,
		ODD
	};

	static RequestId get_new_id (IdPolicy id_policy) NIRVANA_NOEXCEPT;

	RequestId id () const NIRVANA_NOEXCEPT
	{
		return id_;
	}

	void id (RequestId id);

	const Nirvana::DeadlineTime deadline() const NIRVANA_NOEXCEPT
	{
		return deadline_;
	}

	virtual void set_reply (unsigned status, IOP::ServiceContextList&& context,
		Nirvana::Core::Ref <StreamIn>&& stream);

	void set_system_exception (const Char* rep_id, uint32_t minor, CompletionStatus completed)
		NIRVANA_NOEXCEPT;

	void on_reply_exception () NIRVANA_NOEXCEPT
	{
		reply_exception_ = std::current_exception ();
		finalize ();
	}

protected:
	RequestOut (unsigned GIOP_minor, unsigned response_flags, Domain& target_domain, const Internal::Operation& metadata);
	~RequestOut ();

	void write_header (const IOP::ObjectKey& object_key, IOP::ServiceContextList& context);

	virtual bool unmarshal (size_t align, size_t size, void* data) override;
	virtual bool unmarshal_seq(size_t align, size_t element_size, size_t& element_count, void*& data,
		size_t& allocated_size) override;
	virtual size_t unmarshal_seq_begin() override;
	virtual void unmarshal_char (size_t count, Char* data) override;
	virtual void unmarshal_string (IDL::String& s) override;
	virtual void unmarshal_char_seq (IDL::Sequence <Char>& s) override;
	virtual void unmarshal_wchar (size_t count, WChar* data) override;
	virtual void unmarshal_wstring (IDL::WString& s) override;
	virtual void unmarshal_wchar_seq (IDL::Sequence <WChar>& s) override;
	virtual Internal::Interface::_ref_type unmarshal_interface (const IDL::String& interface_id) override;
	virtual TypeCode::_ref_type unmarshal_type_code () override;
	virtual Internal::Interface::_ref_type unmarshal_value (const IDL::String& interface_id) override;
	virtual Internal::Interface::_ref_type unmarshal_abstract (const IDL::String& interface_id) override;
	virtual void unmarshal_end () override;

	virtual bool marshal_op () override;
	virtual void success () override;
	virtual void set_exception (Any& e) override;
	virtual bool get_exception (Any& e) override;

	bool cancel_internal ();

	void pre_invoke (IdPolicy id_policy);

	virtual void finalize () NIRVANA_NOEXCEPT
	{}

private:
	void preunmarshal (TypeCode::_ptr_type tc, std::vector <Octet>& buf, Internal::IORequest::_ptr_type out);

protected:
	Nirvana::DeadlineTime deadline_;
	const Internal::Operation* metadata_;
	RequestId id_;

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
	}
	status_;

	size_t request_id_offset_;

	Nirvana::Core::Ref <RequestLocalBase> preunmarshaled_;

	std::exception_ptr reply_exception_;

	static std::atomic <IdGenType> last_id_;
};

}
}

#endif
