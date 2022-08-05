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
#ifndef NIRVANA_ORB_CORE_OTHERMEMORY_H_
#define NIRVANA_ORB_CORE_OTHERMEMORY_H_
#pragma once

#include "../CoreInterface.h"
#include <Port/ESIOP.h>

namespace Nirvana {
namespace ESIOP {

/// Access to other protection domain memory
class NIRVANA_NOVTABLE OtherMemory
{
	DECLARE_CORE_INTERFACE

public:
	struct Sizes
	{
		size_t block_size;
		size_t sizeof_pointer;
		size_t sizeof_size;
		size_t max_size;
	};

	virtual SharedMemPtr reserve (size_t size) = 0;
	virtual SharedMemPtr copy (SharedMemPtr reserved, void* src, size_t& size, bool release_src) = 0;
	virtual void release (SharedMemPtr p, size_t size) = 0;
	virtual void get_sizes (Sizes& sizes) NIRVANA_NOEXCEPT = 0;
	virtual void* store_pointer (void* where, SharedMemPtr p) NIRVANA_NOEXCEPT = 0;
	virtual void* store_size (void* where, size_t size) NIRVANA_NOEXCEPT = 0;
};

}
}

#endif
