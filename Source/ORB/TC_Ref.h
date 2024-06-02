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
class NIRVANA_NOVTABLE TC_ComplexBase
{
public:
	virtual bool mark () noexcept
	{
		if (!marked_) {
			marked_ = true;
			return true;
		} else
			return false;
	}

	bool sweep () noexcept
	{
		if (marked_) {
			marked_ = false;
			return false;
		} else {
			delete this;
			return true;
		}
	}

	virtual bool has_extern_ref () const noexcept = 0;

	virtual bool set_recursive (const IDL::String& id, const TC_Ref& ref) noexcept
	{
		return false;
	}

	bool is_recursive () const noexcept
	{
		return recursive_;
	}

protected:
	TC_ComplexBase (bool recursive = false) :
		marked_ (false),
		recursive_ (recursive)
	{}

	virtual ~TC_ComplexBase ()
	{}

	void collect_garbage () noexcept;

private:
	bool marked_;
	bool recursive_;
};

template <class Base>
class NIRVANA_NOVTABLE TC_Complex :
	public Base,
	public TC_ComplexBase
{
protected:
	template <class ... Args>
	TC_Complex (Args&& ... args) :
		Base (std::forward <Args> (args)...)
	{}

	virtual void collect_garbage () noexcept override
	{
		TC_ComplexBase::collect_garbage ();
	}

	virtual bool has_extern_ref () const noexcept override
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

	TC_Ref (const TC_Ref& src) noexcept;

	TC_Ref (TC_Ref&& src) noexcept :
		TypeCode::_ptr_type (src),
		complex_ (src.complex_)
	{
		src.p_ = nullptr;
		src.complex_ = nullptr;
	}

	TC_Ref (TypeCode::_ptr_type p, TC_ComplexBase* complex) noexcept;
	TC_Ref (TypeCode::_ptr_type p) noexcept;

	TC_Ref (TypeCode::_ref_type&& src) noexcept :
		complex_ (nullptr)
	{
		reinterpret_cast <TypeCode::_ref_type&> (*this) = std::move (src);
	}

	~TC_Ref ()
	{
		if (!complex_)
			Internal::interface_release (p_);
	}

	TC_Ref& operator = (const TC_Ref& src) noexcept;
	TC_Ref& operator = (TC_Ref&& src) noexcept;
	TC_Ref& operator = (TypeCode::_ptr_type p) noexcept;
	TC_Ref& operator = (TypeCode::_ref_type&& src) noexcept;

	void mark () noexcept
	{
		if (complex_)
			complex_->mark ();
	}

	bool replace_recursive_placeholder (const IDL::String& id, const TC_Ref& ref) noexcept
	{
		if (complex_ && complex_->set_recursive (id, ref)) {
			*this = ref;
			return true;
		}
		return false;
	}

	bool is_recursive () const noexcept
	{
		return complex_ && complex_->is_recursive ();
	}

private:
	TC_ComplexBase* complex_;
};

}
}

#endif
