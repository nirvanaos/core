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
#include "ServantBase.h"
#include "ServantProxyObjectImplicit.h"
#include "POA_Root.h"
#include <CORBA/Proxy/TypeCodeNative.h>

using namespace CORBA::Internal;
using namespace CORBA;
using namespace Nirvana::Core;
using namespace Nirvana;

namespace PortableServer {
namespace Core {

typedef TypeCodeNative <PortableServer::ServantBase> TC_Servant;

class ServantBaseImplicit :
	public ServantBase,
	public CORBA::Core::ServantProxyObjectImplicit
{
public:
	ServantBaseImplicit (Servant user_servant, POA::_ptr_type adapter) :
		ServantBase (static_cast <CORBA::Core::ServantProxyObject&> (*this)),
		CORBA::Core::ServantProxyObjectImplicit (*this, user_servant, adapter)
	{}
};

class ServantBaseExplicit :
	public ServantBase,
	public CORBA::Core::ServantProxyObject
{
public:
	ServantBaseExplicit (Servant user_servant) :
		ServantBase (static_cast <CORBA::Core::ServantProxyObject&> (*this)),
		CORBA::Core::ServantProxyObject (*this, user_servant)
	{}
};

ServantBase* ServantBase::create (Servant user_servant)
{
	POA::_ref_type adapter = user_servant->_default_POA ();
	if (adapter && POA_Base::implicit_activation (adapter) == POA_Base::IMPLICIT_ACTIVATION_WITH_DGC)
		return new ServantBaseImplicit (user_servant, adapter);
	else
		return new ServantBaseExplicit (user_servant);
}

}
}

namespace CORBA {
namespace Internal {
template <>
const Char TypeCodeName <PortableServer::ServantBase>::name_ [] = "Servant";
}
}

NIRVANA_SELECTANY
const Nirvana::ImportInterfaceT <CORBA::TypeCode>
	PortableServer::_tc_Servant = { Nirvana::OLF_IMPORT_INTERFACE, nullptr, nullptr,
	NIRVANA_STATIC_BRIDGE (CORBA::TypeCode, PortableServer::Core::TC_Servant) };
