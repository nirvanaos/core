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
#ifndef NIRVANA_ORB_CORE_REFERENCE_H_
#define NIRVANA_ORB_CORE_REFERENCE_H_
#pragma once

#include "ProxyManager.h"
#include "RefCntProxy.h"
#include "GarbageCollector.h"
#include "PolicyMap.h"

namespace CORBA {
namespace Core {

class StreamOut;

class NIRVANA_NOVTABLE Reference :
	public ProxyManager,
	public SyncGC
{
	template <class> friend class CORBA::servant_reference;
	virtual void _add_ref () noexcept override = 0;
	virtual void _remove_ref () noexcept override = 0;

public:
	/// Reference creation deadline.
	/// The reference creation can cause binding and loading modules.
	static const TimeBase::TimeT DEADLINE_MAX = 10 * TimeBase::MILLISECOND;

	/// Reference flag bits
	enum
	{
		GARBAGE_COLLECTION = 0x0001, ///< The reference is involved in DGC
		PERSISTENT         = 0x0002, ///< LifespanPolicy is PERSISTENT
		LOCAL              = 0x8000  ///< Is ReferenceLocal
	};

	virtual Reference* to_reference () noexcept override
	{
		return this;
	}

	unsigned flags () const noexcept
	{
		return flags_;
	}

	RefCntProxy::IntegralType _refcount_value () const noexcept
	{
		return ref_cnt_.load ();
	}

	virtual Policy::_ref_type _get_policy (PolicyType policy_type) override;

	virtual IDL::String _repository_id () override;
	virtual Object::_ref_type _get_component () override;

protected:
	Reference (Internal::String_in primary_iid, unsigned flags) :
		ProxyManager (primary_iid, false),
		ref_cnt_ (1),
		flags_ (flags)
	{}

	Reference (const ProxyManager& proxy, unsigned flags) :
		ProxyManager (proxy),
		ref_cnt_ (1),
		flags_ (flags)
	{}

	~Reference ()
	{}

	bool is_local_object_op (Internal::OperationIndex op) const noexcept;

protected:
	RefCntProxy ref_cnt_;
	unsigned flags_;
	servant_reference <PolicyMapShared> policies_;
};

template <template <class> class Allocator>
using ReferenceSet = Nirvana::Core::SetUnorderedUnstable <ReferenceRef, std::hash <void*>,
	std::equal_to <void*>, Allocator>;

}
}

#endif
