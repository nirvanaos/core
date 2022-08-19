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
#include <CORBA/Policy_s.h>
#include "../Synchronized.h"

namespace CORBA {
namespace Core {

template <PolicyType type> class PolicyImpl
{
public:
	static Policy::_ref_type create_policy (const Any& a)
	{
		throw PolicyError (UNSUPPORTED_POLICY);
	}
};

template <class PolicyItf, PolicyType id, typename ValueType>
class PolicyImplBase : public servant_traits <PolicyItf>::template Servant <PolicyImpl <id> >
{
public:
	static PolicyType _s_get_policy_type (Internal::Bridge <Policy>*, Internal::Interface*)
	{
		return id;
	}

	typename Policy::_ref_type copy ()
	{
		return this->_this ();
	}

	static void _s_destroy (Internal::Bridge <Policy>* _b, Internal::Interface* _env)
	{}

	static typename PolicyItf::_ref_type create (const ValueType& val)
	{
		SYNC_BEGIN (Nirvana::Core::g_core_free_sync_context, nullptr)
		return make_stateless <PolicyImpl <id> > (std::ref (val))->_this ();
		SYNC_END ()
	}

	static Policy::_ref_type create_policy (const Any& a)
	{
		ValueType val;
		if (a >>= val)
			return create (val);
		throw PolicyError (BAD_POLICY_TYPE);
	}
};

}
}

#define DEFINE_POLICY(id, Itf, ValType, att_name) template <>\
class PolicyImpl <id> : public PolicyImplBase <Itf, id, ValType> {\
public: typedef ValType ValueType;\
  PolicyImpl (const ValType& v) : value_ (v) {}\
  ValType att_name () const NIRVANA_NOEXCEPT { return value_; }\
private: ValType value_; }

#endif
