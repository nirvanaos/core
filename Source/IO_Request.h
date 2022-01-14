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
#pragma once

#include "Event.h"

namespace Nirvana {
namespace Core {

struct IO_Result
{
	uint32_t size;
	int error;
};

/// Input/output request.
class IO_Request :
	public Event
{
public:
	/// I/O operation code.
	enum Operation
	{
		OP_READ = 1,
		OP_WRITE = 2,
		OP_SET_SIZE = 3
	};

	IO_Request (Operation operation) NIRVANA_NOEXCEPT :
		operation_ (operation)
	{}

	/// \returns I/O operation code.
	Operation operation () const NIRVANA_NOEXCEPT
	{
		return operation_;
	}

	void signal (const IO_Result& result) NIRVANA_NOEXCEPT
	{
		result_ = result;
		Event::signal ();
	}

	const IO_Result& result () const NIRVANA_NOEXCEPT
	{
		return result_;
	}

private:
	const Operation operation_;
	IO_Result result_;
};

}
}

#endif
