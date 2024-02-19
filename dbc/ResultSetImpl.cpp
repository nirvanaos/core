/*
* Database connection module.
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
#include "pch.h"
#include "ResultSetImpl.h"
#include <Nirvana/string_conv.h>

namespace NDBC {

NIRVANA_NORETURN void ResultSetImpl::throw_exception (IDL::String msg)
{
	throw NDBC::SQLException (NDBC::SQLWarning (0, std::move (msg)), NDBC::SQLWarnings ());
}

NIRVANA_NORETURN void ResultSetImpl::data_conversion_error ()
{
	throw_exception ("Data conversion error");
}

ResultSetImpl::~ResultSetImpl ()
{}

std::wstring ResultSetImpl::u2w (const IDL::String& utf8)
{
	std::wstring ret;
	Nirvana::append_wide (utf8, ret);
	return ret;
}

const Variant& ResultSetImpl::field (Ordinal ord) const
{
	if (ord == 0 || ord > column_count ())
		throw_exception ("Invalid column index");
	return cache () [cache_position ()][ord - 1];
}

}

NIRVANA_VALUETYPE_IMPL (_exp_NDBC_ResultSet, NDBC::ResultSet, NDBC::ResultSetImpl)

