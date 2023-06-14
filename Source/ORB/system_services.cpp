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
#include "system_services.h"
#include "ESIOP.h"

namespace CORBA {
namespace Core {

Services::Service system_service (const Octet* key, size_t key_len) noexcept
{
	assert (ESIOP::is_system_domain ());
	if (SysObjectKey <Nirvana::Core::SysDomain>::equal (key, key_len))
		return Services::SysDomain;
	else if (SysObjectKey <CosNaming::Core::NameService>::equal (key, key_len))
		return Services::NameService;
	else
		return Services::SERVICE_COUNT;
}

}
}
