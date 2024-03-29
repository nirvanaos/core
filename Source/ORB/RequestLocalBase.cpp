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
#include "RequestLocalBase.h"

using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

void RequestLocalBase::marshal_value (Interface::_ptr_type val)
{
	if (marshal_op ()) {
		if (!val)
			write8 (0);
		else
			marshal_value_internal (value_type2base (val));
	}
}

void RequestLocalBase::marshal_value_internal (ValueBase::_ptr_type base)
{
	assert (base);

	auto ins = value_map_.marshal_map ().emplace (&base, (uintptr_t)cur_ptr_);
	if (!ins.second) {
		// This value was already marshalled, write indirection tag.
		write8 (2);
		write (alignof (uintptr_t), sizeof (uintptr_t), &ins.first->second);
	} else {
		auto factory = base->_factory ();
		if (!factory)
			throw MARSHAL (MAKE_OMG_MINOR (1)); // Unable to locate value factory.
		write8 (1);
		marshal_interface_internal (factory);
		base->_marshal (_get_ptr ());
	}
}

void RequestLocalBase::unmarshal_value (const IDL::String& interface_id, Interface::_ref_type& val)
{
	Internal::Interface::_ref_type ret;
	uintptr_t pos = (uintptr_t)cur_ptr_;
	unsigned tag = read8 ();
	switch (tag) {
	case 0:
		break;

	case 1: {
		ValueFactoryBase::_ref_type factory;
		unmarshal_interface (Internal::RepIdOf <ValueFactoryBase>::id, reinterpret_cast <Interface::_ref_type&> (factory));
		if (!factory)
			throw MARSHAL (MAKE_OMG_MINOR (1)); // Unable to locate value factory.
		ValueBase::_ref_type base (factory->create_for_unmarshal ());
		base->_unmarshal (_get_ptr ());
		value_map_.unmarshal_map ().emplace (pos, &ValueBase::_ptr_type (base));
		val = base->_query_valuetype (interface_id);
	} break;

	case 2: {
		uintptr_t p;
		read (alignof (uintptr_t), sizeof (uintptr_t), &p);
		const auto& unmarshal_map = value_map_.unmarshal_map ();
		Interface* vb = unmarshal_map.find (p);
		if (!vb)
			throw MARSHAL (); // Unexpected
		val = static_cast <ValueBase*> (vb)->_query_valuetype (interface_id);
	} break;

	default:
		throw MARSHAL (); // Unexpected
	}

	if (tag && !val)
		throw MARSHAL (); // Unexpected
}

void RequestLocalBase::marshal_abstract (Interface::_ptr_type itf)
{
	if (marshal_op ()) {
		if (!itf) {
			write8 (0);
			write8 (0);
		} else {
			// Downcast to AbstractBase
			AbstractBase::_ptr_type base = abstract_interface2base (itf);
			if (base->_to_object ()) {
				write8 (1);
				marshal_interface_internal (itf);
			} else {
				ValueBase::_ref_type value = base->_to_value ();
				if (!value)
					Nirvana::throw_MARSHAL (); // Unexpected
				write8 (0);
				marshal_value_internal (value);
			}
		}
	}
}

void RequestLocalBase::unmarshal_abstract (const IDL::String& interface_id, Interface::_ref_type& itf)
{
	unsigned tag = read8 ();
	if (tag)
		unmarshal_interface (interface_id, itf);
	else
		unmarshal_value (interface_id, itf);
}

}
}
