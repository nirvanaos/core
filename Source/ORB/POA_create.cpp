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
		servant_reference <POAManager> && manager) :
		POA_Base (parent, name, std::move (manager))
	{}
};

template <class ... Bases>
POA_Ref POA_create (POA_Base* parent, const IDL::String* name,
	servant_reference <POAManager>&& manager, const CORBA::PolicyList&)
{
	return make_reference <POA_Impl <Bases...> > (parent, name, std::move (manager));
}

#define POLICY(ls, idu, ida, ia, sr, rp) { LifespanPolicyValue::ls, IdUniquenessPolicyValue::idu,\
IdAssignmentPolicyValue::ida, ImplicitActivationPolicyValue::ia, ServantRetentionPolicyValue::sr,\
RequestProcessingPolicyValue::rp }

const POA_Policies POA_Policies::default_ =
POLICY (TRANSIENT, MULTIPLE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY);

const POA_FactoryEntry POA_Base::factories_ [] = {

	// TRANSIENT, UNIQUE_ID

{
	POLICY (TRANSIENT, UNIQUE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
	POA_create <POA_Transient, POA_Unique>
},
/*{
	POLICY (TRANSIENT, UNIQUE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
	Not yet supported
},*/
{
	POLICY (TRANSIENT, UNIQUE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
	POA_create <POA_Transient, POA_Unique, POA_Activator>
},

{
	POLICY (TRANSIENT, UNIQUE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
	POA_create <POA_System, POA_Unique>
},
/*{
	POLICY (TRANSIENT, UNIQUE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
	Not yet supported
},*/
{
	POLICY (TRANSIENT, UNIQUE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
	POA_create <POA_System, POA_Unique, POA_Activator>
},

{
	POLICY (TRANSIENT, UNIQUE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
	POA_create <POA_ImplicitUnique> // RootPOA type
},
/*{
	POLICY (TRANSIENT, UNIQUE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
	Not yet supported
},*/
{
	POLICY (TRANSIENT, UNIQUE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
	POA_create <POA_System, POA_Unique, POA_Activator, POA_Implicit>
},

	// TRANSIENT, MUPLTIPLE_ID

{
	POLICY (TRANSIENT, MULTIPLE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
	POA_create <POA_Transient, POA_Retain>
},
/*{
	POLICY (TRANSIENT, MULTIPLE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
	Not yet supported
},*/
{
	POLICY (TRANSIENT, MULTIPLE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
	POA_create <POA_Transient, POA_Retain, POA_Activator>
},
/*{
	POLICY (PERSISTENT, MULTIPLE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, NON_RETAIN, USE_SERVANT_MANAGER),
	Not yet supported: ServantLocator
},*/

{
	POLICY (TRANSIENT, MULTIPLE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
	POA_create <POA_System, POA_Retain>
},
/*{
	POLICY (TRANSIENT, MULTIPLE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
	Not yet supported
},*/
{
	POLICY (TRANSIENT, MULTIPLE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
	POA_create <POA_System, POA_Retain, POA_Activator>
},

{
	POLICY (TRANSIENT, MULTIPLE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
	POA_create <POA_System, POA_Retain, POA_Implicit>
},
/*{
	POLICY (TRANSIENT, MULTIPLE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
	Not yet supported
},*/
{
	POLICY (TRANSIENT, MULTIPLE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
	POA_create <POA_System, POA_Retain, POA_Activator, POA_Implicit>
},
/*{
	POLICY (TRANSIENT, MULTIPLE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, NON_RETAIN, USE_SERVANT_MANAGER),
	Not yet supported: ServantLocator
},*/

// PERSISTENT, UNIQUE_ID

{
	POLICY (PERSISTENT, UNIQUE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
	POA_create <POA_Persistent, POA_Unique>
},
/*{
	POLICY (PERSISTENT, UNIQUE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
	Not yet supported
},*/
{
	POLICY (PERSISTENT, UNIQUE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
	POA_create <POA_Persistent, POA_Unique, POA_Activator>
},

{
	POLICY (PERSISTENT, UNIQUE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
	POA_create <POA_SystemPersistent, POA_Unique>
},
/*{
	POLICY (PERSISTENT, UNIQUE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
	Not yet supported
},*/
{
	POLICY (PERSISTENT, UNIQUE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
	POA_create <POA_SystemPersistent, POA_Unique, POA_Activator>
},

{
	POLICY (PERSISTENT, UNIQUE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
	POA_create <POA_SystemPersistent, POA_Unique>
},
/*{
	POLICY (PERSISTENT, UNIQUE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
	Not yet supported
},*/
{
	POLICY (PERSISTENT, UNIQUE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
	POA_create <POA_SystemPersistent, POA_Unique, POA_Activator, POA_Implicit>
},

// PERSISTENT, MUPLTIPLE_ID

{
	POLICY (PERSISTENT, MULTIPLE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
	POA_create <POA_Persistent, POA_Retain>
},
/*{
	POLICY (PERSISTENT, MULTIPLE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
	Not yet supported
},*/
{
	POLICY (PERSISTENT, MULTIPLE_ID, USER_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
	POA_create <POA_Persistent, POA_Retain, POA_Activator>
},
/*{
	POLICY (PERSISTENT, MULTIPLE_ID, USER_ID, IMPLICIT_ACTIVATION, NON_RETAIN, USE_SERVANT_MANAGER),
	Not yet supported: ServantLocator
},*/

{
	POLICY (PERSISTENT, MULTIPLE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
	POA_create <POA_SystemPersistent, POA_Retain>
},
/*{
	POLICY (PERSISTENT, MULTIPLE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
	Not yet supported
},*/
{
	POLICY (PERSISTENT, MULTIPLE_ID, SYSTEM_ID, NO_IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
	POA_create <POA_SystemPersistent, POA_Retain, POA_Activator>
},

{
	POLICY (PERSISTENT, UNIQUE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_ACTIVE_OBJECT_MAP_ONLY),
	POA_create <POA_SystemPersistent, POA_Unique, POA_Implicit>
},
/*{
	POLICY (PERSISTENT, UNIQUE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_DEFAULT_SERVANT),
	Not yet supported
},*/
{
	POLICY (PERSISTENT, UNIQUE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, RETAIN, USE_SERVANT_MANAGER),
	POA_create <POA_SystemPersistent, POA_Unique, POA_Activator, POA_Implicit>
},
/*{
	POLICY (PERSISTENT, MULTIPLE_ID, SYSTEM_ID, IMPLICIT_ACTIVATION, NON_RETAIN, USE_SERVANT_MANAGER),
	Not yet supported: ServantLocator
},*/

};

const size_t POA_Base::FACTORY_COUNT = std::size (POA_Base::factories_);

}
}
