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
#ifndef NIRVANA_ORB_CORE_REQUESTENCAPOUT_H_
#define NIRVANA_ORB_CORE_REQUESTENCAPOUT_H_
#pragma once

#include "RequestEncap.h"
#include "StreamOutEncap.h"

namespace CORBA {
namespace Core {

/// Implements IORequest for encapsulated data.
class NIRVANA_NOVTABLE RequestEncapOut : public RequestEncap
{
public:
	OctetSeq& data () NIRVANA_NOEXCEPT
	{
		return stream_.data ();
	}

protected:
	RequestEncapOut (const RequestGIOP& parent) NIRVANA_NOEXCEPT :
		RequestEncap (parent)
	{
		stream_out_ = &stream_;
	}

	virtual bool marshal_op () override
	{
		return true;
	}

private:
	Nirvana::Core::ImplStatic <StreamOutEncap> stream_;
};

inline
void RequestGIOP::marshal_type_code (TypeCode::_ptr_type tc)
{
	if (!marshal_op ())
		return;

	TCKind kind = tc->kind ();
	stream_out_->write_c (alignof (TCKind), sizeof (TCKind), &kind);
	switch (kind) {
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
			Nirvana::Core::ImplStatic <RequestEncapOut> encap (std::ref (*this));
			encap.stream_out ()->write_id_name (tc);
			ULong cnt = tc->member_count ();
			encap.stream_out ()->write_c (alignof (ULong), sizeof (ULong), &cnt);
			for (ULong i = 0; i < cnt; ++i) {
				IDL::String name = tc->member_name (i);
				encap.stream_out ()->write_string (name, true);
				encap.marshal_type_code (tc->member_type (i));
			}
			stream_out_->write_seq (encap.data (), true);
		} break;

		case TCKind::tk_union: {
			Nirvana::Core::ImplStatic <RequestEncapOut> encap (std::ref (*this));
			encap.stream_out ()->write_id_name (tc);
			encap.marshal_type_code (tc->discriminator_type ());
			long def = tc->default_index ();
			encap.stream_out ()->write_c (alignof (long), sizeof (long), &def);
			ULong cnt = tc->member_count ();
			encap.stream_out ()->write_c (alignof (ULong), sizeof (ULong), &cnt);
			for (ULong i = 0; i < cnt; ++i) {
				Any label = tc->member_label (i);
				Internal::Type <Any>::marshal_out (label, _get_ptr ());
				IDL::String name = tc->member_name (i);
				encap.stream_out ()->write_string (name, true);
				encap.marshal_type_code (tc->member_type (i));
			}
			stream_out_->write_seq (encap.data (), true);
		} break;

		case TCKind::tk_enum: {
			Nirvana::Core::ImplStatic <StreamOutEncap> encap;
			encap.write_id_name (tc);
			ULong cnt = tc->member_count ();
			encap.write_c (alignof (ULong), sizeof (ULong), &cnt);
			for (ULong i = 0; i < cnt; ++i) {
				IDL::String name = tc->member_name (i);
				encap.write_string (name, true);
			}
			stream_out_->write_seq (encap.data (), true);
		} break;

		case TCKind::tk_string:
		case TCKind::tk_wstring: {
			ULong len = tc->length ();
			stream_out_->write_c (alignof (ULong), sizeof (ULong), &len);
		} break;

		case TCKind::tk_sequence:
		case TCKind::tk_array: {
			Nirvana::Core::ImplStatic <RequestEncapOut> encap (std::ref (*this));
			encap.marshal_type_code (tc->content_type ());
			ULong len = tc->length ();
			encap.stream_out ()->write_c (alignof (ULong), sizeof (ULong), &len);
			stream_out_->write_seq (encap.data (), true);
		} break;

		case TCKind::tk_alias: {
			Nirvana::Core::ImplStatic <RequestEncapOut> encap (std::ref (*this));
			encap.stream_out ()->write_id_name (tc);
			encap.marshal_type_code (tc->content_type ());
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
			Nirvana::Core::ImplStatic <RequestEncapOut> encap (std::ref (*this));
			encap.stream_out ()->write_id_name (tc);
			ValueModifier mod = tc->type_modifier ();
			encap.stream_out ()->write_c (alignof (ValueModifier), sizeof (ValueModifier), &mod);
			encap.marshal_type_code (tc->concrete_base_type ());
			ULong cnt = tc->member_count ();
			encap.stream_out ()->write_c (alignof (ULong), sizeof (ULong), &cnt);
			for (ULong i = 0; i < cnt; ++i) {
				IDL::String name = tc->member_name (i);
				encap.stream_out ()->write_string (name, true);
				encap.marshal_type_code (tc->member_type (i));
				Visibility vis = tc->member_visibility (i);
				encap.stream_out ()->write_c (alignof (Visibility), sizeof (Visibility), &vis);
			}
			stream_out_->write_seq (encap.data (), true);
		} break;

		case TCKind::tk_value_box: {
			Nirvana::Core::ImplStatic <RequestEncapOut> encap (std::ref (*this));
			encap.stream_out ()->write_id_name (tc);
			encap.marshal_type_code (tc->content_type ());
			stream_out_->write_seq (encap.data (), true);
		} break;

		 {

		} break;
	}
}

}
}

#endif
