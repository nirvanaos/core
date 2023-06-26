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
#include <fnctl.h>

namespace Nirvana {
namespace Core {

class FileAccessBase
{
public:
	static const unsigned MASK_SHIFT = 2;

	enum AccessMask
	{
		READ  = 0x01,
		WRITE = 0x02,
		DENY_READ = READ << MASK_SHIFT,
		DENY_WRITE = WRITE << MASK_SHIFT
	};

	unsigned access_mask () const noexcept
	{
		return access_mask_;
	}

	static unsigned get_access_mask (unsigned flags) noexcept
	{
		if (flags & O_RDWR)
			return AccessMask::READ | AccessMask::WRITE;
		else if (flags & O_WRONLY)
			return AccessMask::WRITE;
		else
			return AccessMask::READ;
	}

	static unsigned get_deny_mask (unsigned flags) noexcept
	{
		unsigned mask = 0;
		if (flags & FILE_SHARE_DENY_READ)
			mask |= AccessMask::READ;
		if (flags & FILE_SHARE_DENY_WRITE)
			mask |= AccessMask::WRITE;
		return mask;
	}

protected:
	FileAccessBase (unsigned access_mask = 0) :
		access_mask_ (access_mask)
	{}

protected:
	unsigned access_mask_;
};

}
}

#endif
