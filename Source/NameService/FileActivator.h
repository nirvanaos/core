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
#ifndef NIRVANA_CORE_FILEACTIVATOR_H_
#define NIRVANA_CORE_FILEACTIVATOR_H_
#pragma once

#include <CORBA/Server.h>
#include <CORBA/PortableServer_s.h>
#include "FileSystem.h"

namespace Nirvana {
namespace Core {

class FileActivator :
	public CORBA::servant_traits <PortableServer::ServantActivator>::Servant <FileActivator>
{
public:
	PortableServer::ServantBase::_ref_type incarnate (const PortableServer::ObjectId& id,
		PortableServer::POA::_ref_type adapter)
	{
		return FileSystem::incarnate (id);
	}

	void etherealize (const PortableServer::ObjectId& id, PortableServer::POA::_ptr_type adapter,
		CORBA::Object::_ptr_type servant, bool cleanup_in_progress, bool remaining_activations)
	{
		FileSystem::etherealize (id, servant);
	}
};

}
}

#endif
