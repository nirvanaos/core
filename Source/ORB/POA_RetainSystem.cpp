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
#include "POA_RetainSystem.h"
#include "RequestInPOA.h"
#include "Reference.h"

using namespace CORBA;
using namespace CORBA::Core;
using namespace Nirvana;
using namespace Nirvana::Core;

namespace PortableServer {
namespace Core {

void POA_RetainSystem::destroy (bool etherealize_objects, bool wait_for_completion)
{
	Base::destroy (etherealize_objects, wait_for_completion);
	active_object_map_.clear ();
}

CoreRef <ProxyObject> POA_RetainSystem::get_proxy (Object::_ptr_type p_servant)
{
	CoreRef <ProxyObject> proxy = object2proxy (p_servant);
	if (!proxy)
		throw BAD_PARAM ();

	assert (proxy->servant ()); // p_servant reference was obtained from the real servant.
	if (proxy->object_key ())
		proxy = CoreRef <ProxyObject>::create <ProxyObjectImpl <ProxyObject> > (std::ref (*proxy));

	return proxy;
}

POA_RetainSystem::AOM::iterator POA_RetainSystem::activate (const ObjectIdSys& oid, CoreRef <ProxyObject>&& proxy)
{
	auto ins = active_object_map_.emplace (oid, std::move (proxy));
	assert (ins.second);
	try {
		ObjectKey object_key;
		get_path (object_key.adapter_path ());
		object_key.object_id (ins.first->first.to_object_id ());
		ins.first->second->activate (ObjectKeyRef::create <ObjectKeyImpl> (std::move (object_key)));
	} catch (...) {
		active_object_map_.erase (ins.first);
		throw;
	}
	return ins.first;
}

ObjectId POA_RetainSystem::activate_object (Object::_ptr_type p_servant)
{
	CoreRef <ProxyObject> proxy = get_proxy (p_servant);
	ObjectIdSys id (*proxy);
	AOM::iterator it = activate (id, std::move (proxy));
	try {
		return it->first.to_object_id ();
	} catch (...) {
		active_object_map_.erase (it);
		throw NO_MEMORY ();
	}
}

void POA_RetainSystem::activate_object_with_id (const ObjectId& oid, Object::_ptr_type p_servant)
{
	const ObjectIdSys* sys_id = ObjectIdSys::from_object_id (oid);
	if (!sys_id)
		throw BAD_PARAM ();
	activate (*sys_id, get_proxy (p_servant));
}

POA_RetainSystem::AOM_Val POA_RetainSystem::deactivate (const ObjectId& oid)
{
	const ObjectIdSys* sys_id = ObjectIdSys::from_object_id (oid);
	if (!sys_id)
		throw BAD_PARAM ();

	AOM::iterator it = active_object_map_.find (*sys_id);
	if (it == active_object_map_.end ())
		throw ObjectNotActive ();
	AOM_Val ret (std::move (it->second));
	active_object_map_.erase (it);
	return ret;
}

void POA_RetainSystem::deactivate_object (const ObjectId& oid)
{
	deactivate (oid);
}

CORBA::Object::_ref_type POA_RetainSystem::create_reference (const CORBA::RepositoryId& intf)
{
	CoreRef <Reference> ref = CoreRef <Reference>::create <ImplDynamic <Reference> > (std::ref (intf));
	ObjectKey object_key;
	get_path (object_key.adapter_path ());
	object_key.object_id (ObjectIdSys (*ref).to_object_id ());
	ref->object_key (ObjectKeyRef::create <ObjectKeyImpl> (std::move (object_key)));
	return ref->get_proxy ();
}

CORBA::Object::_ref_type POA_RetainSystem::create_reference_with_id (const ObjectId& oid, const CORBA::RepositoryId& intf)
{
	if (!ObjectIdSys::from_object_id (oid))
		throw BAD_PARAM ();
	CoreRef <Reference> ref = CoreRef <Reference>::create <ImplDynamic <Reference> > (std::ref (intf));
	ObjectKey object_key;
	get_path (object_key.adapter_path ());
	object_key.object_id (oid);
	ref->object_key (ObjectKeyRef::create <ObjectKeyImpl> (std::move (object_key)));
	return ref->get_proxy ();
}

Object::_ref_type POA_RetainSystem::id_to_reference (const ObjectId& oid)
{
	const ObjectIdSys* sys_id = ObjectIdSys::from_object_id (oid);
	if (!sys_id)
		throw BAD_PARAM ();
	AOM::iterator it = active_object_map_.find (*sys_id);
	if (it != active_object_map_.end ())
		return it->second->get_proxy ();
	throw ObjectNotActive ();
}

void POA_RetainSystem::serve (CORBA::Core::RequestInPOA& request, Nirvana::Core::MemContext* memory)
{
	const ObjectIdSys* sys_id = ObjectIdSys::from_object_id (request.object_key ().object_id ());
	if (!sys_id)
		throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (1));
	auto it = active_object_map_.find (*sys_id);
	if (it == active_object_map_.end ())
		throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (1));

	CoreRef <ProxyObject> proxy = it->second;
	POA_Base::serve (request, *proxy, memory);
}

void POA_RetainSystemUnique::destroy (bool etherealize_objects, bool wait_for_completion)
{
	Base::destroy (etherealize_objects, wait_for_completion);
	servant_map_.clear ();
}

POA_RetainSystemUnique::ServantMap::iterator POA_RetainSystemUnique::servant_add (
	Object::_ptr_type p_servant)
{
	auto ins = servant_map_.emplace (get_servant (p_servant), nullptr);
	if (!ins.second)
		throw ServantAlreadyActive ();
	return ins.first;
}

POA_RetainSystemUnique::ServantMap::const_iterator POA_RetainSystemUnique::servant_find (
	Object::_ptr_type p_servant)
{
	return servant_map_.find (get_servant (p_servant));
}

ObjectId POA_RetainSystemUnique::activate_object (Object::_ptr_type p_servant)
{
	ServantMap::iterator it_servant = servant_add (p_servant);
	CoreRef <ProxyObject> proxy = get_proxy (p_servant);
	ObjectIdSys id (*proxy);
	AOM::iterator it_object = activate (id, std::move (proxy));
	it_servant->second = &*it_object;
	try {
		return it_object->first.to_object_id ();
	} catch (...) {
		active_object_map_.erase (it_object);
		servant_map_.erase (it_servant);
		throw NO_MEMORY ();
	}
}

void POA_RetainSystemUnique::activate_object_with_id (const ObjectId& oid, Object::_ptr_type p_servant)
{
	const ObjectIdSys* sys_id = ObjectIdSys::from_object_id (oid);
	if (!sys_id)
		throw BAD_PARAM ();

	ServantMap::iterator it_servant = servant_add (p_servant);
	AOM::iterator it_object = activate (*sys_id, get_proxy (p_servant));
	it_servant->second = &*it_object;
}

void POA_RetainSystemUnique::deactivate_object (const ObjectId& oid)
{
	AOM_Val proxy = deactivate (oid);
	servant_map_.erase (static_cast <UserServantPtr> (&proxy->servant ()));
}

ObjectId POA_RetainSystemUnique::servant_to_id (Object::_ptr_type p_servant)
{
	auto it = servant_find (p_servant);
	if (it != servant_map_.end ())
		return it->second->first.to_object_id ();
	else
		throw ServantNotActive ();
}

Object::_ref_type POA_RetainSystemUnique::servant_to_reference (Object::_ptr_type p_servant)
{
	Object::_ref_type ret;
	auto it = servant_find (p_servant);
	if (it != servant_map_.end ())
		ret = it->second->second->get_proxy ();
	else if (!(ret = Base::servant_to_reference_nothrow (p_servant)))
		throw ServantNotActive ();
	return ret;
}

}
}
