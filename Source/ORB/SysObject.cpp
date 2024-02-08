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
#include "../pch.h"
#include "SysObject.h"
#include "ReferenceLocal.h"
#include "ObjectFactory.h"

namespace CORBA {
namespace Core {

inline
SysObject::SysObject (CORBA::LocalObject::_ptr_type servant, const Octet* id, size_t id_len) :
	LocalObjectImpl (servant, nullptr),
	id_ (id),
	id_len_ (id_len)
{}

SysObject* SysObject::create (CORBA::LocalObject::_ptr_type servant, const Octet* id, size_t id_len)
{
	ObjectFactory::StatelessCreationFrame* scf = ObjectFactory::begin_proxy_creation (&servant);
	SysObject* proxy = new SysObject (servant, id, id_len);
	if (scf)
		scf->proxy (&static_cast <ServantProxyBase&> (proxy->proxy ()));
	return proxy;
}

ReferenceRef SysObject::marshal (StreamOut& out)
{
	ReferenceLocal::marshal (*this, id_, id_len_, nullptr, out);
	return nullptr;
}

}
}
