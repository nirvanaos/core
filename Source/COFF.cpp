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
#include "COFF.h"

namespace Nirvana {
namespace Core {

const COFF::PE32Header* COFF::pe32_header () const noexcept
{
	const Header* hdr = header ();
	if (hdr->SizeOfOptionalHeader >= sizeof (PE32Header)) {
		const PE32Header* pehdr = (const PE32Header*)(hdr + 1);
		if (pehdr->Magic == PE32Header::PE32 || pehdr->Magic == PE32Header::PE32_PLUS)
			return pehdr;
	}
	return nullptr;
}

const COFF::Section* COFF::sections () const noexcept
{
	return (const Section*)((const uint8_t*)(hdr_ + 1) + hdr_->SizeOfOptionalHeader);
}

bool COFF::is_section (const Section& s, const char* name) noexcept
{
	const char* sn = s.Name;
	char c;
	while ((c = *name) && *sn == c) {
		++sn;
		++name;
	}
	if (c)
		return false;
	if (sn - s.Name < llvm::COFF::NameSize)
		return !*sn;
	else
		return true;
}

const COFF::Section* COFF::find_section (const char* name) const noexcept
{
	for (const Section* s = sections (), *end = s + section_count (); s != end; ++s) {
		if (is_section (*s, name))
			return s;
	}

	return nullptr;
}

}
}
