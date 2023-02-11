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
#include "POA_Root.h"
#include "POA_Activator.h"
#include "POA_Persistent.h"
#include "POA_Default.h"
#include "POA_Locator.h"

using namespace CORBA;

namespace PortableServer {
namespace Core {

template <class ... Bases>
class POA_Impl :
	public Bases...,
	public virtual POA_Base
{
public:
	POA_Impl (POA_Base * parent, const IDL::String * name,
		servant_reference <POAManager>&& manager,
		servant_reference <CORBA::Core::DomainManager>&& domain_manager) :
		POA_Base (parent, name, std::move (manager), std::move (domain_manager))
	{}
};

template <class ... Bases>
POA_Ref POA_create (POA_Base* parent, const IDL::String* name,
	servant_reference <POAManager>&& manager,
	servant_reference <CORBA::Core::DomainManager>&& domain_manager)
{
	return make_reference <POA_Impl <Bases...> > (parent, name, std::move (manager),
		std::move (domain_manager));
}

#define POLICY(ls, idu, ida, ia, sr, rp) { LifespanPolicyValue::ls, IdUniquenessPolicyValue::idu,\
IdAssignmentPolicyValue::ida, ImplicitActivationPolicyValue::ia, ServantRetentionPolicyValue::sr,\
RequestProcessingPolicyValue::rp }

const POA_Policies POA_Policies::default_ =
POLICY (TRANSIENT, MULTIPLE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY);

const POA_FactoryEntry POA_Base::factories_ [] = {
	{ // 0 0 0 1 0 0
		POLICY (TRANSIENT, UNIQUE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
		POA_create <POA_Transient, POA_Unique>
	},
	{ // 0 0 0 1 0 1
		POLICY (TRANSIENT, UNIQUE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
		POA_create <POA_Transient, POA_Unique, POA_Default>
	},
	{ // 0 0 0 1 0 2
		POLICY (TRANSIENT, UNIQUE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
		POA_create <POA_Transient, POA_Unique, POA_Activator>
	},
	{ // 0 0 1 0 0 0
		POLICY (TRANSIENT, UNIQUE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
		POA_create <POA_ImplicitUnique> // RootPOA type
	},
	{ // 0 0 1 0 0 1
		POLICY (TRANSIENT, UNIQUE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
		POA_create <POA_ImplicitUnique, POA_Default>
	},
	{ // 0 0 1 0 0 2
		POLICY (TRANSIENT, UNIQUE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
		POA_create <POA_System, POA_Unique, POA_Activator, POA_Implicit>
	},
	{ // 0 0 1 1 0 0
		POLICY (TRANSIENT, UNIQUE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
		POA_create <POA_System, POA_Unique>
	},
	{ // 0 0 1 1 0 1
		POLICY (TRANSIENT, UNIQUE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
		POA_create <POA_System, POA_Unique, POA_Default>
	},
	{ // 0 0 1 1 0 2
		POLICY (TRANSIENT, UNIQUE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
		POA_create <POA_System, POA_Unique, POA_Activator>
	},
	{ // 0 1 0 1 0 0
		POLICY (TRANSIENT, MULTIPLE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
		POA_create <POA_Transient, POA_Retain>
	},
	{ // 0 1 0 1 0 1
		POLICY (TRANSIENT, MULTIPLE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
		POA_create <POA_Transient, POA_Retain, POA_Default>
	},
	{ // 0 1 0 1 0 2
		POLICY (TRANSIENT, MULTIPLE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
		POA_create <POA_Transient, POA_Retain, POA_Activator>
	},
	{ // 0 1 0 1 1 1
		POLICY (TRANSIENT, MULTIPLE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, NON_RETAIN, USE_DEFAULT_SERVANT),
		POA_create <POA_Transient, POA_Default>
	},
	{ // 0 1 0 1 1 2
		POLICY (TRANSIENT, MULTIPLE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, NON_RETAIN, USE_SERVANT_MANAGER),
		POA_create <POA_Transient, POA_Locator>
	},
	{ // 0 1 1 0 0 0
		POLICY (TRANSIENT, MULTIPLE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
		POA_create <POA_System, POA_Retain, POA_Implicit>
	},
	{ // 0 1 1 0 0 1
		POLICY (TRANSIENT, MULTIPLE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
		POA_create <POA_System, POA_Retain, POA_Implicit, POA_Default>
	},
	{ // 0 1 1 0 0 2
		POLICY (TRANSIENT, MULTIPLE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
		POA_create <POA_System, POA_Retain, POA_Activator, POA_Implicit>
	},
	{ // 0 1 1 1 0 0
		POLICY (TRANSIENT, MULTIPLE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
		POA_create <POA_System, POA_Retain>
	},
	{ // 0 1 1 1 0 1
		POLICY (TRANSIENT, MULTIPLE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
		POA_create <POA_System, POA_Retain, POA_Default>
	},
	{ // 0 1 1 1 0 2
		POLICY (TRANSIENT, MULTIPLE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
		POA_create <POA_System, POA_Retain, POA_Activator>
	},
	{ // 1 0 0 1 0 0
		POLICY (PERSISTENT, UNIQUE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
		POA_create <POA_Persistent, POA_Unique>
	},
	{ // 1 0 0 1 0 1
		POLICY (PERSISTENT, UNIQUE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
		POA_create <POA_Persistent, POA_Unique, POA_Default>
	},
	{ // 1 0 0 1 0 2
		POLICY (PERSISTENT, UNIQUE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
		POA_create <POA_Persistent, POA_Unique, POA_Activator>
	},
	{ // 1 0 1 0 0 0
		POLICY (PERSISTENT, UNIQUE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
		POA_create <POA_SystemPersistent, POA_Unique, POA_Implicit>
	},
	{ // 1 0 1 0 0 1
		POLICY (PERSISTENT, UNIQUE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
		POA_create <POA_SystemPersistent, POA_Unique, POA_Implicit, POA_Default>
	},
	{ // 1 0 1 0 0 2
		POLICY (PERSISTENT, UNIQUE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
		POA_create <POA_SystemPersistent, POA_Unique, POA_Activator, POA_Implicit>
	},
	{ // 1 0 1 1 0 0
		POLICY (PERSISTENT, UNIQUE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
		POA_create <POA_SystemPersistent, POA_Unique>
	},
	{ // 1 0 1 1 0 1
		POLICY (PERSISTENT, UNIQUE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
		POA_create <POA_SystemPersistent, POA_Unique, POA_Default>
	},
	{ // 1 0 1 1 0 2
		POLICY (PERSISTENT, UNIQUE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
		POA_create <POA_SystemPersistent, POA_Unique, POA_Activator>
	},
	{ // 1 1 0 1 0 0
		POLICY (PERSISTENT, MULTIPLE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
		POA_create <POA_Persistent, POA_Retain>
	},
	{ // 1 1 0 1 0 1
		POLICY (PERSISTENT, MULTIPLE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
		POA_create <POA_Persistent, POA_Retain, POA_Default>
	},
	{ // 1 1 0 1 0 2
		POLICY (PERSISTENT, MULTIPLE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
		POA_create <POA_Persistent, POA_Retain, POA_Activator>
	},
	{ // 1 1 0 1 1 1
		POLICY (PERSISTENT, MULTIPLE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, NON_RETAIN, USE_DEFAULT_SERVANT),
		POA_create <POA_Persistent, POA_Default>
	},
	{ // 1 1 0 1 1 2
		POLICY (PERSISTENT, MULTIPLE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, NON_RETAIN, USE_SERVANT_MANAGER),
		POA_create <POA_Persistent, POA_Locator>
	},
	{ // 1 1 1 1 0 0
		POLICY (PERSISTENT, MULTIPLE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
		POA_create <POA_SystemPersistent, POA_Retain>
	},
	{ // 1 1 1 1 0 1
		POLICY (PERSISTENT, MULTIPLE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
		POA_create <POA_SystemPersistent, POA_Retain, POA_Default>
	},
	{ // 1 1 1 1 0 2
		POLICY (PERSISTENT, MULTIPLE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
		POA_create <POA_SystemPersistent, POA_Retain, POA_Activator>
	},
	{ // 1 1 1 1 1 1
		POLICY (PERSISTENT, MULTIPLE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, NON_RETAIN, USE_DEFAULT_SERVANT),
		POA_create <POA_Transient, POA_Default>
	},
	{ // 1 1 1 1 1 2
		POLICY (PERSISTENT, MULTIPLE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, NON_RETAIN, USE_SERVANT_MANAGER),
		POA_create <POA_Transient, POA_Locator>
	}
};

const size_t POA_Base::FACTORY_COUNT = countof (POA_Base::factories_);

}
}
