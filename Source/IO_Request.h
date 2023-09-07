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
#include "UserObject.h"

namespace Nirvana {
namespace Core {

struct IO_Result
{
	uint32_t size;
	int error;

	IO_Result (uint32_t cb, int err) :
		size (cb),
		error (err)
	{}
};

/// Input/output request.
class NIRVANA_NOVTABLE IO_Request :
	public Event,
	public UserObject
{
public:
	void signal (const IO_Result& result) noexcept
	{
		result_ = result;
		Event::signal ();
	}

	const IO_Result& result () const noexcept
	{
		return result_;
	}

	virtual void cancel () noexcept = 0;

protected:
	IO_Request () noexcept :
		result_ (0, 0)
	{}

	virtual ~IO_Request ()
	{}

	template <class> friend class CORBA::servant_reference;

	void _add_ref () noexcept
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () noexcept
	{
		if (!ref_cnt_.decrement ()) {
			wait ();
			delete this;
		}
	}

private:
	RefCounter ref_cnt_;
	IO_Result result_;
};

}
}

#endif
