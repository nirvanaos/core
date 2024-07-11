/*
* Nirvana package manager.
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
#ifndef NIRVANA_PACMAN_STATEMENT_H_
#define NIRVANA_PACMAN_STATEMENT_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Nirvana/NDBC.h>
#include <stack>

typedef std::stack <NDBC::PreparedStatement::_ref_type> StatementPool;

class Statement
{
public:
	Statement (NDBC::PreparedStatement::_ref_type&& stmt, StatementPool& pool) noexcept :
		statement_ (std::move (stmt)),
		pool_ (pool)
	{}

	~Statement ()
	{
		return_to_pool ();
	}

	Statement (Statement&&) = default;
	Statement (const Statement&) = delete;

	Statement& operator = (Statement&& src)
	{
		NIRVANA_ASSERT (&pool_ == &src.pool_);
		return_to_pool ();
		statement_ = std::move (src.statement_);
		return *this;
	}

	Statement& operator = (const Statement&) = delete;

	NDBC::PreparedStatement::_ptr_type operator -> () const noexcept
	{
		NIRVANA_ASSERT (statement_);
		return statement_;
	}

private:
	void return_to_pool () noexcept;

private:
	NDBC::PreparedStatement::_ref_type statement_;
	StatementPool& pool_;
};

#endif
