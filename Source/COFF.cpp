/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
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
#include "llvm/BinaryFormat/COFF.h"

namespace Nirvana {
namespace Core {

inline
bool COFF::is_section (const Section& s, const char* name) NIRVANA_NOEXCEPT
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

const COFF::Section* COFF::find_section (const char* name) const NIRVANA_NOEXCEPT
{
	const Section* s = (const Section*)((const uint8_t*)(hdr_ + 1) + hdr_->SizeOfOptionalHeader);
	for (uint32_t cnt = hdr_->NumberOfSections; cnt; --cnt, ++s) {
		if (is_section (*s, name))
			return s;
	}

	return nullptr;
}

}
}
