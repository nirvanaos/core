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

#include "Services.h"
#include "POA_Root.h"

using namespace Nirvana::Core;
using namespace Nirvana;

namespace CORBA {
namespace Core {

// Initial services. Must be lexicographically ordered.

const Services::Factory Services::factories_ [Service::SERVICE_COUNT] = {
	{ "RootPOA", create_RootPOA, System::MILLISECOND }
};

// Service factories

Object::_ref_type Services::create_RootPOA ()
{
	return make_reference <PortableServer::Core::POA_Root> ()->_this ();
}

}
}
