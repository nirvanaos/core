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
#ifndef NIRVANA_CORE_COREINTERFACE_H_
#define NIRVANA_CORE_COREINTERFACE_H_
#pragma once

#include "AtomicCounter.h"
#include <Nirvana/throw_exception.h>

namespace Nirvana {
namespace Core {

/// Core smart pointer.
/// \tparam T object or core interface class.
template <class T> using Ref = CORBA::servant_reference <T>;

#if defined (__GNUG__) || defined (__clang__)

#define DECLARE_CORE_INTERFACE protected:\
template <class> friend class CORBA::servant_reference;\
template <class> friend class CORBA::Internal::LifeCycleRefCnt;\
_Pragma ("GCC diagnostic push")\
_Pragma ("GCC diagnostic ignored \"-Winconsistent-missing-override\"")\
virtual void _add_ref () noexcept = 0;\
virtual void _remove_ref () noexcept = 0;\
_Pragma ("GCC diagnostic pop")

#else

#define DECLARE_CORE_INTERFACE protected:\
template <class> friend class CORBA::servant_reference;\
template <class> friend class CORBA::Internal::LifeCycleRefCnt;\
virtual void _add_ref () noexcept = 0;\
virtual void _remove_ref () noexcept = 0;

#endif

/// Dynamic implementation of a core object.
/// \tparam T object class.
template <class T>
class ImplDynamic final : public T
{
protected:
	template <class> friend class CORBA::servant_reference;
	friend class CORBA::Internal::LifeCycleRefCnt <ImplDynamic <T> >;

	template <class S, class ... Args> friend
	CORBA::Internal::I_ref <typename S::PrimaryInterface> CORBA::make_pseudo (Args&& ... args);

	template <class ... Args>
	ImplDynamic (Args&& ... args) :
		T (std::forward <Args> (args)...)
	{}

	~ImplDynamic ()
	{}

	void _add_ref () noexcept
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () noexcept
	{
		if (!ref_cnt_.decrement ())
			delete this;
	}

private:
	RefCounter ref_cnt_;
};

/// Dynamic implementation of a core object for usage in synchronized scenarios.
/// \tparam T object class.
template <class T>
class ImplDynamicSync : public T
{
public:
	unsigned _refcount_value () const noexcept
	{
		return ref_cnt_;
	}

protected:
	template <class> friend class CORBA::servant_reference;

	template <class S, class ... Args> friend
		CORBA::servant_reference <S> CORBA::make_reference (Args&& ... args);

	template <class ... Args>
	ImplDynamicSync (Args&& ... args) :
		T (std::forward <Args> (args)...),
		ref_cnt_ (1)
	{}

	void _add_ref () noexcept
	{
		++ref_cnt_;
	}

	void _remove_ref () noexcept
	{
		assert (ref_cnt_);
		if (!--ref_cnt_)
			delete this;
	}

private:
	unsigned ref_cnt_;
};

/// Static or stack implementation of a core object.
/// \tparam T object class.
template <class T>
class ImplStatic : public T
{
public:
	template <class ... Args>
	ImplStatic (Args&& ... args) :
		T (std::forward <Args> (args)...)
	{}

private:
	void _add_ref () noexcept
	{}

	void _remove_ref () noexcept
	{}
};

/// Special implementation of a core object.
/// \tparam T object class.
template <class T>
class ImplNoAddRef final : public T
{
protected:
	template <class> friend class CORBA::servant_reference;

	template <class ... Args>
	ImplNoAddRef (Args&& ... args) :
		T (std::forward <Args> (args)...)
	{}

	~ImplNoAddRef ()
	{}

	void _add_ref () noexcept
	{
		assert (false);
		throw_INTERNAL ();
	}

	void _remove_ref () noexcept
	{
		this->~ImplNoAddRef <T> ();
	}
};

}
}

#endif
