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
#ifndef NDBC_CONNECTIONPOOLIMPL_H_
#define NDBC_CONNECTIONPOOLIMPL_H_
#pragma once

#include "PoolableConnection.h"

namespace NDBC {

class ConnectionPoolImpl :
	public CORBA::servant_traits <ConnectionPool>::Servant <ConnectionPoolImpl>
{
public:
	ConnectionPoolImpl (Driver::_ptr_type driver, IDL::String&& url, IDL::String&& user,
		IDL::String&& password, uint32_t max_size) :
		driver_ (std::move (driver)),
		url_ (std::move (url)),
		user_ (std::move (user)),
		password_ (std::move (password)),
		max_size_ (max_size),
		cur_size_ (0)
	{
		// Create connection to ensure that parameters are correct
		Connection::_ref_type conn = getConnection ();
		conn->close ();
	}

	~ConnectionPoolImpl ()
	{}

	Connection::_ref_type getConnection ()
	{
		CORBA::servant_reference <PoolableConnection> conn;
		if (!connections_.empty ()) {
			conn = CORBA::make_reference <PoolableConnection> (std::ref (*this),
				std::move (connections_.top ()));
			connections_.pop ();
		} else {
			conn = CORBA::make_reference <PoolableConnection> (std::ref (*this),
				ConnectionData (driver_->connect (url_, user_, password_)));
		}
		return conn->_this ();
	}

	uint32_t max_size () const noexcept
	{
		return max_size_;
	}

	void max_size (uint32_t limit) noexcept
	{
		max_size_ = limit;
		while (cur_size_ > max_size_) {
			--cur_size_;
			connections_.pop ();
		}
	}

	Pool <ConnectionData>& connections () noexcept
	{
		return connections_;
	}

	bool release_to_pool () noexcept
	{
		if (cur_size_ < max_size_) {
			++cur_size_;
			return true;
		}
		return false;
	}

	void release_failed () noexcept
	{
		--cur_size_;
	}

private:
	Driver::_ref_type driver_;
	IDL::String url_, user_, password_;
	Pool <ConnectionData> connections_;
	uint32_t max_size_, cur_size_;
};

inline void PoolableConnection::release_to_pool () noexcept
{
	if (parent_->release_to_pool () && !Base::release_to_pool ())
		parent_->release_failed ();
	parent_ = nullptr;
}

}

#endif
