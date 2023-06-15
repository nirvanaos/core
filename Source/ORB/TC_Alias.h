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
#ifndef NIRVANA_ORB_CORE_TC_ALIAS_H_
#define NIRVANA_ORB_CORE_TC_ALIAS_H_
#pragma once

#include "TC_IdName.h"
#include "TC_Ref.h"
#include "TC_Impl.h"

namespace CORBA {
namespace Core {

class TC_Alias : public TC_Impl <TC_Alias, TC_Complex <TC_IdName> >
{
	typedef TC_Impl <TC_Alias, TC_Complex <TC_IdName> > Impl;

public:
	using Servant::_s_id;
	using Servant::_s_name;
	using Servant::_s_content_type;
	using Servant::_s_n_CDR_size;
	using Servant::_s_n_aligned_size;
	using Servant::_s_n_byteswap;

	TC_Alias (IDL::String&& id, IDL::String&& name) noexcept :
		Impl (TCKind::tk_alias, std::move (id), std::move (name))
	{}

	TC_Alias (IDL::String&& id, IDL::String&& name, TC_Ref&& content_type) noexcept :
		Impl (TCKind::tk_alias, std::move (id), std::move (name)),
		content_type_ (std::move (content_type))
	{}

	void set_content_type (TC_Ref&& content_type) noexcept
	{
		content_type_ = std::move (content_type);
	}

	Boolean equal (TypeCode::_ptr_type other) const
	{
		return TypeCodeBase::equal (TCKind::tk_alias, id_, name_, content_type_);
	}

	Boolean equivalent (TypeCode::_ptr_type other) const
	{
		return content_type_->equivalent (other);
	}

	TypeCode::_ref_type content_type () const noexcept
	{
		return content_type_;
	}

	size_t n_aligned_size () const
	{
		return content_type_->n_aligned_size ();
	}

	size_t n_CDR_size () const
	{
		return content_type_->n_CDR_size ();
	}

	size_t n_align () const
	{
		return content_type_->n_align ();
	}

	bool n_is_CDR () const
	{
		return content_type_->n_is_CDR ();
	}

	void n_construct (void* p) const
	{
		content_type_->n_construct (p);
	}

	void n_destruct (void* p) const
	{
		content_type_->n_destruct (p);
	}

	void n_copy (void* dst, const void* src) const
	{
		content_type_->n_copy (dst, src);
	}

	void n_move (void* dst, void* src) const
	{
		content_type_->n_move (dst, src);
	}

	void n_marshal_in (const void* src, size_t count, Internal::IORequest_ptr rq) const
	{
		content_type_->n_marshal_in (src, count, rq);
	}

	void n_marshal_out (void* src, size_t count, Internal::IORequest_ptr rq) const
	{
		content_type_->n_marshal_out (src, count, rq);
	}

	void n_unmarshal (Internal::IORequest_ptr rq, size_t count, void* dst) const
	{
		content_type_->n_unmarshal (rq, count, dst);
	}

	void n_byteswap (void* p, size_t count) const
	{
		content_type_->n_byteswap (p, count);
	}

protected:
	virtual bool mark () noexcept override;
	virtual bool set_recursive (const IDL::String& id, const TC_Ref& ref) noexcept override;

private:
	TC_Ref content_type_;
};

}
}

#endif
