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
#include "ORB.h"
#include <CORBA/TypeCodeImpl.h>

namespace CORBA {
namespace Core {

TypeCode::_ref_type ORB::get_compact_typecode (TypeCode::_ptr_type tc, const TC_Recurse* parent)
{
	TCKind kind = tc->kind ();
	switch (kind) {
		case TCKind::tk_struct:
		case TCKind::tk_union:
		case TCKind::tk_value: {
			for (const TC_Recurse* p = parent; p; p = p->prev) {
				if (&p->tc == &tc)
					return create_recursive_tc (tc->id ());
			}
			ULong member_count = tc->member_count ();
			TC_Recurse rec { parent, tc };
			switch (kind) {
				case TCKind::tk_struct: {
					StructMemberSeq members (member_count);
					for (ULong i = 0; i < member_count; ++i) {
						members [i].type (get_compact_typecode (tc->member_type (i), &rec));
					}
					return create_struct_tc (tc->id (), IDL::String (), members);
				}

				case TCKind::tk_union: {
					UnionMemberSeq members (member_count);
					for (ULong i = 0; i < member_count; ++i) {
						UnionMember& m = members [i];
						m.label (tc->member_label (i));
						m.type (get_compact_typecode (tc->member_type (i), &rec));
					}
					return create_union_tc (tc->id (), IDL::String (), tc->discriminator_type (), members);
				}

				case TCKind::tk_value: {
					ValueMemberSeq members (member_count);
					for (ULong i = 0; i < member_count; ++i) {
						ValueMember& m = members [i];
						m.type (get_compact_typecode (tc->member_type (i), &rec));
						m.access (tc->member_visibility (i));
					}
					return create_value_tc (tc->id (), IDL::String (), tc->type_modifier (), tc->concrete_base_type (), members);
				}
			}
		}

		case TCKind::tk_sequence:
			return create_sequence_tc (tc->length (), get_compact_typecode (tc->content_type (), parent));

		case TCKind::tk_array:
			return create_array_tc (tc->length (), get_compact_typecode (tc->content_type (), parent));

		case TCKind::tk_except: {
			// It is not necessary to check the recursion for the tk_except.
			// But I believe that the get_compact_typecode for tk_except is rarely used.
			// So I prefer to produce slow but compact code for it.
			ULong member_count = tc->member_count ();
			StructMemberSeq members;
			members.reserve (member_count);
			for (ULong i = 0; i < member_count; ++i) {
				members.emplace_back (IDL::String (), get_compact_typecode (tc->member_type (i), parent), nullptr);
			}
			return create_exception_tc (tc->id (), IDL::String (), members);
		}

		default:
			return tc->get_compact_typecode ();
	}
}

bool ORB::tc_equal (TypeCode::_ptr_type left, TypeCode::_ptr_type right, const TC_Pair* parent)
{
	// Disable optimization in Debug configuration for testing purposes.
#ifndef _DEBUG
	if (&left == &right)
		return true;
#endif

	for (const TC_Pair* p = parent; p; p = p->prev) {
		if (
			(&left == &p->left && &right == &p->right)
			||
			(&right == &p->left && &left == &p->right)
			)
			return true;
	}

	TCKind kind = left->kind ();
	if (right->kind () != kind)
		return false;

	ULong member_count = 0;
	switch (kind) {
		case TCKind::tk_struct:
		case TCKind::tk_union:
		case TCKind::tk_value:
		case TCKind::tk_except:
			member_count = left->member_count ();
			if (member_count != right->member_count ()
				|| left->id () != right->id ()
				|| left->name () != right->name ())
				return false;
			for (ULong i = 0; i < member_count; ++i) {
				if (left->member_name (i) != right->member_name (i))
					return false;
			}
			break;

		case TCKind::tk_sequence:
		case TCKind::tk_array:
			if (left->length () != right->length ())
				return false;
			break;

		case TCKind::tk_alias:
			break;

		default:
			return left->equal (right);
	}

	switch (kind) {
		case TCKind::tk_except:
			for (ULong i = 0; i < member_count; ++i) {
				if (!left->member_type (i)->equal (right->member_type (i)))
					return false;
			}
			return true;

		case TCKind::tk_union:
			if (!left->discriminator_type ()->equal (right->discriminator_type ()))
				return false;
			if (left->default_index () != right->default_index ())
				return false;
			for (ULong i = 0; i < member_count; ++i) {
				if (get_union_label (left, i) != get_union_label (right, i))
					return false;
			}
			break;

		case TCKind::tk_value:
			if (left->type_modifier () != right->type_modifier ())
				return false;
			{
				TypeCode::_ref_type base_left = left->concrete_base_type ();
				TypeCode::_ref_type base_right = right->concrete_base_type ();
				if (base_left || base_right) {
					if (base_left && base_right) {
						if (!base_left->equal (base_right))
							return false;
					} else
						return false;
				}
			}
			for (ULong i = 0; i < member_count; ++i) {
				if (left->member_visibility (i) != right->member_visibility (i))
					return false;
			}
			break;
	}

	TC_Pair rec { parent, left, right };

	switch (kind) {
		case TCKind::tk_struct:
		case TCKind::tk_union:
		case TCKind::tk_value:
			for (ULong i = 0; i < member_count; ++i) {
				if (!tc_equal (left->member_type (i), right->member_type (i), &rec))
					return false;
			}
			break;

		default:
			if (!tc_equal (left->content_type (), right->content_type (), &rec))
				return false;
	}

	return true;
}

bool ORB::tc_equivalent (TypeCode::_ptr_type left, TypeCode::_ptr_type right, const TC_Pair* parent)
{
	left = Internal::TypeCodeBase::dereference_alias (left);
	right = Internal::TypeCodeBase::dereference_alias (right);

	// Disable optimization in Debug configuration for testing purposes.
#ifndef _DEBUG
	if (&left == &right)
		return true;
#endif

	for (const TC_Pair* p = parent; p; p = p->prev) {
		if (
			(&left == &p->left && &right == &p->right)
			||
			(&right == &p->left && &left == &p->right)
			)
			return true;
	}

	TCKind kind = left->kind ();
	if (right->kind () != kind)
		return false;

	ULong member_count = 0;
	switch (kind) {
		case TCKind::tk_struct:
		case TCKind::tk_union:
		case TCKind::tk_value:
		case TCKind::tk_except:
			member_count = left->member_count ();
			if (member_count != right->member_count ())
				return false;
			{
				// If the id operation is valid for the TypeCode kind, equivalent returns TRUE if the results of id for both
				// TypeCodes are non - empty strings and both strings are equal. If both ids are non - empty but are not equal, then
				// equivalent returns FALSE. If either or both id is an empty string, or the TypeCode kind does not support the id
				// operation, equivalent will perform a structural comparison of the TypeCodes by comparing the results of the other
				// TypeCode operations.
				IDL::String id_left = left->id ();
				IDL::String id_right = right->id ();
				if (!id_left.empty () && !id_right.empty ())
					return id_left == id_right;
			}
			break;

		case TCKind::tk_sequence:
		case TCKind::tk_array:
			if (left->length () != right->length ())
				return false;
			break;

		default:
			return left->equivalent (right);
	}

	switch (kind) {
		case TCKind::tk_except:
			for (ULong i = 0; i < member_count; ++i) {
				if (!left->member_type (i)->equal (right->member_type (i)))
					return false;
			}
			return true;

		case TCKind::tk_union:
			if (!left->discriminator_type ()->equivalent (right->discriminator_type ()))
				return false;
			if (left->default_index () != right->default_index ())
				return false;
			for (ULong i = 0; i < member_count; ++i) {
				if (get_union_label (left, i) != get_union_label (right, i))
					return false;
			}
			break;

		case TCKind::tk_value:
			if (left->type_modifier () != right->type_modifier ())
				return false;
			{
				TypeCode::_ref_type base_left = left->concrete_base_type ();
				TypeCode::_ref_type base_right = right->concrete_base_type ();
				if (base_left || base_right) {
					if (base_left && base_right) {
						if (!base_left->equivalent (base_right))
							return false;
					} else
						return false;
				}
			}
			for (ULong i = 0; i < member_count; ++i) {
				if (left->member_visibility (i) != right->member_visibility (i))
					return false;
			}
			break;
	}

	TC_Pair rec { parent, left, right };

	switch (kind) {
		case TCKind::tk_struct:
		case TCKind::tk_union:
		case TCKind::tk_value:
			for (ULong i = 0; i < member_count; ++i) {
				if (!tc_equivalent (left->member_type (i), right->member_type (i), &rec))
					return false;
			}
			break;

		default:
			if (!tc_equivalent (left->content_type (), right->content_type (), &rec))
				return false;
	}

	return true;
}

ULongLong ORB::get_union_label (TypeCode::_ptr_type tc, ULong i)
{
	Any a = tc->member_label (i);
	ULongLong u = 0;
	a.type ()->n_copy (&u, a.data ());
	return u;
}

unsigned ORB::from_hex (int x)
{
	if ('0' <= x && x <= '9')
		return x - '0';
	else if ('a' <= x && x <= 'f')
		return x - 'a' + 10;
	else if ('A' <= x && x <= 'F')
		return x - 'A' + 10;
	else
		throw BAD_PARAM (MAKE_OMG_MINOR (9));
}

bool ORB::schema_eq (const Char* schema, const Char* begin, const Char* end) noexcept
{
	do {
		Char c = *begin;
		if ('A' <= c && c <= 'Z')
			c -= 'A' + 'a';
		if (*schema != c)
			return false;
		++schema;
		++begin;
	} while (*schema && begin != end);
	return !*schema && begin == end;
}

}
}
