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
#ifndef NIRVANA_ORB_CORE_POLICYIMPL_H_
#define NIRVANA_ORB_CORE_POLICYIMPL_H_
#pragma once

#include <CORBA/Server.h>
#include <CORBA/NoDefaultPOA.h>
#include <CORBA/Policy_s.h>
#include "PolicyFactory.h"

namespace CORBA {
namespace Core {

template <PolicyType type> class PolicyImpl : public PolicyUnsupported
{};

template <class PolicyItf, PolicyType id, typename ValueType>
class PolicyImplBase :
	public servant_traits <PolicyItf>::template Servant <PolicyImpl <id> >,
	public PortableServer::NoDefaultPOA
{
public:
	// Disable implicit activation
	using PortableServer::NoDefaultPOA::__default_POA;

	static PolicyType _s_get_policy_type (Internal::Bridge <CORBA::Policy>*, Internal::Interface*)
	{
		return id;
	}

	typename Policy::_ref_type copy ()
	{
		return this->_this ();
	}

	static void _s_destroy (Internal::Bridge <Policy>* _b, Internal::Interface* _env)
	{}

	static typename PolicyItf::_ref_type create (const ValueType& val);

	static Policy::_ref_type create_policy (const Any& a)
	{
		ValueType val;
		if (a >>= val)
			return create (val);
		throw PolicyError (BAD_POLICY_TYPE);
	}

	static Policy::_ref_type read (StreamIn& s)
	{
		ValueType val;
		read_value (s, val);
		return create (val);
	}

	static void read_value (StreamIn& s, ValueType& val);

	static void write_value (const ValueType& val, StreamOut& s)
	{
		s.write_c (alignof (ValueType), sizeof (ValueType), &val);
	}

	static PolicyFactory::Functions functions_;

};

template <class PolicyItf, PolicyType id, typename ValueType>
typename PolicyItf::_ref_type PolicyImplBase <PolicyItf, id, ValueType>::create (const ValueType& val)
{
	return make_stateless <PolicyImpl <id> > (std::ref (val))->_this ();
}

template <class PolicyItf, PolicyType id, typename ValueType>
void PolicyImplBase <PolicyItf, id, ValueType>::read_value (StreamIn& s, ValueType& val)
{
	// Assume all policy data is CDR
	s.read (alignof (ValueType), sizeof (ValueType), &val);
	if (s.other_endian ())
		Internal::Type <ValueType>::byteswap (val);
}

template <class PolicyItf, PolicyType id, typename ValueType>
PolicyFactory::Functions PolicyImplBase <PolicyItf, id, ValueType>::functions_ = { create_policy, read, PolicyImpl <id>::write };

}
}

#define DEFINE_POLICY(id, Itf, ValType, att_name) template <>\
class PolicyImpl <id> : public PolicyImplBase <Itf, id, ValType> {\
public: typedef ValType ValueType;\
	PolicyImpl (const ValType& v) : value_ (v) {}\
	ValType att_name () const NIRVANA_NOEXCEPT { return value_; }\
	static void write (Policy::_ptr_type policy, StreamOut& s) { write_value (Itf::_narrow (policy)->att_name (), s); }\
private: ValType value_; }

#endif
