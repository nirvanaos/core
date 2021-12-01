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
#ifndef NIRVANA_CORE_PORTABLEEXECUTABLE_H_
#define NIRVANA_CORE_PORTABLEEXECUTABLE_H_

#include "COFF.h"
#include "Section.h"

namespace Nirvana {
namespace Core {

class PortableExecutable : public COFF
{
public:
	PortableExecutable (const void* base_address);

	const void* base_address () const NIRVANA_NOEXCEPT
	{
		return base_address_;
	}

	const void* section_address (const COFF::Section& s) const NIRVANA_NOEXCEPT
	{
		return (const uint8_t*)base_address_ + s.VirtualAddress;
	}

	bool find_OLF_section (Core::Section& section) const NIRVANA_NOEXCEPT;

private:
	static const void* get_COFF (const void* base_address);

private:
	const void* base_address_;
};


}
}

#endif
