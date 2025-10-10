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
#include "TC_Base.h"
#include "Services.h"
#include "ServantProxyLocal.h"

using Nirvana::Core::SyncContext;

namespace CORBA {
namespace Core {

void TC_Base::_add_ref () noexcept
{
	ref_cnt_.increment ();
}

void TC_Base::_remove_ref () noexcept
{
	if (0 == ref_cnt_.decrement ())
		collect_garbage ();
}

void TC_Base::collect_garbage () noexcept
{
	try {
		SyncContext& sc = local2proxy (Services::bind (Services::TC_Factory))->sync_context ();
		if (&SyncContext::current () == &sc)
			delete this;
		else
			GarbageCollector::schedule (*this, sc);
	} catch (...) {}
}

TypeCode::_ref_type TC_Base::dereference_alias (TypeCode::_ptr_type tc)
{
	TypeCode::_ref_type ret = tc;
	for (;;) {

		if (!ret)
			throw BAD_TYPECODE (MAKE_OMG_MINOR (2));

		switch (ret->kind ()) {
		case TCKind::tk_null:
		case TCKind::tk_void:
		case TCKind::tk_except:
			throw BAD_TYPECODE (MAKE_OMG_MINOR (2));
			break;

		case TCKind::tk_alias:
			ret = ret->content_type ();
			continue;
		}

		break;
	}

	return ret;
}

bool TC_Base::is_var_len (TypeCode::_ptr_type tc, SizeAndAlign& sa)
{
	TypeCode::_ref_type t = dereference_alias (tc);
	switch (t->kind ()) {

	case TCKind::tk_boolean:
	case TCKind::tk_octet:
	case TCKind::tk_char:
		sa.append (1, 1);
		break;

	case TCKind::tk_short:
	case TCKind::tk_ushort:
		sa.append (2, 2);
		break;

	case TCKind::tk_long:
	case TCKind::tk_ulong:
	case TCKind::tk_float:
		sa.append (4, 4);
		break;

	case TCKind::tk_longlong:
	case TCKind::tk_ulonglong:
	case TCKind::tk_double:
		sa.append (8, 8);
		break;

	case TCKind::tk_longdouble:
		sa.append (8, 16);
		break;

	case TCKind::tk_fixed:
		sa.append (1, (t->fixed_digits () + 2) / 2);
		break;

	case TCKind::tk_array: {
		TypeCode::_ref_type content_type = t->content_type ();
		ArrayTraits traits (t->length ());
		get_array_traits (content_type, traits);
		auto size_before = sa.size;
		if (is_var_len (content_type, sa)) {
			sa.invalidate ();
			return true;
		}
		auto size_last = sa.size - size_before;
		sa.size = size_before + (traits.element_count - 1) * content_type->n_size () + size_last;
	} break;

	case TCKind::tk_struct:
	case TCKind::tk_union:
	{
		for (ULong cnt = t->member_count (), i = 0; i < cnt; ++i) {
			if (is_var_len (t->member_type (i), sa))
				return true;
		}
	} break;

	case TCKind::tk_wchar:
		sa.invalidate ();
		break;

	default:
		sa.invalidate ();
		return true;
	}

	return false;
}

bool TC_Base::SizeAndAlign::append (unsigned member_align, unsigned member_size) noexcept
{
	if (!is_valid ())
		return false;

	assert (1 <= member_align && member_align <= 8);
	assert (member_size);

	if (!size) {
		if (alignment < member_align)
			alignment = member_align;
		size = member_size;
		return true;
	}

	if (alignment < member_align) {
		// Gap may be occur here ocassionally, depending on the real alignment.
		// We must break here.
		invalidate ();
		return false;
	}

	size = (size + member_align - 1) / member_align * member_align + member_size;
	return true;
}

void TC_Base::get_array_traits (TypeCode::_ptr_type content_tc, ArrayTraits& traits)
{
	TypeCode::_ref_type t = dereference_alias (content_tc);
	TCKind kind = t->kind ();
	if (TCKind::tk_array == kind) {
		traits.element_count *= t->length ();
		return get_array_traits (t->content_type (), traits);
	} else
		traits.element_type = std::move (t);
}

}
}
