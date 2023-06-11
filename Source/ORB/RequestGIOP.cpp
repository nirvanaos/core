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
#include "RequestGIOP.h"
#include "StreamOutEncap.h"
#include "TC_FactoryImpl.h"
#include "../Binder.h"
#include "unmarshal_object.h"
#include <list>

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

RequestGIOP::RequestGIOP (unsigned GIOP_minor, unsigned response_flags, Domain* servant_domain) :
	GIOP_minor_ (GIOP_minor),
	response_flags_ (response_flags),
	deadline_ (INFINITE_DEADLINE),
	chunk_level_ (0),
	target_domain_ (servant_domain),
	mem_context_ (&MemContext::current ()),
	code_set_conv_ (CodeSetConverter::get_default ()),
	code_set_conv_w_ (CodeSetConverterW::get_default (GIOP_minor, servant_domain != nullptr)),
	top_level_tc_unmarshal_ (mem_context_->heap ()),
	value_map_marshal_ (mem_context_->heap ()),
	value_map_unmarshal_ (mem_context_->heap ()),
	rep_id_map_marshal_ (mem_context_->heap ()),
	rep_id_map_unmarshal_ (mem_context_->heap ()),
	marshaled_DGC_references_ (mem_context_->heap ()),
	references_to_confirm_ (mem_context_->heap ())
{}

RequestGIOP::~RequestGIOP ()
{}

void RequestGIOP::_remove_ref () noexcept
{
	if (!ref_cnt_.decrement ()) {
		ExecDomain& ed = ExecDomain::current ();
		Ref <MemContext> mc = std::move (mem_context_);
		ed.mem_context_replace (*mc);
		delete this;
		ed.mem_context_restore ();
	}
}

void RequestGIOP::set_out_size ()
{
	size_t size = stream_out_->size () - sizeof (GIOP::MessageHeader_1_0);
	if (sizeof (size_t) > sizeof (uint32_t) && size > std::numeric_limits <uint32_t>::max ())
		throw IMP_LIMIT ();
	((GIOP::MessageHeader_1_0*)stream_out_->header (sizeof (GIOP::MessageHeader_1_0)))->message_size ((uint32_t)size);
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
		value_map_unmarshal_.clear ();
		rep_id_map_unmarshal_.clear ();
		size_t more_data = stream_in_->end ();
		stream_in_ = nullptr;
		if (more_data > 7) // 8-byte alignment is ignored
			throw MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
		if (!references_to_confirm_.empty ()) {
			Domain::confirm_DGC_references (references_to_confirm_.size (), references_to_confirm_.data ());
			references_to_confirm_.clear ();
		}
	}
}

bool RequestGIOP::marshal_chunk ()
{
	if (!marshal_op ())
		return false;
	assert (target_domain_);
	if (chunk_level_) {
		if (stream_out_->chunk_size () >= CHUNK_SIZE_LIMIT)
			stream_out_->chunk_end ();
		stream_out_->chunk_begin ();
	}
	return true;
}

bool RequestGIOP::unmarshal (size_t align, size_t size, void* data)
{
	check_align (align);
	stream_in_->read (align, size, data);
	return stream_in_->other_endian ();
}

bool RequestGIOP::unmarshal_seq (size_t align, size_t element_size, size_t& element_count, void*& data,
	size_t& allocated_size)
{
	check_align (align);
	return stream_in_->read_seq (align, element_size, element_count, data, allocated_size);
}

void RequestGIOP::marshal_object (Object::_ptr_type obj)
{
	ReferenceRef ref = ProxyManager::cast (obj)->marshal (*stream_out_);
	if (ref && (ref->flags () & Reference::GARBAGE_COLLECTION))
		marshaled_DGC_references_.emplace (std::move (ref));
}

Interface::_ref_type RequestGIOP::unmarshal_interface (const IDL::String& interface_id)
{
	ReferenceRemoteRef unconfirmed_remote_ref;
	Object::_ref_type obj = unmarshal_object (*stream_in_, unconfirmed_remote_ref);
	Interface::_ref_type ret;
	if (obj) {
		if (RepId::compatible (RepIdOf <Object>::id, interface_id))
			ret = std::move (obj);
		else {
			ret = obj->_query_interface (interface_id);
			if (!ret)
				throw INV_OBJREF ();
		}
		if (unconfirmed_remote_ref)
			references_to_confirm_.push_back (std::move (unconfirmed_remote_ref));
	}
	return ret;
}

void RequestGIOP::marshal_type_code (StreamOut& stream, TypeCode::_ptr_type tc, IndirectMapMarshal& map, size_t parent_offset)
{
	if (!tc) {
		stream.write32 (0);
		return;
	}

	ULong kind;
	try {
		kind = (ULong)tc->kind ();
	} catch (...) {
		throw BAD_TYPECODE (MAKE_OMG_MINOR (1)); // Attempt to marshal incomplete TypeCode.
	}

	if (TC_FactoryImpl::is_simple_type ((TCKind)kind)) {
		stream.write32 (kind);
		return;
	}

	switch ((TCKind)kind) {
		case TCKind::tk_string:
		case TCKind::tk_wstring:
			stream.write32 (kind);
			stream.write32 (tc->length ());
			return;

		case TCKind::tk_fixed: {
			stream.write32 (kind);

			struct
			{
				UShort d;
				Short s;
			} ds { tc->fixed_digits (), tc->fixed_scale () };
			stream.write_c (2, 4, &ds);
		} return;
	}

	size_t pos = round_up (stream.size (), (size_t)4) + parent_offset;
	{
		auto ins = map.emplace (&tc, pos);
		if (!ins.second) {
			stream.write32 (INDIRECTION_TAG);
			Long offset = (Long)(ins.first->second - (pos + 4));
			stream.write32 (offset);
			return;
		}
	}

	stream.write32 (kind);
	ImplStatic <StreamOutEncap> encap;
	pos += 8; // parent_offset for the next level calls
	switch ((TCKind)kind) {
		case TCKind::tk_objref:
		case TCKind::tk_native:
		case TCKind::tk_abstract_interface:
		case TCKind::tk_local_interface:
			encap.write_id_name (tc);
			break;

		case TCKind::tk_struct:
		case TCKind::tk_except: {
			encap.write_id_name (tc);
			ULong cnt = tc->member_count ();
			encap.write32 (cnt);
			for (ULong i = 0; i < cnt; ++i) {
				IDL::String name = tc->member_name (i);
				encap.write_string (name, true);
				marshal_type_code (encap, tc->member_type (i), map, pos);
			}
		} break;

		case TCKind::tk_union: {
			encap.write_id_name (tc);
			TypeCode::_ref_type discriminator_type = tc->discriminator_type ();
			marshal_type_code (encap, discriminator_type, map, pos);
			size_t discriminator_size = discriminator_type->n_size ();
			Long default_index = tc->default_index ();
			encap.write_c (alignof (long), sizeof (long), &default_index);
			ULong cnt = tc->member_count ();
			encap.write32 (cnt);
			for (ULong i = 0; i < cnt; ++i) {
				if (i != default_index) {
					Any label = tc->member_label (i);
					encap.write_c (discriminator_size, discriminator_size, label.data ());
				} else {
					// The discriminant value used in the actual typecode parameter associated with the default
					// member position in the list, may be any valid value of the discriminant type, and has no
					// semantic significance (i.e., it should be ignored and is only included for syntactic
					// completeness of union type code marshaling).
					ULongLong def = 0;
					encap.write_c (discriminator_size, discriminator_size, &def);
				}
				IDL::String name = tc->member_name (i);
				encap.write_string (name, true);
				marshal_type_code (encap, tc->member_type (i), map, pos);
			}
		} break;

		case TCKind::tk_enum: {
			encap.write_id_name (tc);
			ULong cnt = tc->member_count ();
			encap.write32 (cnt);
			for (ULong i = 0; i < cnt; ++i) {
				IDL::String name = tc->member_name (i);
				encap.write_string (name, true);
			}
		} break;

		case TCKind::tk_sequence:
		case TCKind::tk_array:
			marshal_type_code (encap, tc->content_type (), map, pos);
			encap.write32 (tc->length ());
			break;

		case TCKind::tk_alias:
			encap.write_id_name (tc);
			marshal_type_code (encap, tc->content_type (), map, pos);
			break;

		case TCKind::tk_value: {
			encap.write_id_name (tc);
			ValueModifier mod = tc->type_modifier ();
			encap.write_c (alignof (ValueModifier), sizeof (ValueModifier), &mod);
			marshal_type_code (encap, tc->concrete_base_type (), map, pos);
			ULong cnt = tc->member_count ();
			encap.write32 (cnt);
			for (ULong i = 0; i < cnt; ++i) {
				IDL::String name = tc->member_name (i);
				encap.write_string (name, true);
				marshal_type_code (encap, tc->member_type (i), map, pos);
				Visibility vis = tc->member_visibility (i);
				encap.write_c (alignof (Visibility), sizeof (Visibility), &vis);
			}
		} break;

		case TCKind::tk_value_box:
			encap.write_id_name (tc);
			marshal_type_code (encap, tc->content_type (), map, pos);
			break;

		default:
			throw BAD_TYPECODE ();
	}
	stream.write_seq (encap.data (), true);
}

TypeCode::_ref_type RequestGIOP::unmarshal_type_code ()
{
	size_t start_pos = round_up (stream_in_->position (), (size_t)4);
	ULong kind = stream_in_->read32 ();
	if (INDIRECTION_TAG == kind) {
		Long off = stream_in_->read32 ();
		if (off >= -4)
			throw MARSHAL ();
		IndirectMapUnmarshal::iterator it = top_level_tc_unmarshal_.find (start_pos + 4 + off);
		if (it == top_level_tc_unmarshal_.end ())
			throw BAD_TYPECODE ();
		return TypeCode::_ptr_type (static_cast <TypeCode*> (it->second));
	}

	TypeCode::_ref_type ret;
	if (!TC_FactoryImpl::get_simple_tc ((TCKind)kind, ret)) {
		ret = TC_FactoryImpl::unmarshal_type_code ((TCKind)kind, *stream_in_,
			top_level_tc_unmarshal_.get_allocator ().heap ());
		top_level_tc_unmarshal_.emplace (start_pos, &TypeCode::_ptr_type (ret));
	}
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

	marshal_value (value_type2base (val), val, output);
}

void RequestGIOP::marshal_value (ValueBase::_ptr_type base, Interface::_ptr_type val, bool output)
{
	size_t pos = round_up (stream_out_->size (), (size_t)4);
	auto ins = value_map_marshal_.emplace (&base, pos);
	if (!ins.second) {
		stream_out_->write32 (INDIRECTION_TAG);
		Long off = Long (ins.first->second - (pos + 4));
		stream_out_->write32 (off);
		return;
	}
	Interface::_ptr_type primary = base->_query_valuetype (nullptr);
	if (!primary)
		throw UNKNOWN ();
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
				marshal_val_rep_id (std::move (id));
			}
		} else {
			// Single ID
			tag |= 2; // Single ID
			if (chunk_level_) {
				--chunk_level_;
				tag |= 8;
			}
			stream_out_->write32 (tag);
			marshal_val_rep_id (primary->_epv ().interface_id);
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

void RequestGIOP::marshal_val_rep_id (IDL::String&& id)
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
	size_t start_pos = round_up (stream_in_->position (), (size_t)4);
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
	std::vector <const IDL::String*> type_info;
	switch (value_tag & 0x00000006) {
		case 0: // No type information
			type_info.emplace_back (&interface_id);
			break;
		case 2: // Single repository id
			type_info.push_back (&unmarshal_val_rep_id ());
			break;
		case 6: { // Sequence of the repository IDs
			ULong count = stream_in_->read32 ();
			type_info.reserve (count);
			while (count--) {
				type_info.push_back (&unmarshal_val_rep_id ());
			}
		} break;

		default: // 4 is illegal value
			throw MARSHAL ();
	}

	ValueFactoryBase::_ref_type factory;
	bool truncate = false;
	for (const IDL::String* id : type_info) {
		try {
			factory = Binder::bind_interface <ValueFactoryBase> (*id);
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

const IDL::String& RequestGIOP::unmarshal_val_rep_id ()
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

Interface::_ref_type RequestGIOP::unmarshal_abstract (const IDL::String& interface_id)
{
	Octet is_object;
	stream_in_->read (1, 1, &is_object);
	if (is_object)
		return RequestGIOP::unmarshal_interface (interface_id);
	else
		return RequestGIOP::unmarshal_value (interface_id);
}

}
}
