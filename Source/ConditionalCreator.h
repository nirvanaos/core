/// \file
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
#ifndef NIRVANA_CORE_CONDITIONALCREATOR_H_
#define NIRVANA_CORE_CONDITIONALCREATOR_H_
#pragma once

#include "ObjectPool.h"

namespace Nirvana {
namespace Core {

template <class T>
class CreatorWithPool
{
public:
	using ObjRef = typename ObjectCreator <T>::ObjRef;

	static void initialize () noexcept
	{
		pool_.construct ();
	}

	static void terminate () noexcept
	{
		pool_.destruct ();
	}

	static ObjRef create ()
	{
		return pool_->create ();
	}

	static void release (T& obj) noexcept
	{
		pool_->release (obj);
	}

private:
	static StaticallyAllocated <ObjectPool <T> > pool_;
};

template <class T>
StaticallyAllocated <ObjectPool <T> > CreatorWithPool <T>::pool_;

template <class T>
class CreatorNoPool : public ObjectCreator <T>
{
public:
	using ObjRef = typename ObjectCreator <T>::ObjRef;

	static void initialize () noexcept
	{}

	static void terminate () noexcept
	{}

	static ObjRef create ()
	{
		ObjRef obj;
		ObjectCreator <T>::create (obj);
		return obj;
	}

};

template <class T, bool use_pool>
using ConditionalCreator = typename std::conditional <use_pool,
	CreatorWithPool <T>, CreatorNoPool <T> >::type;

class Empty
{};

template <bool use_pool>
using ConditionalObjectBase = typename std::conditional <use_pool, StackElem, Empty>::type;

}
}

#endif
