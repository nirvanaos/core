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
#ifndef NIRVANA_ORB_CORE_REQUESTIN_H_
#define NIRVANA_ORB_CORE_REQUESTIN_H_
#pragma once

#include "Request.h"
#include "../UserObject.h"

namespace CORBA {
namespace Core {

/// Implements server-side IORequest for GIOP.
class NIRVANA_NOVTABLE RequestIn :
	public Request,
	public Nirvana::Core::UserObject
{
public:
	virtual uint32_t request_id () const NIRVANA_NOEXCEPT = 0;
	virtual const IOP::ServiceContextList& service_context () const NIRVANA_NOEXCEPT = 0;
	virtual const IOP::ObjectKey& object_key () const = 0;
	virtual const IDL::String& operation () const NIRVANA_NOEXCEPT = 0;

protected:
	RequestIn (Nirvana::Core::CoreRef <StreamIn>&& in,
		Nirvana::Core::CoreRef <CodeSetConverterW>&& cscw);

	virtual void unmarshal_end () override;
	virtual void marshal_op () override;
	virtual void success () override;
	virtual void cancel () override;

private:
	void switch_to_reply ();
};

/// Implements server-side IORequest for GIOP version.
/// 
/// \typeparam Hdr RequestHeader type.
template <class Hdr>
class NIRVANA_NOVTABLE RequestInVer : public RequestIn
{
protected:
	const Hdr& header () const NIRVANA_NOEXCEPT
	{
		return header_;
	}

	virtual uint32_t request_id () const NIRVANA_NOEXCEPT
	{
		return header_.request_id ();
	}

	virtual const IOP::ServiceContextList& service_context () const NIRVANA_NOEXCEPT
	{
		return header_.service_context ();
	}

	virtual const IDL::String& operation () const NIRVANA_NOEXCEPT
	{
		return header_.operation ();
	}

protected:
	RequestInVer (Nirvana::Core::CoreRef <StreamIn>&& in,
		Nirvana::Core::CoreRef <CodeSetConverterW>&& cscw) :
		RequestIn (std::move (in), std::move (cscw))
	{
		Internal::Type <Hdr>::unmarshal (_get_ptr (), header_);
	}

private:
	Hdr header_;
};

/// Implements server-side IORequest for GIOP 1.0.
class NIRVANA_NOVTABLE RequestIn_1_0 : public RequestInVer <GIOP::RequestHeader_1_0>
{
	typedef RequestInVer <GIOP::RequestHeader_1_0> Base;

public:
	RequestIn_1_0 (Nirvana::Core::CoreRef <StreamIn>&& in) :
		Base (std::move (in), CodeSetConverterW_1_0::get_default (false))
	{}

protected:
	virtual const IOP::ObjectKey& object_key () const
	{
		return header ().object_key ();
	}
};

/// Implements server-side IORequest for GIOP 1.1.
class NIRVANA_NOVTABLE RequestIn_1_1 : public RequestInVer <GIOP::RequestHeader_1_1>
{
	typedef RequestInVer <GIOP::RequestHeader_1_1> Base;

public:
	RequestIn_1_1 (Nirvana::Core::CoreRef <StreamIn>&& in) :
		Base (std::move (in), CodeSetConverterW_1_1::get_default ())
	{}

protected:
	virtual const IOP::ObjectKey& object_key () const
	{
		return header ().object_key ();
	}
};

/// Implements server-side IORequest for GIOP 1.2 and later.
class NIRVANA_NOVTABLE RequestIn_1_2 : public RequestInVer <GIOP::RequestHeader_1_2>
{
	typedef RequestInVer <GIOP::RequestHeader_1_2> Base;

public:
	RequestIn_1_2 (Nirvana::Core::CoreRef <StreamIn>&& in) :
		Base (std::move (in), CodeSetConverterW_1_2::get_default ())
	{}

protected:
	virtual const IOP::ObjectKey& object_key () const;

private:
	static const IOP::ObjectKey& key_from_profile (const IOP::TaggedProfile& profile)
	{
		// TODO: Some check of the profile id?
		return profile.profile_data ();
	}
};

}
}

#endif
