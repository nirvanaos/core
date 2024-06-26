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
#include "../pch.h"
#include "POA_Root.h"
#include <CORBA/Servant_var.h>

namespace PortableServer {
namespace Core {

CORBA::servant_reference <POAManager> POAManagerFactory::create (const IDL::String& id, const CORBA::PolicyList& policies)
{
	Servant_var <POAManager> manager;
	auto ins = managers_.emplace (std::ref (*this), std::ref (id), std::ref (policies));
	if (ins.second)
		manager = &const_cast <POAManager&> (*ins.first);
	return manager;
}

}
}
