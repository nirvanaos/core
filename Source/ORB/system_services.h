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
#ifndef NIRVANA_ORB_CORE_SYSTEM_SERVICES_H_
#define NIRVANA_ORB_CORE_SYSTEM_SERVICES_H_
#pragma once

#include "SysObjectKey.h"

namespace Nirvana {
namespace Core {

class SysDomain;
class ProtDomain;

}
}

namespace CosNaming {
namespace Core {

class NameService;

}
}

namespace CORBA {
namespace Core {

template <>
const Octet SysObjectKey <Nirvana::Core::SysDomain>::key [] = { 0 };

template <>
const Octet SysObjectKey <Nirvana::Core::ProtDomain>::key [] = { 1 };

template <>
const Octet SysObjectKey <CosNaming::Core::NameService>::key [] =
{ 'N', 'a', 'm', 'e', 'S', 'e', 'r', 'v', 'i', 'c', 'e'};

bool is_sys_domain_object (const Octet* key, size_t key_len) noexcept;

inline bool is_sys_domain_object (const IOP::ObjectKey& object_key) noexcept
{
	return is_sys_domain_object (object_key.data (), object_key.size ());
}

}
}

#endif
