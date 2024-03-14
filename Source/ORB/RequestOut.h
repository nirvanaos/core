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
#include "RequestLocalRoot.h"
#include "../ExecDomain.h"
#include "../UserObject.h"
#include "../Timer.h"
#include "../Security.h"

namespace CORBA {
namespace Core {

/// Implements client-side IORequest for GIOP.
class NIRVANA_NOVTABLE RequestOut : public RequestGIOP
{
	typedef RequestGIOP Base;
	static const unsigned FLAG_PREUNMARSHAL = 8;

	static const TimeBase::TimeT DEFAULT_TIMEOUT = 10 * TimeBase::MINUTE;
	static const TimeBase::TimeT MIN_TIMEOUT = 1 * TimeBase::SECOND;

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

	static RequestId get_new_id (IdPolicy id_policy) noexcept;

	RequestId id () const noexcept
	{
		return id_;
	}

	void id (RequestId id);

	virtual void set_reply (unsigned status, IOP::ServiceContextList&& context,
		servant_reference <StreamIn>&& stream);

	void set_system_exception (SystemException::Code code, uint32_t minor, CompletionStatus completed)
		noexcept;

protected:
	RequestOut (unsigned response_flags, Domain& target_domain,
		const Internal::Operation& metadata, ReferenceRemote* ref);
	~RequestOut ();

	void write_header (const IOP::ObjectKey& object_key, IOP::ServiceContextList& context);

	virtual bool unmarshal (size_t align, size_t size, void* data) override;
	virtual bool unmarshal_seq (size_t align, size_t element_size, size_t CDR_element_size,
		size_t& element_count, void*& data, size_t& allocated_size) override;
	virtual size_t unmarshal_seq_begin() override;
	virtual void unmarshal_string (IDL::String& s) override;
	virtual void unmarshal_wchar (size_t count, WChar* data) override;
	virtual void unmarshal_wstring (IDL::WString& s) override;
	virtual void unmarshal_wchar_seq (IDL::Sequence <WChar>& s) override;
	virtual void unmarshal_interface (const IDL::String& interface_id, Internal::Interface::_ref_type& itf) override;
	virtual void unmarshal_type_code (TypeCode::_ref_type& tc) override;
	virtual void unmarshal_value (const IDL::String& interface_id, Internal::Interface::_ref_type& val) override;
	virtual void unmarshal_abstract (const IDL::String& interface_id, Internal::Interface::_ref_type& itf) override;
	virtual void unmarshal_end () override;

	virtual bool marshal_op () override;
	virtual void success () override;
	virtual void set_exception (Any& e) override;
	virtual bool get_exception (Any& e) override;

	bool cancel_internal ();

	void pre_invoke (IdPolicy id_policy);

	virtual void finalize () noexcept
	{
		timer_.cancel ();
	}

	const Internal::Operation& metadata () const noexcept
	{
		return metadata_;
	}

	void clear_preunmarshal () noexcept
	{
		response_flags_ &= ~FLAG_PREUNMARSHAL;
	}

private:
	const Internal::Operation& metadata_;

	// Hold reference alive while request exists
	servant_reference <ReferenceRemote> ref_;
	
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

	servant_reference <RequestLocalRoot> preunmarshaled_;

	SystemException::Code system_exception_code_;
	SystemException::_Data system_exception_data_;

	class Timer : public Nirvana::Core::Timer
	{
	public:
		void set (TimeBase::TimeT timeout, RequestId id, Nirvana::Core::Heap& heap)
		{
			id_ = id;
			timeout_ = timeout;
			heap_ = (&heap);
			Nirvana::Core::Timer::set (0, timeout, 0);
		}

	protected:
		virtual void signal () noexcept override;

	private:
		RequestId id_;
		servant_reference <Nirvana::Core::Heap> heap_;
		TimeBase::TimeT timeout_;
	};

	class Timeout;

	Timer timer_;

	static std::atomic <IdGenType> last_id_;
};

}
}

#endif
