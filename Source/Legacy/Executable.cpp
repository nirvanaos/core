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
#include "Executable.h"
#include "../Binder.inl"

namespace Nirvana {
namespace Legacy {
namespace Core {

Executable::Executable (const std::string& file) :
	Nirvana::Core::Binary (file),
	entry_point_ (Nirvana::Core::Binder::bind (*this)),
	bound_ (true)
{}

Executable::~Executable ()
{
	if (bound_)
		Nirvana::Core::Binder::unbind (*this);
}

void Executable::unbind () NIRVANA_NOEXCEPT
{
	if (bound_) {
		bound_ = false;
		Nirvana::Core::Binder::unbind (*this);
	}
}

Nirvana::Core::Binary* Executable::binary () NIRVANA_NOEXCEPT
{
	return this;
}

void Executable::raise_exception (CORBA::SystemException::Code code, unsigned minor)
{
	CORBA::Internal::Bridge <Main>* br = static_cast <CORBA::Internal::Bridge <Main>*> (&entry_point_);
	br->_epv ().epv.raise_exception (br, (short)code, (unsigned short)minor, nullptr);
}


}
}
}
