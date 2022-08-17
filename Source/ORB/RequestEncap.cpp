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

#include "RequestEncap.h"

namespace CORBA {
namespace Core {

RequestEncap::RequestEncap (const Request& parent) :
	Request (parent)
{}

void RequestEncap::set_exception (Any& e)
{
	assert (false);
}

void RequestEncap::success ()
{
	assert (false);
}

void RequestEncap::invoke ()
{
	assert (false);
}

bool RequestEncap::is_exception () const NIRVANA_NOEXCEPT
{
	assert (false);
	return false;
}

bool RequestEncap::completed () const NIRVANA_NOEXCEPT
{
	assert (false);
	return false;
}

bool RequestEncap::wait (uint64_t timeout)
{
	assert (false);
	return false;
}

void RequestEncap::cancel ()
{
	assert (false);
}

}
}
