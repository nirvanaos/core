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
#include "POA_Locator.h"
#include "POA_Root.h"

using namespace CORBA;
using namespace CORBA::Internal;
using namespace CORBA::Core;

namespace PortableServer {
namespace Core {

ServantManager::_ref_type POA_Locator::get_servant_manager ()
{
	check_exist ();

	return locator_;
}

void POA_Locator::set_servant_manager (ServantManager::_ptr_type imgr)
{
	check_exist ();

	if (locator_)
		throw BAD_INV_ORDER (MAKE_OMG_MINOR (6));
	locator_ = ServantLocator::_narrow (imgr);
	if (!locator_)
		throw OBJ_ADAPTER (MAKE_OMG_MINOR (4));
}

inline
Object::_ref_type POA_Locator::preinvoke (Type <ObjectId>::C_in oid, String_in operation,
	void*& the_cookie)
{
	Bridge <ServantLocator>* bridge = static_cast <Bridge <ServantLocator>*>
		(&ServantLocator::_ptr_type (locator_));
	EnvironmentEx <ForwardRequest> env;
	Type <Object>::C_ret ret ((bridge->_epv ().epv.preinvoke) (bridge,
		&oid,
		&POA::_ptr_type (_this ()),
		&operation,
		&the_cookie,
		&env));
	env.check ();
	return ret;
}

void POA_Locator::postinvoke (Type <ObjectId>::C_in oid, String_in operation,
	void* the_cookie, Object::_ptr_type the_servant) noexcept
{
	Bridge <ServantLocator>* bridge = static_cast <Bridge <ServantLocator>*>
		(&ServantLocator::_ptr_type (locator_));
	(bridge->_epv ().epv.postinvoke) (bridge,
		&oid,
		&POA::_ptr_type (_this ()),
		&operation,
		the_cookie,
		&the_servant,
		nullptr); // Ignore exception
}

void POA_Locator::serve_default (Request& request)
{
	if (!locator_)
		throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (2));

	const ObjectId oid = ObjectKey::get_object_id (request.object_key ());
	CORBA::Internal::StringView <Char> op = request.operation ();
	void* cookie;
	Object::_ref_type servant;
	try {
		servant = preinvoke (oid, op, cookie);
	} catch (const ForwardRequest&) {
		// ForwardRequest behaviour is not supported for incoming request.
		// TODO: Log to let an user understand the restriction.
		throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (2));
	}

	try {
		if (!servant) // User code returned nil servant
			throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (2));

		ReferenceLocalRef reference = root ().find_reference (request.object_key ());
		serve_request (request, oid, reference, *object2proxy (servant));
	} catch (...) {
		postinvoke (oid, op, cookie, servant);
		throw;
	}
	postinvoke (oid, op, cookie, servant);
}

}
}
