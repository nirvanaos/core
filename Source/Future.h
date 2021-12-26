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
#ifndef NIRVANA_CORE_FUTURE_H_
#define NIRVANA_CORE_FUTURE_H_

#include "Event.h"

namespace Nirvana {
namespace Core {

/// The result of asynchronous operation.
template <class T>
class Future :
	public Event
{
public:
	/// Waits until the future has a valid result and (depending on which template is used) retrieves it.
	/// It effectively calls wait() in order to wait for the result.
	/// 
	/// \returns The result of operation.
	const T& get () NIRVANA_NOEXCEPT
	{
		Event::wait ();
		return value_;
	}

	/// Set the result of asynchronous operation.
	/// 
	/// \param val The result of asynchronous operation.
	void set (const T& val) NIRVANA_NOEXCEPT
	{
		value_ = val;
		Event::set ();
	}

private:
	T value_;
};

}
}

#endif
