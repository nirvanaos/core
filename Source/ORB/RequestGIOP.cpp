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
#include "RequestEncapIn.h"
#include "RequestEncapOut.h"
#include "TC_Interface.h"
#include "TC_Fixed.h"
#include "TC_Struct.h"
#include "TC_Union.h"
#include "TC_Enum.h"
#include "TC_String.h"
#include "TC_Sequence.h"
#include "TC_Array.h"
#include "TC_Alias.h"
#include "TC_Value.h"
#include "TC_ValueBox.h"
#include "TC_Abstract.h"
#include "TC_Native.h"
#include "../Binder.h"
#include <list>

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

RequestGIOP::RequestGIOP (unsigned GIOP_minor, bool client_side) :
	code_set_conv_ (CodeSetConverter::get_default ()),
	code_set_conv_w_ (CodeSetConverterW::get_default (GIOP_minor, client_side)),
	chunk_level_ (0)
{}

RequestGIOP::RequestGIOP (const RequestGIOP& src) :
	code_set_conv_ (src.code_set_conv_),
	code_set_conv_w_ (src.code_set_conv_w_),
	chunk_level_ (0)
{}

void RequestGIOP::set_out_size ()
{
	size_t size = stream_out_->size () - sizeof (GIOP::MessageHeader_1_3);
	if (sizeof (size_t) > sizeof (uint32_t) && size > std::numeric_limits <uint32_t>::max ())
		throw IMP_LIMIT ();
	((GIOP::MessageHeader_1_3*)stream_out_->header (sizeof (GIOP::MessageHeader_1_3)))->message_size ((uint32_t)size);
}

void RequestGIOP::invoke ()
{
	value_map_marshal_.clear ();
	rep_id_map_marshal_.clear ();
}

void RequestGIOP::unmarshal_end ()
{
	if (stream_in_) {
		top_level_tc_unmarshal_.clear ();
		value_map_marshal_.clear ();
		rep_id_map_unmarshal_.clear ();
		size_t more_data = !stream_in_->end ();
		stream_in_ = nullptr;
		if (more_data > 7) // 8-byte alignment is ignored
			throw MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
	}
}

bool RequestGIOP::marshal_chunk ()
{
	if (!marshal_op ())
		return false;
	if (chunk_level_) {
		if (stream_out_->chunk_size () >= CHUNK_SIZE_LIMIT)
			stream_out_->chunk_end ();
		stream_out_->chunk_begin ();
	}
	return true;
}

void RequestGIOP::marshal_type_code (TypeCode::_ptr_type tc, IndirectMapMarshal& map, size_t parent_offset)
{
	if (!tc) {
		stream_out_->write32 (0);
		return;
	}

	size_t off = stream_out_->size () + parent_offset;
	auto ins = map.emplace (&tc, off);
	if (!ins.second) {
		stream_out_->write32 (INDIRECTION_TAG);
		Long offset = (Long)(ins.first->second - (off + 4));
		stream_out_->write32 (offset);
		return;
	}

	ULong kind = (ULong)tc->kind ();
	stream_out_->write32 (kind);
	switch ((TCKind)kind) {
		case TCKind::tk_objref:
		case TCKind::tk_native:
		case TCKind::tk_abstract_interface:
		case TCKind::tk_local_interface: {
			Nirvana::Core::ImplStatic <StreamOutEncap> encap;
			encap.write_id_name (tc);
			stream_out_->write_seq (encap.data (), true);
		} break;

		case TCKind::tk_struct:
		case TCKind::tk_except: {
			size_t off = parent_offset + stream_out_->size () + 4;
			Nirvana::Core::ImplStatic <RequestEncapOut> encap (std::ref (*this));
			encap.stream_out ()->write_id_name (tc);
			ULong cnt = tc->member_count ();
			encap.stream_out ()->write32 (cnt);
			for (ULong i = 0; i < cnt; ++i) {
				IDL::String name = tc->member_name (i);
				encap.stream_out ()->write_string (name, true);
				encap.marshal_type_code (tc->member_type (i), map, off);
			}
			stream_out_->write_seq (encap.data (), true);
		} break;

		case TCKind::tk_union: {
			size_t off = parent_offset + stream_out_->size () + 4;
			Nirvana::Core::ImplStatic <RequestEncapOut> encap (std::ref (*this));
			encap.stream_out ()->write_id_name (tc);
			TypeCode::_ref_type discriminator_type = tc->discriminator_type ();
			encap.marshal_type_code (tc->discriminator_type (), map, off);
			Long default_index = tc->default_index ();
			encap.stream_out ()->write_c (alignof (long), sizeof (long), &default_index);
			ULong cnt = tc->member_count ();
			encap.stream_out ()->write32 (cnt);
			for (ULong i = 0; i < cnt; ++i) {
				if (i != default_index) {
					Any label = tc->member_label (i);
					discriminator_type->n_marshal_in (label.data (), 1, encap._get_ptr ());
				} else {
					// The discriminant value used in the actual typecode parameter associated with the default
					// member position in the list, may be any valid value of the discriminant type, and has no
					// semantic significance (i.e., it should be ignored and is only included for syntactic
					// completeness of union type code marshaling).
					ULongLong def = 0;
					discriminator_type->n_marshal_in (&def, 1, encap._get_ptr ());
				}
				IDL::String name = tc->member_name (i);
				encap.stream_out ()->write_string (name, true);
				encap.marshal_type_code (tc->member_type (i), map, off);
			}
			stream_out_->write_seq (encap.data (), true);
		} break;

		case TCKind::tk_enum: {
			Nirvana::Core::ImplStatic <StreamOutEncap> encap;
			encap.write_id_name (tc);
			ULong cnt = tc->member_count ();
			encap.write32 (cnt);
			for (ULong i = 0; i < cnt; ++i) {
				IDL::String name = tc->member_name (i);
				encap.write_string (name, true);
			}
			stream_out_->write_seq (encap.data (), true);
		} break;

		case TCKind::tk_string:
		case TCKind::tk_wstring:
			stream_out_->write32 (tc->length ());
			break;

		case TCKind::tk_sequence:
		case TCKind::tk_array: {
			Nirvana::Core::ImplStatic <RequestEncapOut> encap (std::ref (*this));
			encap.marshal_type_code (tc->content_type (), map, parent_offset + stream_out_->size () + 4);
			encap.stream_out ()->write32 (tc->length ());
			stream_out_->write_seq (encap.data (), true);
		} break;

		case TCKind::tk_alias: {
			Nirvana::Core::ImplStatic <RequestEncapOut> encap (std::ref (*this));
			encap.stream_out ()->write_id_name (tc);
			encap.marshal_type_code (tc->content_type (), map, parent_offset + stream_out_->size () + 4);
			stream_out_->write_seq (encap.data (), true);
		} break;

		case TCKind::tk_fixed: {
			struct
			{
				UShort d;
				Short s;
			} ds{ tc->fixed_digits (), tc->fixed_scale () };
			stream_out_->write_c (2, 4, &ds);
		} break;

		case TCKind::tk_value: {
			size_t off = parent_offset + stream_out_->size () + 4;
			Nirvana::Core::ImplStatic <RequestEncapOut> encap (std::ref (*this));
			encap.stream_out ()->write_id_name (tc);
			ValueModifier mod = tc->type_modifier ();
			encap.stream_out ()->write_c (alignof (ValueModifier), sizeof (ValueModifier), &mod);
			encap.marshal_type_code (tc->concrete_base_type (), map, off);
			ULong cnt = tc->member_count ();
			encap.stream_out ()->write32 (cnt);
			for (ULong i = 0; i < cnt; ++i) {
				IDL::String name = tc->member_name (i);
				encap.stream_out ()->write_string (name, true);
				encap.marshal_type_code (tc->member_type (i), map, off);
				Visibility vis = tc->member_visibility (i);
				encap.stream_out ()->write_c (alignof (Visibility), sizeof (Visibility), &vis);
			}
			stream_out_->write_seq (encap.data (), true);
		} break;

		case TCKind::tk_value_box: {
			Nirvana::Core::ImplStatic <RequestEncapOut> encap (std::ref (*this));
			encap.stream_out ()->write_id_name (tc);
			encap.marshal_type_code (tc->content_type (), map, parent_offset + stream_out_->size () + 4);
			stream_out_->write_seq (encap.data (), true);
		} break;

		default:
			throw BAD_TYPECODE ();
	}
}

TC_Ref RequestGIOP::unmarshal_type_code (IndirectMapUnmarshal& map, size_t parent_offset)
{
	size_t start_pos = stream_in_->position () + parent_offset;
	ULong kind = stream_in_->read32 ();

	if (INDIRECTION_TAG == kind) {
		Long off = stream_in_->read32 ();
		if (off >= -4)
			throw MARSHAL ();
		IndirectMapUnmarshal* pmap = parent_offset ? &map : &top_level_tc_unmarshal_;
		IndirectMapUnmarshal::iterator it = pmap->find (start_pos + 4 + off);
		if (it == pmap->end ())
			throw BAD_TYPECODE ();
		return TC_Ref (static_cast <TypeCode*> (it->second), !parent_offset);
	}

	// Parent offset of the encapsulated data:
	// start_pos + 4 byte kind + 4 byte encaplulated sequence size
	size_t encap_pos = start_pos + 8;

	TC_Ref ret;
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
			stream_in_->read_seq (encap);
			Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (encap));
			TC_Base::String id, name;
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
		} break;

		case TCKind::tk_struct:
		case TCKind::tk_except: {
			OctetSeq encap;
			stream_in_->read_seq (encap);
			Nirvana::Core::ImplStatic <RequestEncapIn> rq (std::ref (*this), std::ref (encap));
			TC_Base::String id, name;
			rq.stream_in ()->read_string (id);
			rq.stream_in ()->read_string (name);
			ULong cnt = rq.stream_in ()->read32 ();
			if (TCKind::tk_struct == (TCKind)kind && !cnt)
				throw BAD_TYPECODE (MAKE_OMG_MINOR (1));
			TC_Struct::Members members;
			if (cnt) {
				members.construct (cnt);
				TC_Struct::Member* pm = members.begin ();
				while (cnt--) {
					rq.stream_in ()->read_string (pm->name);
					if (!(pm->type = rq.unmarshal_type_code (map, encap_pos)))
						throw BAD_TYPECODE ();
					++pm;
				}
			}
			if (rq.stream_in ()->end () != 0)
				throw CORBA::MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
			ret = make_pseudo <TC_Struct> ((TCKind)kind, std::move (id), std::move (name), std::move (members));
		} break;

		case TCKind::tk_union: {
			OctetSeq encap;
			stream_in_->read_seq (encap);
			Nirvana::Core::ImplStatic <RequestEncapIn> rq (std::ref (*this), std::ref (encap));
			TC_Base::String id, name;
			rq.stream_in ()->read_string (id);
			rq.stream_in ()->read_string (name);
			TC_Ref discriminator_type = rq.unmarshal_type_code (map, encap_pos);
			Long default_index = rq.stream_in ()->read32 ();
			ULong cnt = rq.stream_in ()->read32 ();
			if (!cnt)
				throw BAD_TYPECODE (MAKE_OMG_MINOR (1));
			TC_Union::Members members;
			members.construct (cnt);
			TC_Union::Member* pm = members.begin ();
			for (ULong i = 0; i < cnt; ++pm, ++i) {
				ULongLong buf;
				discriminator_type->n_unmarshal (rq._get_ptr (), 1, &buf);
				// The discriminant value used in the actual typecode parameter associated with the default
				// member position in the list, may be any valid value of the discriminant type, and has no
				// semantic significance (i.e., it should be ignored and is only included for syntactic
				// completeness of union type code marshaling).
				if (i != default_index)
					pm->label.copy_from (discriminator_type, &buf);
				else
					pm->label <<= Any::from_octet (0);
				rq.stream_in ()->read_string (pm->name);
				if (!(pm->type = rq.unmarshal_type_code (map, encap_pos)))
					throw BAD_TYPECODE ();
			}
			if (rq.stream_in ()->end () != 0)
				throw CORBA::MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
			ret = make_pseudo <TC_Union> (id, name, std::move (discriminator_type), default_index,
				std::move (members));
		} break;

		case TCKind::tk_enum: {
			OctetSeq encap;
			stream_in_->read_seq (encap);
			Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (encap));
			TC_Base::String id, name;
			stm.read_string (id);
			stm.read_string (name);
			ULong cnt = stm.read32 ();
			if (!cnt)
				throw BAD_TYPECODE (MAKE_OMG_MINOR (1));
			TC_Enum::Members members;
			members.construct (cnt);
			TC_Base::String* pm = members.begin ();
			while (cnt--) {
				stm.read_string (*pm);
				++pm;
			}
			if (stm.end () != 0)
				throw CORBA::MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
			ret = make_pseudo <TC_Enum> (std::move (id), std::move (name), std::move (members));
		} break;

		case TCKind::tk_string: {
			ULong bound = stream_in_->read32 ();
			if (0 == bound)
				ret = _tc_string;
			else
				ret = make_pseudo <TC_String> (bound);
		} break;

		case TCKind::tk_wstring: {
			ULong bound = stream_in_->read32 ();
			if (0 == bound)
				ret = _tc_wstring;
			else
				ret = make_pseudo <TC_WString> (bound);
		} break;

		case TCKind::tk_sequence: {
			OctetSeq encap;
			stream_in_->read_seq (encap);
			Nirvana::Core::ImplStatic <RequestEncapIn> rq (std::ref (*this), std::ref (encap));
			TC_Ref content_type = rq.unmarshal_type_code (map, encap_pos);
			ULong bound = rq.stream_in ()->read32 ();
			if (rq.stream_in ()->end () != 0)
				throw CORBA::MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
			ret = make_pseudo <TC_Sequence> (std::move (content_type), bound);
		} break;

		case TCKind::tk_array: {
			OctetSeq encap;
			stream_in_->read_seq (encap);
			Nirvana::Core::ImplStatic <RequestEncapIn> rq (std::ref (*this), std::ref (encap));
			TC_Ref content_type = rq.unmarshal_type_code (map, encap_pos);
			ULong bound = rq.stream_in ()->read32 ();
			if (rq.stream_in ()->end () != 0)
				throw CORBA::MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
			ret = make_pseudo <TC_Array> (std::move (content_type), bound);
		} break;

		case TCKind::tk_alias: {
			OctetSeq encap;
			stream_in_->read_seq (encap);
			Nirvana::Core::ImplStatic <RequestEncapIn> rq (std::ref (*this), std::ref (encap));
			TC_Base::String id, name;
			rq.stream_in ()->read_string (id);
			rq.stream_in ()->read_string (name);
			TC_Ref content_type = rq.unmarshal_type_code (map, encap_pos);
			if (!content_type)
				throw BAD_TYPECODE ();
			if (rq.stream_in ()->end () != 0)
				throw CORBA::MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
			ret = make_pseudo <TC_Alias> (std::move (id), std::move (name), std::move (content_type));
		} break;

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
			stream_in_->read (alignof (UShort), sizeof (UShort), &digits);
			Short scale;
			stream_in_->read (alignof (Short), sizeof (Short), &scale);
			if (stream_in_->other_endian ()) {
				byteswap (digits);
				byteswap (scale);
			}
			ret = make_pseudo <TC_Fixed> (digits, scale);
		} break;

		case TCKind::tk_value: {
			OctetSeq encap;
			stream_in_->read_seq (encap);
			Nirvana::Core::ImplStatic <RequestEncapIn> rq (std::ref (*this), std::ref (encap));
			TC_Base::String id, name;
			rq.stream_in ()->read_string (id);
			rq.stream_in ()->read_string (name);
			ValueModifier modifier;
			rq.stream_in ()->read (alignof (ValueModifier), sizeof (ValueModifier), &modifier);
			if (rq.stream_in ()->other_endian ())
				byteswap (modifier);
			TC_Ref concrete_base = rq.unmarshal_type_code (map, encap_pos);
			ULong cnt = rq.stream_in ()->read32 ();
			TC_Value::Members members;
			if (cnt) {
				members.construct (cnt);
				TC_Value::Member* pm = members.begin ();
				while (cnt--) {
					rq.stream_in ()->read_string (pm->name);
					if (!(pm->type = rq.unmarshal_type_code (map, encap_pos)))
						throw BAD_TYPECODE ();
					rq.stream_in ()->read (alignof (Short), sizeof (Short), &pm->visibility);
					++pm;
				}
			}
			if (rq.stream_in ()->end () != 0)
				throw CORBA::MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
			ret = make_pseudo <TC_Value> (std::move (id), std::move (name), modifier,
				std::move (concrete_base), std::move (members));
		} break;

		case TCKind::tk_value_box: {
			OctetSeq encap;
			stream_in_->read_seq (encap);
			Nirvana::Core::ImplStatic <RequestEncapIn> rq (std::ref (*this), std::ref (encap));
			TC_Base::String id, name;
			rq.stream_in ()->read_string (id);
			rq.stream_in ()->read_string (name);
			TC_Ref content_type = rq.unmarshal_type_code (map, encap_pos);
			if (!content_type)
				throw BAD_TYPECODE ();
			if (rq.stream_in ()->end () != 0)
				throw CORBA::MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
			ret = make_pseudo <TC_ValueBox> (std::move (id), std::move (name), std::move (content_type));
		} break;

		default:
			throw BAD_TYPECODE ();
	}

	map.emplace (start_pos, &ret);
	if (!parent_offset)
		top_level_tc_unmarshal_.emplace (start_pos, &ret);

	return ret;
}

void RequestGIOP::marshal_value (Interface::_ptr_type val, bool output)
{
	if (!marshal_op ())
		return;

	if (chunk_level_)
		stream_out_->chunk_end ();

	if (!val) {
		stream_out_->write32 (0);
		return;
	}

	ValueBase::_ptr_type base = value_type2base (val);
	size_t pos = stream_out_->size ();
	auto ins = value_map_marshal_.emplace (&base, pos);
	if (!ins.second) {
		stream_out_->write32 (INDIRECTION_TAG);
		Long off = ins.first->second - (pos + 4);
		stream_out_->write32 (off);
		return;
	}
	Internal::Interface::_ptr_type primary = base->_query_valuetype (nullptr);
	Long tag = 0x7FFFFF00;
	if (&primary != &val) {
		// Parameter type corresponds to the primary type - we need to provide type information.
		TypeCode::_ref_type truncatable_base = base->_truncatable_base ();
		if (truncatable_base) {
			// Truncatable - provide a list of repository IDs
			tag |= 6 | 8; // List of IDs and chunking
			--chunk_level_;
			std::list <IDL::String> list;
			list.emplace_back (primary->_epv ().interface_id);
			for (;;) {
				list.emplace_back (truncatable_base->id ());
				truncatable_base = truncatable_base->concrete_base_type ();
				if (!truncatable_base || truncatable_base->type_modifier () != VM_TRUNCATABLE)
					break;
			}
			stream_out_->write32 (tag);
			stream_out_->write_size (list.size ());
			for (IDL::String& id : list) {
				marshal_rep_id (std::move (id));
			}
		} else {
			// Single ID
			tag |= 2; // Single ID
			if (chunk_level_) {
				--chunk_level_;
				tag |= 8;
			}
			stream_out_->write32 (tag);
			marshal_rep_id (primary->_epv ().interface_id);
		}
	} else {
		if (chunk_level_) {
			--chunk_level_;
			tag |= 8;
		}
		stream_out_->write32 (tag);
	}
	base->_marshal (_get_ptr ());
	if (chunk_level_) {
		stream_out_->chunk_end ();
		stream_out_->write32 (chunk_level_);
		++chunk_level_;
	}
}

void RequestGIOP::marshal_rep_id (IDL::String&& id)
{
	size_t pos = stream_out_->size ();
	auto ins = rep_id_map_marshal_.emplace (std::move (id), pos);
	if (!ins.second) {
		stream_out_->write32 (INDIRECTION_TAG);
		Long off = (Long)(ins.first->second - (pos + 4));
		stream_out_->write32 (off);
	} else {
		stream_out_->write_string_c (ins.first->first);
	}
}

Interface::_ref_type RequestGIOP::unmarshal_value (const IDL::String& interface_id)
{
	if (chunk_level_) {
		if (stream_in_->chunk_tail ())
			throw MARSHAL (); // Unexpected
		stream_in_->chunk_mode (false);
	}
	size_t start_pos = stream_in_->position ();
	Long value_tag = stream_in_->read32 ();
	if (0 == value_tag)
		return nullptr;

	if (INDIRECTION_TAG == value_tag) {
		Long off = stream_in_->read32 ();
		if (off >= -4)
			throw MARSHAL ();
		auto it = value_map_unmarshal_.find (start_pos + 4 + off);
		if (it == value_map_unmarshal_.end ())
			throw MARSHAL ();
		Interface::_ptr_type itf = static_cast <ValueBase*> (it->second)->_query_valuetype (interface_id);
		if (!itf)
			throw MARSHAL (); // Unexpected
		return itf;
	}

	if (value_tag < 0x7FFFFF00)
		throw MARSHAL (); // Unexpected

	// Skip codebase_URL, if any
	if (value_tag & 1) {
		ULong string_len = stream_in_->read32 ();
		if (INDIRECTION_TAG == string_len)
			stream_in_->read (alignof (Long), sizeof (Long), nullptr); // Skip offset
		else
			stream_in_->read (alignof (Char), string_len, nullptr); // Skip string
	}

	// Read type information
	std::vector <IDL::String> type_info;
	switch (value_tag & 0x00000006) {
		case 0: // No type information
			type_info.emplace_back (interface_id);
			break;
		case 2: // Single repository id
			type_info.emplace_back ();
			stream_in_->read_string (type_info.front ());
			break;
		case 6: { // Sequence of the repository IDs
			ULong count = stream_in_->read32 ();
			type_info.resize (count);
			for (IDL::String& s : type_info) {
				stream_in_->read_string (s);
			}
		} break;

		default: // 4 is illegal value
			throw MARSHAL ();
	}

	ValueFactoryBase::_ref_type factory;
	bool truncate = false;
	for (const IDL::String& id : type_info) {
		try {
			factory = Binder::bind_interface <ValueFactoryBase> (id);
		} catch (...) {
			truncate = true;
		}
	}

	if (!factory)
		throw MARSHAL (MAKE_OMG_MINOR (1)); // Unable to locate value factory.

	ValueBase::_ref_type base (factory->create_for_unmarshal ());
	Interface::_ref_type ret (base->_query_valuetype (interface_id));
	if (!ret)
		throw MARSHAL (); // Unexpected

	value_map_unmarshal_.emplace (start_pos, &ValueBase::_ptr_type (base));

	if (value_tag & 0x00000008) {
		stream_in_->chunk_mode (true);
		--chunk_level_;
	}

	base->_unmarshal (_get_ptr ());

	Long end_tag;
	if (truncate)
		end_tag = stream_in_->skip_chunks ();
	else if (chunk_level_) {
		if (stream_in_->chunk_tail ())
			throw MARSHAL ();
		stream_in_->chunk_mode (false);
		end_tag = stream_in_->read32 ();
	}
	
	if (chunk_level_) {
		if (end_tag >= 0 || end_tag < chunk_level_)
			throw MARSHAL ();
		chunk_level_ = end_tag + 1;
	}

	return ret;
}

const IDL::String& RequestGIOP::unmarshal_rep_id ()
{
	size_t pos = stream_in_->position ();
	ULong size = stream_in_->read32 ();
	if (INDIRECTION_TAG == size) {
		Long off = stream_in_->read32 ();
		pos += off + 4;
		auto it = rep_id_map_unmarshal_.find (pos);
		if (it == rep_id_map_unmarshal_.end ())
			throw MARSHAL ();
		return it->second;
	}
	IDL::String id;
	id.resize (size - 1);
	Char* buf = &*id.begin ();
	stream_in_->read (1, size, buf);
	if (buf [size - 1]) // Not zero-terminated
		throw MARSHAL ();
	return rep_id_map_unmarshal_.emplace (pos, std::move (id)).first->second;
}

}
}
