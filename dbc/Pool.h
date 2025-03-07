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
#ifndef NDBC_POOL_H_
#define NDBC_POOL_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <CORBA/Server.h>
#include <Nirvana/NDBC_s.h>
#include <stack>

namespace NDBC {

template <class Data>
class Pool : public std::stack <Data>
{
	using Base = std::stack <Data>;

public:
	Pool () = default;
	~Pool ();

	Pool (const Pool&) = delete;
	Pool (Pool&&) = default;

	Pool& operator = (const Pool&) = delete;
	Pool& operator = (Pool&&) = delete;
};

template <class Data>
Pool <Data>::~Pool ()
{
	while (!Base::empty ()) {
		Data data (std::move (Base::top ()));
		Base::pop ();
		try {
			data->close ();
		} catch (...) {
			assert (false);
			// TODO: Log
		}
	}
}

class PoolableBase
{
public:
	bool isClosed () const noexcept
	{
		return closed_;
	}

	static void throw_closed ();

protected:
	PoolableBase () :
		closed_ (false)
	{}

	void check () const
	{
		if (closed_)
			throw_closed ();
	}

	bool close ()
	{
		if (!closed_) {
			closed_ = true;
			return true;
		}
		return false;
	}

	static void deactivate (PortableServer::Servant servant) noexcept;

private:
	bool closed_;
};

template <class Data>
class Poolable : public PoolableBase
{
public:
	using DataType = Data;
	using PoolType = Pool <Data>;

protected:
	Poolable (PoolType& pool, DataType&& data) noexcept :
		pool_ (pool),
		data_ (std::move (data))
	{}

	DataType& data ()
	{
		check ();
		return data_;
	}

	static void cleanup (DataType& data)
	{}

protected:
	PoolType& pool_;
	DataType data_;
};

template <class Data, class I, class S>
class PoolableS : 
	public CORBA::servant_traits <I>::template Servant <S>,
	public Poolable <Data>
{
	using Base = Poolable <Data>;

public:
	bool close ()
	{
		if (Base::close ()) {
			release_to_pool ();
			Base::deactivate (this);
			return true;
		}
		return false;
	}

protected:
	PoolableS (Pool <Data>& pool, Data&& data) noexcept :
		Base (pool, std::move (data))
	{}

	void destruct () noexcept
	{
		if (!Base::isClosed ())
			static_cast <S&> (*this).release_to_pool ();
	}

	bool release_to_pool () noexcept
	{
		Data data (std::move (Base::data_));
		try {
			static_cast <S&> (*this).cleanup (data);
			Base::pool_.push (std::move (data));
			return true;
		} catch (...) {
			assert (false);
		}
		return false;
	}

};

}

#endif
