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
#include "../pch.h"
#include "ORB.h"
#include "LocalAddress.h"

namespace CORBA {

TypeCode::_ref_type Static_the_orb::get_compact_typecode (TypeCode::_ptr_type tc, const TC_Recurse* parent)
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

				default: {
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

		case TCKind::tk_abstract_interface:
			return create_abstract_interface_tc (tc->id (), IDL::String ());

		case TCKind::tk_local_interface:
			return create_local_interface_tc (tc->id (), IDL::String ());

		case TCKind::tk_objref:
			return create_interface_tc (tc->id (), IDL::String ());

		default:
			return tc->get_compact_typecode ();
	}
}

bool Static_the_orb::tc_equal (TypeCode::_ptr_type left, TypeCode::_ptr_type right, const TC_Pair* parent)
{
	// Disable optimization in Debug configuration for testing purposes.
#ifdef NDEBUG
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

		case TCKind::tk_objref:
		case TCKind::tk_local_interface:
		case TCKind::tk_abstract_interface:
			return left->id () == right->id () && left->name () == right->name ();

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

bool Static_the_orb::tc_equivalent (TypeCode::_ptr_type left, TypeCode::_ptr_type right, const TC_Pair* parent)
{
	left = Internal::TypeCodeBase::dereference_alias (left);
	right = Internal::TypeCodeBase::dereference_alias (right);

	// Disable optimization in Debug configuration for testing purposes.
#ifdef NDEBUG
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

		case TCKind::tk_objref:
		case TCKind::tk_local_interface:
		case TCKind::tk_abstract_interface:
			return left->id () == right->id ();

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

ULongLong Static_the_orb::get_union_label (TypeCode::_ptr_type tc, ULong i)
{
	Any a = tc->member_label (i);
	ULongLong u = 0;
	a.type ()->n_copy (&u, a.data ());
	return u;
}

unsigned Static_the_orb::from_hex (int x)
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

bool Static_the_orb::schema_eq (const Char* schema, const Char* begin, const Char* end) noexcept
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

OctetSeq Static_the_orb::unescape_object_key (const Char* begin, const Char* end)
{
	OctetSeq key;
	key.reserve (end - begin);
	while (begin != end) {
		if ('%' == *begin) {
			++begin;
			if (end - begin < 2)
				throw BAD_PARAM (MAKE_OMG_MINOR (9));
			key.push_back ((from_hex (begin [0]) << 4) | from_hex (begin [1]));
			begin += 2;
		} else
			key.push_back (*(begin++));
	}
	return key;
}

Object::_ref_type Static_the_orb::string_to_object (Internal::String_in str, Internal::String_in iid)
{
	const Char* str_begin = str.data ();
	const Char* str_end = str_begin + str.size ();
	const Char* schema_end = std::find (str_begin, str_end, ':');
	if (*schema_end != ':')
		throw BAD_PARAM (MAKE_OMG_MINOR (7));

	Object::_ref_type obj;
	if (schema_eq ("ior", str_begin, schema_end)) {

		size_t digits = str_end - schema_end - 1;
		if (digits % 2)
			throw BAD_PARAM (MAKE_OMG_MINOR (9));
		OctetSeq octets;
		octets.reserve (digits / 2);
		for (const Char* pd = schema_end + 1; pd != str_end; pd += 2) {
			octets.push_back ((from_hex (pd [0]) << 4) | from_hex (pd [1]));
		}
		Core::ReferenceRemoteRef unconfirmed_ref;
		Nirvana::Core::ImplStatic <Core::StreamInEncap> stm (std::ref (octets));
		obj = unmarshal_object (stm, unconfirmed_ref);

	} else if (schema_eq ("corbaloc", str_begin, schema_end)) {

		const Char* addr_begin = schema_end + 1;
		const Char* addr_end = std::find (addr_begin, str_end, '/');
		if (*addr_end != '/')
			throw BAD_PARAM (MAKE_OMG_MINOR (9));

		while (addr_begin < addr_end) {
			const Char* prot_end = std::find (addr_begin, addr_end, ',');
			if (addr_begin == prot_end)
				throw BAD_PARAM (MAKE_OMG_MINOR (8)); // Empty protocol
			const Char* schema_end = std::find (addr_begin, prot_end, ':');
			if (*schema_end != ':')
				throw BAD_PARAM (MAKE_OMG_MINOR (8));
			if (schema_end == addr_begin || schema_eq ("iiop", addr_begin, schema_end)) {
				// iiop protocol

				Nirvana::Core::ImplStatic <Core::StreamOutEncap> octets (true);
				{
					const Char* ver_begin = schema_end + 1;
					const Char* ver_end = std::find (ver_begin, prot_end, '@');
					const Char* host_begin;

					IIOP::Version ver (1, 0);
					if (ver_end == prot_end)
						host_begin = ver_begin;
					else {
						size_t len;
						unsigned major = std::stoi (std::string (ver_begin, ver_end), &len);
						ver_begin += len;
						if (*ver_begin != '.')
							throw BAD_PARAM (MAKE_OMG_MINOR (9));
						++ver_begin;
						unsigned minor = std::stoi (std::string (ver_begin, ver_end), &len);
						if (ver_begin + len < ver_end)
							throw BAD_PARAM (MAKE_OMG_MINOR (9));
						if (major != 1 || minor > 255)
							throw BAD_PARAM (MAKE_OMG_MINOR (9));
						host_begin = ver_end + 1;
						ver.minor ((Octet)minor);
					}
					const Char* host_end = std::find (host_begin, prot_end, ':');

					UShort port = IIOP_DEFAULT_PORT;
					if (host_end != prot_end) {
						const Char* port_begin = host_end;
						size_t len;
						unsigned u = std::stoi (std::string (port_begin, prot_end), &len);
						if (port_begin + len != prot_end || u > std::numeric_limits <UShort>::max ())
							throw BAD_PARAM (MAKE_OMG_MINOR (9));
						port = (UShort)u;
					}

					Nirvana::Core::ImplStatic <Core::StreamOutEncap> encap;
					encap.write_one (ver);
					size_t host_len = host_end - host_begin;
					if (host_len) {
						encap.write_size (host_len);
						encap.write_c (1, host_len, host_begin);
					} else {
						encap.write_string_c (Core::LocalAddress::singleton ().host ());
					}
					encap.write_c (2, 2, &port);

					OctetSeq object_key;
					if (addr_end < str_end)
						object_key = unescape_object_key (addr_end + 1, str_end);

					encap.write_seq (object_key);

					if (ver.minor () > 0) {
						IOP::TaggedComponentSeq components;
						if (!iid.empty ()) {
							Nirvana::Core::ImplStatic <Core::StreamOutEncap> encap;
							uint32_t ORB_type = ESIOP::ORB_TYPE;
							encap.write_c (4, 4, &ORB_type);
							components.emplace_back (IOP::TAG_ORB_TYPE, std::move (encap.data ()));
						}
						encap.write_tagged (components); // empty IOP::TaggedComponentSeq
					}

					IOP::TaggedProfileSeq addr {
						IOP::TaggedProfile (IOP::TAG_INTERNET_IOP, std::move (encap.data ()))
					};

					octets.write_tagged (addr);
				}

				Core::ReferenceRemoteRef unconfirmed_ref;
				Nirvana::Core::ImplStatic <Core::StreamInEncap> stm (std::ref (octets.data ()), true);
				obj = unmarshal_object (iid, stm, unconfirmed_ref);

			} else if (schema_eq ("rir", addr_begin, schema_end)) {
				// The rir protocol cannot be used with any other protocol in a URL.
				if (*prot_end == ',')
					throw BAD_PARAM (MAKE_OMG_MINOR (8));

				if (addr_end < str_end) {
					const Char* key_begin = addr_end + 1;
					obj = resolve_initial_references (Internal::StringView <Char> (key_begin, str_end - key_begin));
				} else
					obj = resolve_initial_references ("NameService");
			}
			if (obj)
				break;
			addr_begin = prot_end + 1;
		}
		if (!obj)
			throw BAD_PARAM (MAKE_OMG_MINOR (8));

	} else if (schema_eq ("corbaname", str_begin, schema_end)) {
		// TODO: Support
		throw BAD_PARAM (MAKE_OMG_MINOR (7));
	} else
		throw BAD_PARAM (MAKE_OMG_MINOR (7));

	return obj;
}

}
