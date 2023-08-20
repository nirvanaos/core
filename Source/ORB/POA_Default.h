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
#ifndef NIRVANA_ORB_CORE_POA_DEFAULT_H_
#define NIRVANA_ORB_CORE_POA_DEFAULT_H_
#pragma once

#include "POA_Base.h"

namespace PortableServer {
namespace Core {

// USE_DEFAULT_SERVANT
class POA_Default : public virtual POA_Base
{
public:
	virtual CORBA::Object::_ref_type get_servant () override;
	virtual void set_servant (CORBA::Object::_ptr_type p_servant) override;

protected:
	virtual CORBA::Object::_ref_type reference_to_servant_default (bool not_active) override;
	virtual CORBA::Object::_ref_type id_to_servant_default (bool not_active) override;
	virtual void serve_default (Request& request) override;

private:
	CORBA::Object::_ref_type servant_;
};

}
}

#endif
