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
#ifndef NIRVANA_ORB_CORE_PROXYOBJECTIMPLICIT_H_
#define NIRVANA_ORB_CORE_PROXYOBJECTIMPLICIT_H_
#pragma once

#include "ReferenceLocal.h"

namespace CORBA {
namespace Core {

/// \brief Server-side Object proxy with implicit activation.
class NIRVANA_NOVTABLE ProxyObjectImplicit : public ProxyObject
{
	typedef ProxyObject Base;

protected:
	ProxyObjectImplicit (PortableServer::Core::ServantBase& core_servant,
		PortableServer::Servant user_servant, PortableServer::POA::_ptr_type adapter) :
		Base (core_servant, user_servant),
		adapter_ (adapter)
	{}

	~ProxyObjectImplicit ()
	{
		ReferenceLocal* ref = reference_.exchange (nullptr);
		if (ref)
			ref->on_delete_implicit (core_servant ());
	}

private:
	virtual void activate (ReferenceLocal& reference) NIRVANA_NOEXCEPT override;
	virtual void add_ref_1 () override;

private:
	PortableServer::POA::_ref_type adapter_;
};

}
}

#endif
