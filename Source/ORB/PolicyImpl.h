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
#ifndef NIRVANA_ORB_CORE_POLICY_H_
#define NIRVANA_ORB_CORE_POLICY_H_

#include <CORBA/Server.h>
#include <CORBA/Policy_s.h>

namespace CORBA {
namespace Core {

template <class PolicyItf, PolicyType type, typename ValueType>
class PolicyImpl : public servant_traits <PolicyItf>::Servant <PolicyImpl <PolicyItf, type, ValueType> >
{
public:
	static PolicyType _s_get_policy_type (Internal::Bridge <Policy>*, Internal::Interface*)
	{
		return type;
	}

	typename PolicyItf::_ref_type copy ()
	{
		return this->_this ();
	}

	static void _s_destroy (Internal::Bridge <Policy>* _b, Internal::Interface* _env)
	{}

	ValueType value () const
	{
		return value_;
	}

	static typename PolicyItf::_ref_type create (ValueType val)
	{
		return make_stateless <PolicyImpl> (val)->_this ();
	}

	static typename PolicyItf::_ref_type create (const Any& a)
	{
		ValueType val;
		if (a >>= val)
			return create (val);
		throw BAD_PARAM;
	}

	PolicyImpl (ValueType val) :
		value_ (val)
	{}

private:
	ValueType value_;
};

}
}

#endif
