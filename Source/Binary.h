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
#ifndef NIRVANA_CORE_BINARY_H_
#define NIRVANA_CORE_BINARY_H_
#pragma once

#include <CORBA/Server.h>
#include <Nirvana/Module_s.h>
#include <Port/Module.h>

namespace Nirvana {
namespace Core {

/// Binary: Nirvana::Module interface implementation.
class Binary :
	public Port::Module
{
public:
	const void* base_address () const NIRVANA_NOEXCEPT
	{
		return Port::Module::address ();
	}

protected:
	Binary (const StringView& file) :
		Nirvana::Core::Port::Module (file)
	{}
};

}
}

#endif
