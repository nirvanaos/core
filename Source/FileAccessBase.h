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
#ifndef NIRVANA_CORE_FILEACCESSBASE_H_
#define NIRVANA_CORE_FILEACCESSBASE_H_
#pragma once

#include <stdint.h>

namespace Nirvana {
namespace Core {

class FileAccessBase
{
public:
	static const unsigned MASK_SHIFT = 2;

	enum AccessMask
	{
		ALLOW_READ  = 0x01,
		ALLOW_WRITE = 0x02,
		DENY_READ = ALLOW_READ << MASK_SHIFT,
		DENY_WRITE = ALLOW_WRITE << MASK_SHIFT
	};

	uint_fast16_t mask () const noexcept
	{
		return mask_;
	}

protected:
	uint_fast16_t mask_;
};

}
}

#endif
