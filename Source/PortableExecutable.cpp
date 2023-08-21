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
#include "PortableExecutable.h"
#include <Nirvana/throw_exception.h>
#include <Nirvana/OLF.h>
#include "llvm/BinaryFormat/COFF.h"

namespace Nirvana {
namespace Core {

const void* PortableExecutable::get_COFF (const void* base_address)
{
	const llvm::COFF::DOSHeader* dos_header = (const llvm::COFF::DOSHeader*)base_address;

	static const char DOSMagic [] = { 'M', 'Z' };

	if (dos_header->Magic != *(const uint16_t*)DOSMagic)
		throw_BAD_PARAM (make_minor_errno (ENOEXEC));

	const uint32_t* PE_magic = (const uint32_t*)((const uint8_t*)base_address + dos_header->AddressOfNewExeHeader);
	if (*PE_magic != *(const uint32_t*)llvm::COFF::PEMagic)
		throw_BAD_PARAM (make_minor_errno (ENOEXEC));

	return PE_magic + 1;
}

PortableExecutable::PortableExecutable (const void* base_address) :
	COFF (get_COFF (base_address)),
	base_address_ (base_address)
{}

bool PortableExecutable::find_OLF_section (Core::Section& section) const noexcept
{
	const COFF::Section* ps = COFF::find_section (OLF_BIND);
	if (ps) {
		section.address = section_address (*ps);
		section.size = ps->VirtualSize;
		return true;
	}
	return false;
}

}
}
