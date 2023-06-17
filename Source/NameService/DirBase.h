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
#ifndef NIRVANA_FS_CORE_DIRBASE_H_
#define NIRVANA_FS_CORE_DIRBASE_H_
#pragma once

#include "ItemBase.h"
#include <CORBA/Server.h>
#include <Nirvana/FS_s.h>
#include "FileSystem.h"
#include "NamingContextRoot.h"

namespace Nirvana {
namespace FS {
namespace Core {

class DirBase : public CORBA::servant_traits <Nirvana::FS::Dir>::Servant <DirBase>,
	public ItemBase,
	public CosNaming::Core::NamingContextRoot
{
public:
	static FileType type () noexcept
	{
		return FileType::directory;
	}

	void bind (CosNaming::Name& n, CORBA::Object::_ptr_type obj, bool rebind = false);

	void rebind (CosNaming::Name& n, CORBA::Object::_ptr_type obj)
	{
		bind (n, obj, true);
	}

	virtual void bind_file (CosNaming::Name& n, const PortableServer::ObjectId& file,
		bool rebind) = 0;

	void bind_context (CosNaming::Name& n, CosNaming::NamingContext::_ptr_type nc, bool rebind = false);

	void rebind_context (CosNaming::Name& n, CosNaming::NamingContext::_ptr_type nc)
	{
		bind_context (n, nc, true);
	}

	virtual void bind_dir (CosNaming::Name& n, const PortableServer::ObjectId& dir,
		bool rebind) = 0;

	CORBA::Object::_ref_type resolve (CosNaming::Name& n)
	{
		return FileSystem::get_reference (resolve_path (n));
	}

	virtual PortableServer::ObjectId resolve_path (CosNaming::Name& n) = 0;

	virtual void unbind (CosNaming::Name& n) = 0;

	static CORBA::Internal::Interface* _s_new_context (CORBA::Internal::Bridge <CosNaming::NamingContext>*,
		CORBA::Internal::Interface* _env)
	{
		CORBA::Internal::set_BAD_OPERATION (_env);
	}

	CosNaming::NamingContext::_ref_type bind_new_context (CosNaming::Name& n)
	{
		return FileSystem::get_dir (create_dir (n));
	}

	virtual PortableServer::ObjectId create_dir (CosNaming::Name& n) = 0;

};

}
}
}

#endif
