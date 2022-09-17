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
#include <signal.h>

namespace CORBA {

using namespace Internal;

namespace Core {

Object::_ptr_type RqHelper::interface2object (Interface::_ptr_type itf)
{
	Object::_ptr_type obj;
	const CORBA::Internal::StringView <Char> interface_id = itf->_epv ().interface_id;
	if (RepId::compatible (interface_id, RepIdOf <Object>::id))
		obj = (Object*)&itf;
	else {
		Internal::Environment env;
		Interface* p = ((const EPV_Header&)itf->_epv ()).base.to_base (&itf, &Type <IDL::String>::C_in (RepIdOf <ValueBase>::id), &env);
		env.check ();
		obj = Object::_check (p);
	}
	return obj;
}

ValueBase::_ptr_type RqHelper::value_type2base (Interface::_ptr_type val)
{
	ValueBase::_ptr_type base;
	const CORBA::Internal::StringView <Char> interface_id = val->_epv ().interface_id;
	if (RepId::compatible (interface_id, RepIdOf <ValueBase>::id))
		base = (ValueBase*)&val;
	else {
		Internal::Environment env;
		Interface* p = ((const EPV_Header&)val->_epv ()).base.to_base (&val, &Type <IDL::String>::C_in (RepIdOf <ValueBase>::id), &env);
		env.check ();
		base = ValueBase::_check (p);
	}
	return base;
}

AbstractBase::_ptr_type RqHelper::abstract_interface2base (Interface::_ptr_type itf)
{
	AbstractBase::_ptr_type base;
	Internal::Environment env;
	Interface* p = ((const EPV_Header&)itf->_epv ()).base.to_base (&itf, &Type <IDL::String>::C_in (RepIdOf <AbstractBase>::id), &env);
	env.check ();
	base = AbstractBase::_check (p);
	return base;
}

void RqHelper::check_align (size_t align)
{
	if (!(align == 1 || align == 2 || align == 4 || align == 8))
		throw BAD_PARAM ();
}

Any RqHelper::signal2exception (const siginfo& signal) NIRVANA_NOEXCEPT
{
	Exception::Code ec ;
	if (signal.si_excode != Exception::EC_NO_EXCEPTION)
		ec = (Exception::Code)signal.si_excode;
	else
		ec = SystemException::EC_UNKNOWN;
	const ExceptionEntry* ee = SystemException::_get_exception_entry (ec);
	std::aligned_storage <sizeof (SystemException), alignof (SystemException)>::type buf;
	(ee->construct) (&buf);
	Any a;
	a <<= (const Exception&)buf;
	return a;
}

Any RqHelper::exception2any (Exception&& e)
{
	Any a;
	a <<= std::move (e);
	return a;
}

}
}
