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
#ifndef PACMAN_CONNECTIONPOOL_H_
#define PACMAN_CONNECTIONPOOL_H_
#pragma once

#include "Connection.h"
#include <stack>
#include <memory>

class ConnectionPool
{
	typedef std::unique_ptr <::Connection> ConnectionPtr;

public:
	ConnectionPool (const char* connstr) noexcept :
		connstr_ (connstr)
	{}

	class Connection
	{
	public:
		Connection (ConnectionPool& pool);
		~Connection ();

		Connection (const Connection&) = delete;
		Connection (Connection&&) = delete;

		Connection& operator = (const Connection&) = delete;
		Connection& operator = (Connection&&) = delete;

		::Connection* operator -> () const noexcept
		{
			return ptr_.get ();
		}

	private:
		ConnectionPool& pool_;
		ConnectionPtr ptr_;
	};

private:
	const char* connstr_;
	std::stack <ConnectionPtr> stack_;
};

#endif
