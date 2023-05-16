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
#include "POA_Activator.h"
#include "POA_Root.h"

using namespace CORBA;
using namespace CORBA::Internal;
using namespace CORBA::Core;

namespace PortableServer {
namespace Core {

Object::_ref_type POA_Activator::incarnate (Type <ObjectId>::C_in oid)
{
	Bridge <ServantActivator>* bridge = static_cast <Bridge <ServantActivator>*>
		(&ServantActivator::_ptr_type (activator_));
	EnvironmentEx <ForwardRequest> env;
	Type <Object>::C_ret ret ((bridge->_epv ().epv.incarnate) (bridge,
		&Type <ObjectId>::C_in (oid),
		&POA::_ptr_type (_this ()), &env));
	env.check ();
	return ret;
}

void POA_Activator::etherialize (Type <ObjectId>::C_in oid, Object::_ptr_type serv,
	bool cleanup_in_progress,
	bool remaining_activations)
{
	Bridge <ServantActivator>* bridge = static_cast <Bridge <ServantActivator>*>
		(&ServantActivator::_ptr_type (activator_));
	(activator_->_epv ().epv.etherealize) (bridge, &oid, &POA::_ptr_type (_this ()),
		&serv, cleanup_in_progress, remaining_activations,
		nullptr); // Ignore exceptions
}

void POA_Activator::serve_default (const RequestRef& request, ReferenceLocal& reference)
{
	if (!activator_)
		throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (2));
	
	const ObjectId& oid = reference.core_key ().object_id ();
	auto ins = activation_map_.emplace (oid, ACTIVATION_TIMEOUT);
	ActivationMap::reference entry = *ins.first;
	Object::_ref_type servant;
	if (ins.second) {
		try {
			try {
				servant = incarnate (oid);
			} catch (const ForwardRequest&) {
				// ForwardRequest behaviour is not supported for incoming request.
				// TODO: Log to let an user understand the restriction.
				throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (2));
			}

			if (!servant) // User code returned nil servant
				throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (2));

			try {
				activate_object (reference, *object2proxy (servant));
			} catch (const ServantAlreadyActive&) {
				etherialize (oid, servant, false, false);
				throw OBJ_ADAPTER ();
			}
			entry.second.finish_construction (servant);
			activation_map_.erase (entry.first);
		} catch (...) {
			entry.second.on_exception ();
			activation_map_.erase (entry.first);
			throw;
		}
	} else
		servant = entry.second.get ();

	serve_request (request, reference, *object2proxy (servant));
}

ServantManager::_ref_type POA_Activator::get_servant_manager ()
{
	return activator_;
}

void POA_Activator::set_servant_manager (ServantManager::_ptr_type imgr)
{
	if (activator_)
		throw BAD_INV_ORDER (MAKE_OMG_MINOR (6));
	activator_ = ServantActivator::_narrow (imgr);
	if (!activator_)
		throw OBJ_ADAPTER (MAKE_OMG_MINOR (4));
}

ReferenceLocalRef POA_Activator::create_reference (ObjectKey&& key,
	const RepositoryId& intf)
{
	return POA_Base::create_reference (std::move (key), intf, Reference::GARBAGE_COLLECTION);
}

void POA_Activator::etherialize (const ObjectId& oid, ServantProxyObject& proxy,
	bool cleanup_in_progress) NIRVANA_NOEXCEPT
{
	if (activator_)
		etherialize (oid, proxy.get_proxy (), cleanup_in_progress, proxy.is_active ());
}

}
}
