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
#ifndef NIRVANA_CORE_COFF_H_
#define NIRVANA_CORE_COFF_H_

#include <core.h>

namespace llvm {
namespace COFF {
struct header;
struct section;
}
}

namespace Nirvana {
namespace Core {

class COFF
{
public:
	typedef llvm::COFF::section Section;

	COFF (const void* addr) NIRVANA_NOEXCEPT :
		hdr_ ((const llvm::COFF::header*)addr)
	{}

	const Section* find_section (const char* name) const NIRVANA_NOEXCEPT;

private:
	static bool is_section (const Section& s, const char* name) NIRVANA_NOEXCEPT;

private:
	const llvm::COFF::header* hdr_;
};

}
}

#endif
