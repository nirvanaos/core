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
#ifndef NIRVANA_ORB_CORE_POA_SYSTEM_H_
#define NIRVANA_ORB_CORE_POA_SYSTEM_H_
#pragma once

#include "POA_Transient.h"
#include "POA_Persistent.h"

namespace PortableServer {
namespace Core {

/// SYSTEM_ID, TRANSIENT
class NIRVANA_NOVTABLE POA_System : public POA_Transient
{
protected:
	POA_System () :
		next_id_ (0)
	{}

	virtual ObjectId generate_object_id ();
	virtual void check_object_id (const ObjectId& oid);

protected:
	typedef unsigned int ID;
	ID next_id_;
};

/// SYSTEM_ID, PERSISTENT
class NIRVANA_NOVTABLE POA_SystemPersistent : public POA_Persistent
{
protected:
	virtual ObjectId generate_object_id ();
	virtual void check_object_id (const ObjectId& oid);
};

}
}

#endif