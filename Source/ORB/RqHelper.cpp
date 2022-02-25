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
#include "RqHelper.h"

namespace CORBA {
namespace Internal {
namespace Core {

Object::_ptr_type RqHelper::interface2object (Interface::_ptr_type itf)
{
	Object::_ptr_type obj;
	const StringBase <Char> interface_id = itf->_epv ().interface_id;
	if (RepId::compatible (interface_id, Object::repository_id_))
		obj = (Object*)&itf;
	else {
		Environment env;
		Interface* p = ((const EPV_Header&)itf->_epv ()).base.to_base (&itf, ValueBase::repository_id_, &env);
		env.check ();
		obj = Object::_check (p);
	}
	return obj;
}

ValueBase::_ptr_type RqHelper::value_type2base (Interface::_ptr_type val)
{
	ValueBase::_ptr_type base;
	const StringBase <Char> interface_id = val->_epv ().interface_id;
	if (RepId::compatible (interface_id, ValueBase::repository_id_))
		base = (ValueBase*)&val;
	else {
		Environment env;
		Interface* p = ((const EPV_Header&)val->_epv ()).base.to_base (&val, ValueBase::repository_id_, &env);
		env.check ();
		base = ValueBase::_check (p);
	}
	return base;
}

AbstractBase::_ptr_type RqHelper::abstract_interface2base (Interface::_ptr_type itf)
{
	AbstractBase::_ptr_type base;
	Environment env;
	Interface* p = ((const EPV_Header&)itf->_epv ()).base.to_base (&itf, AbstractBase::repository_id_, &env);
	env.check ();
	base = AbstractBase::_check (p);
	return base;
}

}
}
}
