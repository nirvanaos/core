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
#include "pch.h"
#include "ConnectionPool.h"

ConnectionPool::Connection::Connection (ConnectionPool& pool) :
	pool_ (pool)
{
	if (pool.stack_.empty ())
		ptr_ = std::make_unique <::Connection> (pool.connstr_);
	else {
		ptr_ = std::move (pool.stack_.top ());
		pool.stack_.pop ();
	}
}

ConnectionPool::Connection::~Connection ()
{
	try {
		pool_.stack_.push (std::move (ptr_));
	} catch (...) {
	}
}
