/*
* Database connection module.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2025 Igor Popov.
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
#include "PoolableStatement.h"

namespace NDBC {

PoolableStatementBase::PoolableStatementBase (Connection::_ref_type&& conn,
	StatementBase::_ptr_type base) noexcept :
	conn_ (std::move (conn)),
	base_ (base)
{}

void PoolableStatementBase::close () noexcept
{
	conn_ = nullptr;
	base_ = nullptr;
}

StatementBase::_ptr_type PoolableStatementBase::base () const
{
	PoolableBase::check (static_cast <bool> (base_));
	return base_;
}

void PoolableStatementBase::cleanup (StatementBase::_ptr_type s)
{
	while (s->getMoreResults ())
		;
	s->getUpdateCount ();
	s->clearWarnings ();
}

}
