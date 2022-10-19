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
#ifndef NIRVANA_ORB_CORE_TC_REF_H_
#define NIRVANA_ORB_CORE_TC_REF_H_
#pragma once

#include <CORBA/CORBA.h>

namespace CORBA {
namespace Core {

class TC_Ref;

/// Complex TypeCode uses mark-and-sweep garbage collection algorithm.
class TC_ComplexBase
{
public:
	virtual bool mark () NIRVANA_NOEXCEPT = 0
	{
		if (!marked_) {
			marked_ = true;
			return true;
		} else
			return false;
	}

	bool sweep () NIRVANA_NOEXCEPT
	{
		if (marked_) {
			marked_ = false;
			return false;
		} else {
			delete this;
			return true;
		}
	}

	virtual bool has_extern_ref () const NIRVANA_NOEXCEPT = 0;

	virtual bool set_recursive (const IDL::String& id, const TC_Ref& ref) NIRVANA_NOEXCEPT
	{
		return false;
	}

protected:
	TC_ComplexBase () :
		marked_ (false)
	{}

	virtual ~TC_ComplexBase ()
	{}

	void collect_garbage () NIRVANA_NOEXCEPT;

private:
	bool marked_;
};

template <class Base>
class TC_Complex :
	public Base,
	public TC_ComplexBase
{
protected:
	template <class ... Args>
	TC_Complex (Args ... args) :
		Base (std::forward <Args> (args)...)
	{}

	virtual void collect_garbage () NIRVANA_NOEXCEPT override
	{
		TC_ComplexBase::collect_garbage ();
	}

	virtual bool has_extern_ref () const NIRVANA_NOEXCEPT override
	{
		return Base::ref_cnt_.load () != 0;
	}
};

class TC_Ref : public TypeCode::_ptr_type
{
public:
	TC_Ref () :
		TypeCode::_ptr_type (nullptr),
		complex_ (nullptr)
	{}

	TC_Ref (const TC_Ref& src) NIRVANA_NOEXCEPT;

	TC_Ref (TC_Ref&& src) NIRVANA_NOEXCEPT :
		TypeCode::_ptr_type (src),
		complex_ (src.complex_)
	{
		src.p_ = nullptr;
		src.complex_ = nullptr;
	}

	TC_Ref (TypeCode::_ptr_type p, TC_ComplexBase* complex) NIRVANA_NOEXCEPT;
	TC_Ref (TypeCode::_ptr_type p) NIRVANA_NOEXCEPT;

	TC_Ref (TypeCode::_ref_type&& src) NIRVANA_NOEXCEPT :
		complex_ (nullptr)
	{
		reinterpret_cast <TypeCode::_ref_type&> (*this) = std::move (src);
	}

	~TC_Ref ()
	{
		if (!complex_)
			Internal::interface_release (p_);
	}

	TC_Ref& operator = (const TC_Ref& src) NIRVANA_NOEXCEPT;
	TC_Ref& operator = (TC_Ref&& src) NIRVANA_NOEXCEPT;
	TC_Ref& operator = (TypeCode::_ptr_type p) NIRVANA_NOEXCEPT;
	TC_Ref& operator = (TypeCode::_ref_type&& src) NIRVANA_NOEXCEPT;

	void mark () NIRVANA_NOEXCEPT
	{
		if (complex_)
			complex_->mark ();
	}

	void replace_recursive_placeholder (const IDL::String& id, const TC_Ref& ref) NIRVANA_NOEXCEPT
	{
		if (complex_ && complex_->set_recursive (id, ref))
			*this = ref;
	}

private:
	TC_ComplexBase* complex_;
};

}
}

#endif
