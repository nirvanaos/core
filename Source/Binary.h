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

#include <Port/Module.h>

namespace Nirvana {
namespace Core {

/// Binary: Nirvana::Module interface implementation.
class Binary :
	public Port::Module
{
public:
	const void* base_address () const noexcept
	{
		return Port::Module::address ();
	}

	/// \brief Raise system exception in the binary object (Module or Executable).
	/// 
	/// \code  System exception code.
	/// \minor System exception minor code.
	virtual void raise_exception (CORBA::SystemException::Code code, unsigned minor) = 0;

protected:
	Binary (AccessDirect::_ptr_type file) :
		Nirvana::Core::Port::Module (file)
	{}

	Binary (Port::Module&& mod) :
		Nirvana::Core::Port::Module (std::move (mod))
	{}
};

}
}

#endif
