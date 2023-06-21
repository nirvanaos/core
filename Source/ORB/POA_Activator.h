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
#ifndef NIRVANA_ORB_CORE_POA_ACTIVATOR_H_
#define NIRVANA_ORB_CORE_POA_ACTIVATOR_H_
#pragma once

#include "POA_Retain.h"
#include "POA_DGC.h"
#include "HashOctetSeq.h"

namespace PortableServer {
namespace Core {

// RETAIN and USE_SERVANT_MANAGER
class POA_Activator :
	public virtual POA_Retain,
	public virtual POA_DGC
{
	static const TimeBase::TimeT ACTIVATION_TIMEOUT = 100 * TimeBase::MILLISECOND;

public:
	virtual ServantManager::_ref_type get_servant_manager () override;
	virtual void set_servant_manager (ServantManager::_ptr_type imgr) override;

protected:
	virtual CORBA::Core::ReferenceLocalRef create_reference (ObjectKey&& key,
		CORBA::Internal::String_in iid) override;

	virtual void serve_default (const RequestRef& request, CORBA::Core::ReferenceLocal& reference)
		override;

	virtual void etherialize (const ObjectId& oid, CORBA::Core::ServantProxyObject& proxy,
		bool cleanup_in_progress) noexcept override;

private:
	CORBA::Object::_ref_type incarnate (CORBA::Internal::Type <ObjectId>::C_in oid);

	void etherialize (CORBA::Internal::Type <ObjectId>::C_in oid, CORBA::Object::_ptr_type serv,
		bool cleanup_in_progress,
		bool remaining_activations);

private:
	ServantActivator::_ref_type activator_;

	typedef const CORBA::Core::ReferenceLocal* ActivationKey;
	typedef Nirvana::Core::WaitableRef <CORBA::Object::_ref_type> ActivationRef;

	typedef Nirvana::Core::MapUnorderedStable <ActivationKey, ActivationRef,
		std::hash <ActivationKey>, std::equal_to <ActivationKey>, Nirvana::Core::UserAllocator>
		ActivationMap;

	ActivationMap activation_map_;
};

}
}

#endif
