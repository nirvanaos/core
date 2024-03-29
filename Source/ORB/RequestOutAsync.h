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
#ifndef NIRVANA_ORB_CORE_REQUESTOUTASYNC_H_
#define NIRVANA_ORB_CORE_REQUESTOUTASYNC_H_
#pragma once

#include <CORBA/Server.h>
#include "CallbackRef.h"
#include "../ExecDomain.h"

namespace CORBA {
namespace Core {

/// \brief Asynchronous outgoing request.
/// 
/// \typeparam Rq Base request class, derived from RequestOut.
template <class Rq>
class RequestOutAsync : public Rq
{
	typedef Rq Base;

public:
	RequestOutAsync (unsigned response_flags, Domain& target_domain,
		const IOP::ObjectKey& object_key, const Internal::Operation& metadata, ReferenceRemote* ref,
		CallbackRef&& callback) :
		Base (response_flags, target_domain, object_key, metadata, ref),
		callback_ (std::move (callback))
	{
		assert (callback_);
		Base::deadline_ = Nirvana::Core::ExecDomain::current ().get_request_deadline (false);
	}

private:
	virtual void finalize () noexcept override
	{
		Base::finalize ();
		callback_.call (Base::metadata (), Base::_get_ptr ());
	}

private:
	CallbackRef callback_;
};

}
}

#endif
