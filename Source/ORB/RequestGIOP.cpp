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
#include "TC_FactoryImpl.h"
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

Internal::Interface::_ref_type RequestGIOP::unmarshal_interface (const IDL::String& interface_id)
{
	Object::_ref_type obj;
	{
		IDL::String primary_iid;
		stream_in_->read_string (primary_iid);
		if (primary_iid.empty ())
			return nullptr; // nil reference

		IOP::TaggedProfileSeq addr;
		stream_in_->read_tagged (addr);

		IIOP::ListenPoint listen_point;
		IOP::ObjectKey object_key;
		bool object_key_found = false;
		IOP::TaggedComponentSeq components;
		for (const IOP::TaggedProfile& profile : addr) {
			switch (profile.tag ()) {
				case IOP::TAG_INTERNET_IOP: {
					Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (profile.profile_data ()));
					IIOP::Version ver;
					stm.read (alignof (IIOP::Version), sizeof (IIOP::Version), &ver);
					if (ver.major () != 1)
						throw NO_IMPLEMENT (MAKE_OMG_MINOR (3));
					stm.read_string (listen_point.host ());
					CORBA::UShort port;
					stm.read (alignof (CORBA::UShort), sizeof (CORBA::UShort), &port);
					if (stm.other_endian ())
						Nirvana::byteswap (port);
					listen_point.port (port);
					stm.read_seq (object_key);
					object_key_found = true;
					if (ver.minor () >= 1) {
						if (components.empty ())
							stm.read_tagged (components);
						else {
							IOP::TaggedComponentSeq comp;
							stm.read_tagged (comp);
							components.insert (components.end (), comp.begin (), comp.end ());
						}
					}
				} break;

				case IOP::TAG_MULTIPLE_COMPONENTS: {
					Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (profile.profile_data ()));
					if (components.empty ())
						stm.read_tagged (components);
					else {
						IOP::TaggedComponentSeq comp;
						stm.read_tagged (comp);
						components.insert (components.end (), comp.begin (), comp.end ());
					}
				} break;
			}
		}

		if (!object_key_found)
			throw IMP_LIMIT (MAKE_OMG_MINOR (1)); // Unable to use any profile in IOR.

		bool nirvana = false;
		for (const IOP::TaggedComponent& comp : components) {
			if (comp.tag () == IOP::TAG_ORB_TYPE) {
				if (comp.component_data ().size () == sizeof (ULong)) {
					ULong type = *(const ULong*)comp.component_data ().data ();
					if (stream_in_->other_endian ())
						Nirvana::byteswap (type);
					if (ESIOP::ORB_TYPE == type)
						nirvana = true;
				} else
					throw BAD_PARAM ();
				break;
			}
		}

		ESIOP::ProtDomainId domain_id;
		Octet flags = 0;
		bool domain_found = false;
		if (nirvana) {
			for (const IOP::TaggedComponent& comp : components) {
				switch (comp.tag ()) {
					case ESIOP::TAG_DOMAIN_ADDRESS:
						if (comp.component_data ().size () == sizeof (ESIOP::ProtDomainId)) {
							domain_id = *(const ESIOP::ProtDomainId*)comp.component_data ().data ();
							if (stream_in_->other_endian ())
								Nirvana::byteswap (domain_id);
							domain_found = true;
						} else
							throw INV_OBJREF ();
						break;

					case ESIOP::TAG_FLAGS:
						flags = *(const Octet*)comp.component_data ().data ();
						break;
				}
			}
		}

		size_t host_len = listen_point.host ().size ();
		const Char* host = listen_point.host ().data ();
		if (0 == host_len ||
			(LocalAddress::singleton ().host ().size () == host_len &&
				std::equal (host, host + host_len, LocalAddress::singleton ().host ().data ()))) {

			// Local system
			if (!nirvana)
				throw INV_OBJREF ();

			if (host_len && listen_point.port () != LocalAddress::singleton ().port ()) {
#ifdef SINGLE_DOMAIN
				throw INV_OBJREF ();
#else
				if (!domain_found)
					throw INV_OBJREF ();
				if (ESIOP::current_domain_id () != domain_id)
					obj = RemoteReferences::singleton ().unmarshal (primary_iid, std::move (addr), flags, domain_id);
				else
#endif
					obj = PortableServer::Core::POA_Root::unmarshal (primary_iid, object_key);
			}
		} else
			obj = RemoteReferences::singleton ().unmarshal (primary_iid, std::move (addr), flags, std::move (listen_point));
	}
	if (Internal::RepId::compatible (Internal::RepIdOf <Object>::id, interface_id))
		return obj;
	else
		return obj->_query_interface (interface_id);
}

void RequestGIOP::marshal_type_code (TypeCode::_ptr_type tc, IndirectMapMarshal& map, size_t parent_offset)
{
	if (!tc) {
		stream_out_->write32 (0);
		return;
	}

	ULong kind;
	try {
		kind = (ULong)tc->kind ();
	} catch (...) {
		throw BAD_TYPECODE (MAKE_OMG_MINOR (1)); // Attempt to marshal incomplete TypeCode.
	}

	static const TCKind simple_types [] = {
		TCKind::tk_short,
		TCKind::tk_long,
		TCKind::tk_ushort,
		TCKind::tk_ulong,
		TCKind::tk_float,
		TCKind::tk_double,
		TCKind::tk_boolean,
		TCKind::tk_char,
		TCKind::tk_octet,
		TCKind::tk_any,
		TCKind::tk_TypeCode,
		TCKind::tk_longlong,
		TCKind::tk_ulonglong,
		TCKind::tk_longdouble,
		TCKind::tk_wchar
	};

	const TCKind* f = std::lower_bound (simple_types, std::end (simple_types), (TCKind)kind);
	if (f != std::end (simple_types) && *f == (TCKind)kind) {
		stream_out_->write32 (kind);
		return;
	}

	switch ((TCKind)kind) {
		case TCKind::tk_string:
		case TCKind::tk_wstring:
			stream_out_->write32 (kind);
			stream_out_->write32 (tc->length ());
			return;

		case TCKind::tk_fixed: {
			stream_out_->write32 (kind);

			struct
			{
				UShort d;
				Short s;
			} ds { tc->fixed_digits (), tc->fixed_scale () };
			stream_out_->write_c (2, 4, &ds);
		} return;
	}

	size_t off = stream_out_->size () + parent_offset - 4;
	auto ins = map.emplace (&tc, off);
	if (!ins.second) {
		stream_out_->write32 (INDIRECTION_TAG);
		Long offset = (Long)(ins.first->second - (off + 4));
		stream_out_->write32 (offset);
		return;
	}

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

TypeCode::_ref_type RequestGIOP::unmarshal_type_code ()
{
	size_t start_pos = stream_in_->position ();
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
	TypeCode::_ref_type tc = TC_FactoryImpl::unmarshal_type_code (kind, *stream_in_);
	top_level_tc_unmarshal_.emplace (start_pos, &TypeCode::_ptr_type (tc));
	return tc;
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
		Long off = Long (ins.first->second - (pos + 4));
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
