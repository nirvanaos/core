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
#ifndef NIRVANA_ORB_CORE_REQUESTOUTESIOP_H_
#define NIRVANA_ORB_CORE_REQUESTOUTESIOP_H_
#pragma once

#include "RequestOut.h"
#include "StreamOutSM.h"
#include "OutgoingRequests.h"

using namespace CORBA;
using namespace CORBA::Core;
using namespace Nirvana;
using namespace Nirvana::Core;

namespace ESIOP {

class RequestOut : public CORBA::Core::RequestOut
{
	typedef CORBA::Core::RequestOut Base;

public:
	RequestOut (DomainLocal& domain, IOP::ObjectKey object_key,
		const CORBA::Internal::Operation& metadata, unsigned response_flags,
		IOP::ServiceContextList context) :
		Base ((response_flags & 3) == 1 ? 2 : 1, response_flags, metadata),
		domain_ (&domain)
	{
		stream_out_ = CoreRef <StreamOut>::create <ImplDynamic <StreamOutSM> > (std::ref (domain));
		if (response_flags & 3)
			id_ = OutgoingRequests::new_request (*this, OutgoingRequests::IdPolicy::ANY);
		IDL::String operation = metadata.name;
		DeadlineTime dl = deadline ();
		if (INFINITE_DEADLINE != dl) {
			ImplStatic <StreamOutEncap> dl;
			dl.write_c (alignof (DeadlineTime), sizeof (DeadlineTime), &dl);
			context.emplace_back ();
			context.back ().context_id (CONTEXT_ID_DEADLINE);
			context.back ().context_data (std::move (dl.data ()));
		}
		write_header (object_key, operation, context);
	}

protected:
	virtual void invoke () override;
	virtual void cancel () override;

private:
	CORBA::servant_reference <DomainLocal> domain_;
};

}

#endif
