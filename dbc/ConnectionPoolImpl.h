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
#include <Nirvana/System.h>

namespace NDBC {

class ConnectionPoolImpl :
	public CORBA::servant_traits <ConnectionPool>::Servant <ConnectionPoolImpl>
{
public:
	ConnectionPoolImpl (Driver::_ptr_type driver, IDL::String&& url, IDL::String&& user,
		IDL::String&& password, uint32_t max_size, uint32_t max_create, uint16_t options) :
		driver_ (std::move (driver)),
		url_ (std::move (url)),
		user_ (std::move (user)),
		password_ (std::move (password)),
		max_size_ (max_size),
		cur_size_ (0),
		max_create_ (max_create),
		cur_created_ (0),
		may_create_ (Nirvana::the_system->create_event (false, max_create > 1)),
		creation_timeout_ (std::numeric_limits <TimeBase::TimeT>::max ()),
		options_ (options)
	{
		if (max_create == 0 || max_create < max_size)
			throw CORBA::BAD_PARAM ();

		// Create connection to ensure that parameters are correct.
		// Do not activate it, return to pool immediately.
		connections_.push (ConnectionData (driver_->connect (url_, user_, password_)));
		cur_created_ = 1;
		cur_size_ = 1;
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
			--cur_size_;
		} else {

			for (;;) {
				if (cur_created_ >= max_create_ && !may_create_->wait (creation_timeout_))
					throw SQLException (SQLWarning (1, "Connection create timeout"), NDBC::SQLWarnings ());
				assert (cur_created_ < max_create_);

				conn = CORBA::make_reference <PoolableConnection> (std::ref (*this),
					ConnectionData (driver_->connect (url_, user_, password_)));
				if (max_create_ == ++cur_created_)
					may_create_->reset ();
				else if (cur_created_ > max_create_) {
					conn->close ();
					conn = nullptr;
					if (max_create_ == --cur_created_)
						may_create_->reset ();
				} else
					break;
			}
		}
		return conn->_this ();
	}

	uint32_t maxSize () const noexcept
	{
		return max_size_;
	}

	void maxSize (uint32_t limit) noexcept
	{
		max_size_ = limit;
		while (cur_size_ > max_size_) {
			--cur_size_;
			connections_.pop ();
		}
	}

	uint32_t maxCreate () const noexcept
	{
		return max_create_;
	}

	void maxCreate (uint32_t limit)
	{
		if (0 == limit)
			throw CORBA::BAD_PARAM ();
		if (limit > max_create_ && max_create_ <= cur_created_)
			may_create_->signal ();
		max_create_ = limit;
	}

	TimeBase::TimeT creationTimeout () const noexcept
	{
		return creation_timeout_;
	}

	void creationTimeout (TimeBase::TimeT t) noexcept
	{
		creation_timeout_ = t;
	}

	uint32_t connectionCount () const noexcept
	{
		return cur_created_;
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
		connection_destructed ();
	}

	void connection_destructed ()
	{
		if ((cur_created_--) == max_create_)
			may_create_->signal ();
	}

	unsigned options () const noexcept
	{
		return options_;
	}

private:
	Driver::_ref_type driver_;
	IDL::String url_, user_, password_;
	Pool <ConnectionData> connections_;
	uint32_t max_size_, cur_size_, max_create_, cur_created_;
	Nirvana::Event::_ref_type may_create_;
	TimeBase::TimeT creation_timeout_;
	const unsigned options_;
};

inline void PoolableConnection::cleanup (ConnectionData& data)
{
	if (parent_->options () & Manager::DO_NOT_SHARE_PREPARED)
		data.prepared_statements.clear ();

	for (auto it = savepoints_.cbegin (); it != savepoints_.cend (); ++it) {
		try {
			data->releaseSavepoint (*it);
		} catch (...) {
			assert (false);
		}
	}
	savepoints_.clear ();

	if (!data->getAutoCommit ()) {
		data->rollback (nullptr);
		data->setAutoCommit (true);
	}

	data.reset ();
}

inline void PoolableConnection::release_to_pool () noexcept
{
	if (parent_->release_to_pool ()) {
		if (!Base::release_to_pool ())
			parent_->release_failed ();
	} else {
		{
			ConnectionData data (std::move (Base::data_));
		}
		parent_->connection_destructed ();
	}
	parent_ = nullptr;
}

}

#endif
