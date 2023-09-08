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
#include "../pch.h"
#include "RequestEncap.h"

namespace CORBA {
namespace Core {

void RequestEncap::invoke ()
{
	throw BAD_OPERATION ();
}

void RequestEncap::cancel () noexcept
{}

void RequestEncap::success ()
{
	throw BAD_INV_ORDER ();
}

void RequestEncap::set_exception (Any& e)
{
	throw BAD_INV_ORDER ();
}

bool RequestEncap::get_exception (Any& e)
{
	return false;
}

bool RequestEncap::marshal_op ()
{
	return stream_out_;
}

}
}
