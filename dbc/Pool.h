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
using Pool = std::stack <Data>;

class PoolableBase
{
public:
	static void check (bool valid);
};

template <class Data>
class Poolable
{
public:
	using DataType = Data;
	using PoolType = Pool <Data>;

	bool isClosed () const
	{
		return !static_cast <bool> (data_);
	}

protected:
	Poolable (PoolType& pool, DataType&& data) noexcept :
		pool_ (pool),
		data_ (std::move (data))
	{}

	void check () const
	{
		PoolableBase::check (static_cast <bool> (data_));
	}

	DataType& data ();

	static void cleanup (DataType& data)
	{}

protected:
	PoolType& pool_;
	DataType data_;
};

template <class Data>
Data& Poolable <Data>::data ()
{
	check ();
	return data_;
}

template <class Data, class S>
class PoolableS : public Poolable <Data>
{
	using Base = Poolable <Data>;

public:
	void close ()
	{
		Base::check ();
		release_to_pool ();
	}

protected:
	PoolableS (Pool <Data>& pool, Data&& data) noexcept :
		Base (pool, std::move (data))
	{}

	~PoolableS ()
	{
		if (static_cast <bool> (Base::data_))
			release_to_pool ();
	}

private:
	void release_to_pool () noexcept;
};

template <class Data, class S>
void PoolableS <Data, S>::release_to_pool () noexcept
{
	assert (static_cast <bool> (Base::data_));
	Data data = std::move (Base::data_);
	assert (!static_cast <bool> (Base::data_));
	try {
		S::cleanup (data);
		Base::pool_.push (std::move (data));
	} catch (...) {}
}

}

#endif
