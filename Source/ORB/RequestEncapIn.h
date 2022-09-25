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
#ifndef NIRVANA_ORB_CORE_REQUESTENCAPIN_H_
#define NIRVANA_ORB_CORE_REQUESTENCAPIN_H_
#pragma once

#include "RequestEncap.h"
#include "StreamInEncap.h"
#include "TC_ObjRef.h"
#include "TC_Fixed.h"
#include "TC_Struct.h"
#include "TC_Union.h"

namespace CORBA {
namespace Core {

/// Implements IORequest for encapsulated data.
class NIRVANA_NOVTABLE RequestEncapIn : public RequestEncap
{
protected:
	RequestEncapIn (const RequestGIOP& parent, const IDL::Sequence <Octet>& data) NIRVANA_NOEXCEPT :
		RequestEncap (parent),
		stream_ (std::ref (data))
	{
		stream_in_ = &stream_;
	}

	virtual bool marshal_op () override
	{
		return false;
	}

private:
	Nirvana::Core::ImplStatic <StreamInEncap> stream_;
};

TypeCode::_ref_type RequestGIOP::unmarshal_type_code ()
{
	TCKind kind;
	stream_in_->read (alignof (TCKind), sizeof (TCKind), &kind);
	TypeCode::_ref_type ret;
	switch (kind) {
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

		case TCKind::tk_objref: {
			OctetSeq encap;
			stream_in_->read_seq (encap);
			Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (encap));
			IDL::String id, name;
			stm.read_string (id);
			stm.read_string (name);
			if (stm.end () != 0)
				throw CORBA::MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
			ret = make_pseudo <TC_ObjRef> (std::move (id), std::move (name));
		} break;

		case TCKind::tk_struct: {
			OctetSeq encap;
			stream_in_->read_seq (encap);
			Nirvana::Core::ImplStatic <RequestEncapIn> rq (std::ref (*this), std::ref (encap));
			IDL::String id, name;
			rq.stream_in ()->read_string (id);
			rq.stream_in ()->read_string (name);
			ULong cnt;
			rq.stream_in ()->read (alignof (ULong), sizeof (ULong), &cnt);
			TC_Struct::Members members;
			members.construct (cnt);
			TC_Struct::Member* pm = members.begin ();
			while (cnt--) {
				rq.stream_in ()->read_string (pm->name);
				pm->type = rq.unmarshal_type_code ();
				++pm;
			}
			ret = make_pseudo <TC_Struct> (std::move (id), std::move (name), std::move (members));
		} break;

		case TCKind::tk_union: {
			OctetSeq encap;
			stream_in_->read_seq (encap);
			Nirvana::Core::ImplStatic <RequestEncapIn> rq (std::ref (*this), std::ref (encap));
			IDL::String id, name;
			rq.stream_in ()->read_string (id);
			rq.stream_in ()->read_string (name);
			TypeCode::_ref_type discriminator_type = rq.unmarshal_type_code ();
			Long default_index;
			rq.stream_in ()->read (alignof (Long), sizeof (Long), &default_index);
			ULong cnt;
			rq.stream_in ()->read (alignof (ULong), sizeof (ULong), &cnt);
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
				pm->type = rq.unmarshal_type_code ();
			}
			ret = make_pseudo <TC_Union> (id, name, std::move (discriminator_type), default_index,
				std::move (members));
		} break;

		default:
			throw BAD_TYPECODE ();
	}
	return ret;
}

}
}

#endif
