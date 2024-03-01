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
#ifndef NIRVANA_ORB_CORE_POA_LOCATOR_H_
#define NIRVANA_ORB_CORE_POA_LOCATOR_H_
#pragma once

#include "POA_Base.h"

namespace PortableServer {
namespace Core {

// NON_RETAIN, USE_SERVANT_MANAGER
class POA_Locator : public virtual POA_Base
{
public:
	virtual ServantManager::_ref_type get_servant_manager () override;
	virtual void set_servant_manager (ServantManager::_ptr_type imgr) override;

protected:
	virtual void serve_default (Request& request, const ObjectId& oid,
		CORBA::Core::ReferenceLocal* reference) override;

	virtual void deactivate_objects (bool etherealize) noexcept override;

private:
	inline
	CORBA::Object::_ref_type preinvoke (IDL::Type <ObjectId>::C_in oid,
		CORBA::Internal::String_in operation, void*& the_cookie);

	void postinvoke (IDL::Type <ObjectId>::C_in oid,
		CORBA::Internal::String_in operation, void* the_cookie,
		CORBA::Object::_ptr_type the_servant) noexcept;

private:
	ServantLocator::_ref_type locator_;
};

}
}

#endif
