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
#ifndef NIRVANA_CORE_IO_REQUEST_H_
#define NIRVANA_CORE_IO_REQUEST_H_

#include "Future.h"

namespace Nirvana {
namespace Core {

/// Result of an input/output operation.
struct IO_Result
{
	uint32_t size;
	int error;
};

class IO_Request :
	public Future <IO_Result>
{
	typedef Future <IO_Result> Base;
public:
	enum Operation
	{
		OP_READ = 1,
		OP_WRITE = 2,
		OP_SET_SIZE = 3
	};

	IO_Request (Operation operation) NIRVANA_NOEXCEPT :
		operation_ (operation)
	{}

	Operation operation () const NIRVANA_NOEXCEPT
	{
		return operation_;
	}

private:
	Operation operation_;
};

}
}

#endif
