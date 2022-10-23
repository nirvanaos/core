/*
* Nirvana IDL support library.
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
#include "TC_FactoryImpl.h"
#include "ProxyLocal.h"
#include "../ExecDomain.h"
#include "StreamInEncap.h"

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

std::atomic_flag TC_FactoryImpl::GC_scheduled_ = ATOMIC_FLAG_INIT;

inline void TC_FactoryImpl::schedule_GC () NIRVANA_NOEXCEPT
{
	if (!GC_scheduled_.test_and_set ()) {
		ExecDomain& ed = ExecDomain::current ();
		auto saved = ed.deadline_policy_oneway ();
		try {
			System::DeadlinePolicy dp;
			if (PROXY_GC_DEADLINE == INFINITE_DEADLINE)
				dp._d (System::DeadlinePolicyType::DEADLINE_INFINITE);
			else
				dp.timeout (PROXY_GC_DEADLINE);
			ed.deadline_policy_oneway (dp);
			TC_Factory::_narrow (Services::bind (Services::TC_Factory))->collect_garbage ();
		} catch (...) {
		}
		ed.deadline_policy_oneway (saved);
	}
}

void TC_ComplexBase::collect_garbage () NIRVANA_NOEXCEPT
{
	TC_FactoryImpl::schedule_GC ();
}

void TC_FactoryImpl::check_name (const IDL::String& name, ULong minor)
{
	if (!name.empty ()) {
		Char c = name.front ();
		if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')) {
			const Char* p = name.data () + 1, * end = p + name.size () - 1;
			for (; p != end; ++p) {
				c = *p;
				if (!(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || '_' == c))
					break;
			}
			if (p == end)
				return;
		}
	}
	throw BAD_PARAM (minor);
}

void TC_FactoryImpl::check_member_name (NameSet& names, const IDL::String& name)
{
	// Operations that take members shall check that the member names are valid IDL names
	// and that they are unique within the member list, and if the name is found to be incorrect,
	// they shall raise a BAD_PARAM with standard minor code 17.
	if (!name.empty ()) {
		check_name (name, MAKE_OMG_MINOR (17));
		if (!names.insert (name).second)
			throw BAD_PARAM (MAKE_OMG_MINOR (17));
	}
}

void TC_FactoryImpl::check_id (const IDL::String& id)
{
	// Operations that take a RepositoryId argument shall check that the argument passed in is
	// a string of the form <format> : <string>and if not, then raise a BAD_PARAM exception with
	// standard minor code 16.
	size_t col = id.find (':');
	if (col == IDL::String::npos || col == 0 || col == id.size () - 1)
		throw BAD_PARAM (MAKE_OMG_MINOR (16));
}

TC_Ref TC_FactoryImpl::check_type (TypeCode::_ptr_type tc) const
{
	// Operations that take content or member types as arguments shall check that they are legitimate
	// (i.e., that they don’t have kinds tk_null, tk_void, or tk_exception). If not, they shall raise
	// the BAD_TYPECODE exception with standard minor code 2.
	if (tc) {
		TC_ComplexBase* complex = complex_base (tc);
		if (complex) {
			if (complex->is_recursive () || tc->kind () != TCKind::tk_except)
				return TC_Ref (tc, complex);
		} else {
			switch (tc->kind ()) {
				case TCKind::tk_null:
				case TCKind::tk_void:
				case TCKind::tk_except:
					break;
				default:
					return TC_Ref (tc);
			}
		}
	}
	throw BAD_TYPECODE (MAKE_OMG_MINOR (2));
}

TypeCode::_ref_type TC_FactoryImpl::create_struct_tc (TCKind kind, RepositoryId&& id,
	Identifier&& name, StructMemberSeq& members)
{
	check_name (name);
	NameSet names;
	TC_Struct::Members smembers;
	if (members.size ()) {
		smembers.construct (members.size ());
		TC_Struct::Member* pm = smembers.begin ();
		for (StructMember& m : members) {
			check_member_name (names, m.name ());
			pm->name = std::move (m.name ());
			pm->type = check_type (m.type ());
			++pm;
		}
	}
	servant_reference <TC_Struct> ref =
		make_reference <TC_Struct> (kind, std::move (id), std::move (name), std::move (smembers));
	TypeCode::_ptr_type tc = ref->_get_ptr ();
	complex_objects_.emplace (&tc, ref);
	return tc;
}

TypeCode::_ref_type TC_FactoryImpl::create_interface_tc (TCKind kind, const RepositoryId& id,
	const Identifier& name)
{
	check_id (id);
	check_name (name);
	return make_pseudo <TC_Interface> (kind, id, name);
}

TC_ComplexBase* TC_FactoryImpl::complex_base (TypeCode::_ptr_type p) const NIRVANA_NOEXCEPT
{
	if (!p)
		return nullptr;
	auto it = complex_objects_.find (&p);
	if (it == complex_objects_.end ())
		return nullptr;
	else
		return it->second;
}

TypeCode::_ref_type TC_FactoryImpl::unmarshal_type_code (StreamIn& stream, IndirectMapUnmarshal& indirect_map, size_t parent_offset)
{
	ULong kind = stream.read32 ();

	if (INDIRECTION_TAG == kind) {
		size_t pos = stream.position () + parent_offset;
		Long off = stream.read32 ();
		if (off >= -4)
			throw MARSHAL ();
		IndirectMapUnmarshal::iterator it = indirect_map.find (pos + off);
		if (it == indirect_map.end ())
			throw BAD_TYPECODE ();
		return TypeCode::_ptr_type (static_cast <TypeCode*> (it->second));
	}

	return unmarshal_type_code (kind, stream, indirect_map, parent_offset);
}

TypeCode::_ref_type TC_FactoryImpl::unmarshal_type_code (ULong kind, StreamIn& stream, IndirectMapUnmarshal& indirect_map, size_t parent_offset)
{
	size_t start_pos = stream.position () + parent_offset - 4;

	// Parent offset of the encapsulated data:
	// start_pos + 4 byte kind + 4 byte encaplulated sequence size
	size_t encap_pos = start_pos + 8;

	TypeCode::_ref_type ret;
	switch ((TCKind)kind) {
		case TCKind::tk_null:
			break;

		case TCKind::tk_void:
			ret = _tc_void;
			break;

		case TCKind::tk_short:
			ret = _tc_short;
			break;

		case TCKind::tk_long:
			ret = _tc_long;
			break;

		case TCKind::tk_ushort:
			ret = _tc_ushort;
			break;

		case TCKind::tk_ulong:
			ret = _tc_ulong;
			break;

		case TCKind::tk_float:
			ret = _tc_float;
			break;

		case TCKind::tk_double:
			ret = _tc_double;
			break;

		case TCKind::tk_boolean:
			ret = _tc_boolean;
			break;

		case TCKind::tk_char:
			ret = _tc_char;
			break;

		case TCKind::tk_octet:
			ret = _tc_octet;
			break;

		case TCKind::tk_any:
			ret = _tc_any;
			break;

		case TCKind::tk_TypeCode:
			ret = _tc_TypeCode;
			break;

		case TCKind::tk_objref:
		case TCKind::tk_abstract_interface:
		case TCKind::tk_local_interface:
		case TCKind::tk_native: {
			OctetSeq encap;
			stream.read_seq (encap);
			Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (encap));
			IDL::String id, name;
			stm.read_string (id);
			stm.read_string (name);
			if (stm.end () != 0)
				throw CORBA::MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
			switch ((TCKind)kind) {
				case TCKind::tk_abstract_interface:
					ret = make_pseudo <TC_Abstract> (std::move (id), std::move (name));
					break;
				case TCKind::tk_native:
					ret = make_pseudo <TC_Native> (std::move (id), std::move (name));
					break;
				default:
					ret = make_pseudo <TC_Interface> ((TCKind)kind, std::move (id), std::move (name));
			}
			indirect_map.emplace (start_pos, &TypeCode::_ptr_type (ret));
		} break;

		case TCKind::tk_struct:
		case TCKind::tk_except: {
			OctetSeq encap;
			stream.read_seq (encap);
			Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (encap));
			IDL::String id, name;
			stm.read_string (id);
			stm.read_string (name);
			ULong cnt = stm.read32 ();
			if (TCKind::tk_struct == (TCKind)kind && !cnt)
				throw BAD_TYPECODE ();
			servant_reference <TC_Struct> ref = make_reference <TC_Struct> ((TCKind)kind, std::move (id), std::move (name));
			TypeCode::_ptr_type tc = ref->_get_ptr ();
			complex_objects_.emplace (&tc, ref);
			indirect_map.emplace (start_pos, &tc);
			try {
				TC_Struct::Members members;
				if (cnt) {
					members.construct (cnt);
					TC_Struct::Member* pm = members.begin ();
					while (cnt--) {
						stm.read_string (pm->name);
						TypeCode::_ref_type mt = unmarshal_type_code (stm, indirect_map, encap_pos);
						if (!mt)
							throw BAD_TYPECODE (MAKE_OMG_MINOR (2));
						pm->type = TC_Ref (mt, complex_base (mt));
						++pm;
					}
				}
				if (stm.end () != 0)
					throw CORBA::MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
				ref->set_members (std::move (members));
				return tc;
			} catch (...) {
				indirect_map.erase (start_pos);
				throw;
			}
		}

		case TCKind::tk_union: {
			OctetSeq encap;
			stream.read_seq (encap);
			Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (encap));
			IDL::String id, name;
			stm.read_string (id);
			stm.read_string (name);
			TypeCode::_ref_type discriminator_type = unmarshal_type_code (stm, indirect_map,
				encap_pos);
			if (!discriminator_type)
				throw BAD_TYPECODE (MAKE_OMG_MINOR (2));
			size_t discriminator_size = discriminator_type->n_size ();
			Long default_index = stm.read32 ();
			ULong cnt = stm.read32 ();
			if (!cnt)
				throw BAD_TYPECODE ();
			servant_reference <TC_Union> ref = make_reference <TC_Union> (std::move (id), std::move (name),
				std::move (discriminator_type), default_index);
			TypeCode::_ptr_type tc = ref->_get_ptr ();
			complex_objects_.emplace (&tc, ref);
			indirect_map.emplace (start_pos, &tc);
			try {
				TC_Union::Members members;
				members.construct (cnt);
				TC_Union::Member* pm = members.begin ();
				for (ULong i = 0; i < cnt; ++pm, ++i) {
					ULongLong buf;
					stm.read (discriminator_size, discriminator_size, &buf);
					if (stm.other_endian ())
						discriminator_type->n_byteswap (&buf, 1);
					// The discriminant value used in the actual typecode parameter associated with the default
					// member position in the list, may be any valid value of the discriminant type, and has no
					// semantic significance (i.e., it should be ignored and is only included for syntactic
					// completeness of union type code marshaling).
					if (i != default_index)
						pm->label.copy_from (discriminator_type, &buf);
					else
						pm->label <<= Any::from_octet (0);
					stm.read_string (pm->name);
					TypeCode::_ref_type mt = unmarshal_type_code (stm, indirect_map, encap_pos);
					if (!mt)
						throw BAD_TYPECODE (MAKE_OMG_MINOR (2));
					pm->type = TC_Ref (mt, complex_base (mt));
				}
				if (stm.end () != 0)
					throw CORBA::MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
				ref->set_members (std::move (members));
				return tc;
			} catch (...) {
				indirect_map.erase (start_pos);
				throw;
			}
		}

		case TCKind::tk_enum: {
			OctetSeq encap;
			stream.read_seq (encap);
			Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (encap));
			IDL::String id, name;
			stm.read_string (id);
			stm.read_string (name);
			ULong cnt = stm.read32 ();
			if (!cnt)
				throw BAD_TYPECODE ();
			TC_Enum::Members members;
			members.construct (cnt);
			IDL::String* pm = members.begin ();
			while (cnt--) {
				stm.read_string (*pm);
				++pm;
			}
			if (stm.end () != 0)
				throw CORBA::MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
			ret = make_pseudo <TC_Enum> (std::move (id), std::move (name), std::move (members));
			indirect_map.emplace (start_pos, &TypeCode::_ptr_type (ret));
		} break;

		case TCKind::tk_string: {
			ULong bound = stream.read32 ();
			if (0 == bound)
				ret = _tc_string;
			else
				ret = make_pseudo <TC_String> (bound);
		} break;

		case TCKind::tk_wstring: {
			ULong bound = stream.read32 ();
			if (0 == bound)
				ret = _tc_wstring;
			else
				ret = make_pseudo <TC_WString> (bound);
		} break;

		case TCKind::tk_sequence: {
			servant_reference <TC_Sequence> ref = make_reference <TC_Sequence> ();
			TypeCode::_ptr_type tc = ref->_get_ptr ();
			complex_objects_.emplace (&tc, ref);
			indirect_map.emplace (start_pos, &tc);
			try {
				OctetSeq encap;
				stream.read_seq (encap);
				Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (encap));
				TypeCode::_ref_type content_type = unmarshal_type_code (stm, indirect_map, encap_pos);
				if (!content_type)
					throw BAD_TYPECODE ();
				ULong bound = stm.read32 ();
				if (stm.end () != 0)
					throw CORBA::MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
				ref->set_content_type (TC_Ref (content_type, complex_base (content_type)), bound);
				return tc;
			} catch (...) {
				indirect_map.erase (start_pos);
				throw;
			}
		}

		case TCKind::tk_array: {
			servant_reference <TC_Array> ref = make_reference <TC_Array> ();
			TypeCode::_ptr_type tc = ref->_get_ptr ();
			complex_objects_.emplace (&tc, ref);
			indirect_map.emplace (start_pos, &tc);
			try {
				OctetSeq encap;
				stream.read_seq (encap);
				Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (encap));
				TypeCode::_ref_type content_type = unmarshal_type_code (stm, indirect_map, encap_pos);
				if (!content_type)
					throw BAD_TYPECODE ();
				ULong bound = stm.read32 ();
				if (stm.end () != 0)
					throw CORBA::MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
				ref->set_content_type (TC_Ref (content_type, complex_base (content_type)), bound);
				return tc;
			} catch (...) {
				indirect_map.erase (start_pos);
				throw;
			}
		}

		case TCKind::tk_alias: {
			OctetSeq encap;
			stream.read_seq (encap);
			Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (encap));
			IDL::String id, name;
			stm.read_string (id);
			stm.read_string (name);
			servant_reference <TC_Alias> ref = make_reference <TC_Alias> (std::move (id), std::move (name));
			TypeCode::_ptr_type tc = ref->_get_ptr ();
			complex_objects_.emplace (&tc, ref);
			indirect_map.emplace (start_pos, &tc);
			try {
				TC_Ref content_type = unmarshal_type_code (stm, indirect_map, encap_pos);
				if (!content_type)
					throw BAD_TYPECODE ();
				if (stm.end () != 0)
					throw CORBA::MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
				ref->set_content_type (TC_Ref (content_type, complex_base (content_type)));
				return tc;
			} catch (...) {
				indirect_map.erase (start_pos);
				throw;
			}
		}

		case TCKind::tk_longlong:
			ret = _tc_longlong;
			break;

		case TCKind::tk_ulonglong:
			ret = _tc_ulonglong;
			break;

		case TCKind::tk_longdouble:
			ret = _tc_longdouble;
			break;

		case TCKind::tk_wchar:
			ret = _tc_wchar;
			break;

		case TCKind::tk_fixed: {
			UShort digits;
			stream.read (alignof (UShort), sizeof (UShort), &digits);
			Short scale;
			stream.read (alignof (Short), sizeof (Short), &scale);
			if (stream.other_endian ()) {
				byteswap (digits);
				byteswap (scale);
			}
			ret = make_pseudo <TC_Fixed> (digits, scale);
		} break;

		case TCKind::tk_value: {
			OctetSeq encap;
			stream.read_seq (encap);
			Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (encap));
			IDL::String id, name;
			stm.read_string (id);
			stm.read_string (name);
			ValueModifier modifier;
			stm.read (alignof (ValueModifier), sizeof (ValueModifier), &modifier);
			if (stm.other_endian ())
				byteswap (modifier);

			servant_reference <TC_Value> ref = make_reference <TC_Value> (std::move (id), std::move (name), modifier);
			TypeCode::_ptr_type tc = ref->_get_ptr ();
			complex_objects_.emplace (&tc, ref);
			indirect_map.emplace (start_pos, &tc);
			try {
				TypeCode::_ref_type concrete_base = unmarshal_type_code (stm, indirect_map, encap_pos);
				ULong cnt = stm.read32 ();
				TC_Value::Members members;
				if (cnt) {
					members.construct (cnt);
					TC_Value::Member* pm = members.begin ();
					while (cnt--) {
						stm.read_string (pm->name);
						TypeCode::_ref_type mt = unmarshal_type_code (stm, indirect_map, encap_pos);
						if (!mt)
							throw BAD_TYPECODE (MAKE_OMG_MINOR (2));
						pm->type = TC_Ref (mt, complex_base (mt));
						Short visibility;
						stm.read (alignof (Short), sizeof (Short), &visibility);
						if (stm.other_endian ())
							byteswap (visibility);
						pm->visibility = visibility;
						++pm;
					}
				}
				if (stm.end () != 0)
					throw CORBA::MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
				ref->set_members (TC_Ref (concrete_base, complex_base (concrete_base)), std::move (members));
				return tc;
			} catch (...) {
				indirect_map.erase (start_pos);
				throw;
			}
		}

		case TCKind::tk_value_box: {
			OctetSeq encap;
			stream.read_seq (encap);
			Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (encap));
			IDL::String id, name;
			stm.read_string (id);
			stm.read_string (name);
			servant_reference <TC_ValueBox> ref = make_reference <TC_ValueBox> (std::move (id), std::move (name));
			TypeCode::_ptr_type tc = ref->_get_ptr ();
			complex_objects_.emplace (&tc, ref);
			indirect_map.emplace (start_pos, &tc);
			try {
				TypeCode::_ref_type content_type = unmarshal_type_code (stm, indirect_map, encap_pos);
				if (!content_type)
					throw BAD_TYPECODE ();
				if (stm.end () != 0)
					throw CORBA::MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
				ref->set_content_type (TC_Ref (content_type, complex_base (content_type)));
				return tc;
			} catch (...) {
				indirect_map.erase (start_pos);
				throw;
			}
		} break;

		default:
			throw BAD_TYPECODE ();
	}

	return ret;
}

}
}
