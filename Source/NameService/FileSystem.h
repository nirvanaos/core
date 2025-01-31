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
#ifndef NIRVANA_FS_CORE_FILESYSTEM_H_
#define NIRVANA_FS_CORE_FILESYSTEM_H_
#pragma once

#include <Port/FileSystem.h>
#include "../MapUnorderedUnstable.h"
#include "../Synchronized.h"
#include "../ORB/Services.h"
#include "NamingContextBase.h"
#include <Nirvana/File_s.h>
#include <CORBA/NoDefaultPOA.h>
#include <Nirvana/posix_defs.h>

namespace PortableServer {
namespace Core {
class FileActivator;
}
}

namespace Nirvana {
namespace Core {

class FileSystem : 
	public CORBA::servant_traits <Nirvana::FileSystem>::Servant <FileSystem>,
	public PortableServer::NoDefaultPOA,
	public CosNaming::Core::NamingContextBase,
	private Port::FileSystem
{
public:
	using PortableServer::NoDefaultPOA::__default_POA;

	static const char adapter_name_ [];

	static PrimaryInterface::_ref_type create ()
	{
		PrimaryInterface::_ref_type ret;
		SYNC_BEGIN (g_core_free_sync_context, &Heap::user_heap ())

			// Create file system object adapter
			PortableServer::POA::_ref_type root_poa = PortableServer::POA::_narrow (
				CORBA::Core::Services::bind (CORBA::Core::Services::RootPOA));
			root_poa->find_POA (adapter_name_, true);
			assert (adapter ());

			// Create file system root object
			CORBA::servant_reference <FileSystem> fs = CORBA::make_reference <FileSystem> ();
			root_poa->activate_object (fs);
			ret = fs->_this ();

		SYNC_END ();
		return ret;
	}

	FileSystem ()
	{
		// Build root map
		Roots roots = Port::FileSystem::get_roots ();
		for (auto& r : roots) {
			roots_.emplace (std::move (r.dir), r.factory);
		}
	}

	~FileSystem ()
	{}

	static Nirvana::FileType get_item_type (const DirItemId& id)
	{
		return Port::FileSystem::get_item_type (id);
	}

	static Nirvana::DirItem::_ref_type get_reference (const DirItemId& id);
	static Nirvana::Dir::_ref_type get_dir (const DirItemId& id);
	static Nirvana::File::_ref_type get_file (const DirItemId& id);

	static PortableServer::ServantBase::_ref_type incarnate (const DirItemId& id)
	{
		return Port::FileSystem::incarnate (id);
	}

	static void etherealize (const DirItemId& id, CORBA::Object::_ptr_type servant)
	{
		Port::FileSystem::etherealize (id, servant);
	}

	static PortableServer::POA::_ref_type& adapter () noexcept
	{
		assert ((PortableServer::POA::_ref_type&)adapter_);
		return adapter_;
	}

	/// Translate path from a host-specific form to standard.
	/// 
	/// On the UNIX-like systems probably does nothing and just returns `false`.
	/// 
	/// \param path Host-specific path.
	/// \param [out] translated Translated standard path.
	/// \returns `true` if path was translated and \p translated string is not empty.
	static bool translate_path (const IDL::String& path, IDL::String& translated)
	{
		return Port::FileSystem::translate_path (path, translated);
	}

	/// Append filesystem path to CosNaming::Name
	/// 
	/// \param [in,out] name Target name.
	/// \param path Path to append.
	static void append_path (CosNaming::Name& name, const IDL::String& path);

	static bool is_absolute (const CosNaming::Name& n) noexcept;
	static bool is_absolute (const IDL::String& path) noexcept;

	Access::_ref_type open (CosNaming::Name& n, uint_fast16_t flags, uint_fast16_t mode)
	{
		check_name (n);
		Nirvana::Dir::_ref_type dir (resolve_root (n));
		if (dir) {
			n.erase (n.begin ());
			return dir->open (n, flags, mode);
		} else
			throw NotFound (NotFoundReason::missing_node, std::move (n));
	}

	Access::_ref_type mkostemps (IDL::String&, uint_fast16_t, uint_fast16_t, mode_t)
	{
		throw_NO_IMPLEMENT (make_minor_errno (ENOSYS));
	}

	static void opendir (const IDL::String& regexp, unsigned flags,
		uint32_t how_many, DirEntryList& l, DirIterator::_ref_type& di)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	bool mkdir (CosNaming::Name& n, uint_fast16_t mode)
	{
		check_name (n);
		Nirvana::Dir::_ref_type dir (resolve_root (n));
		if (dir) {
			n.erase (n.begin ());
			return dir->mkdir (n, mode);
		} else
			throw NotFound (NotFoundReason::missing_node, std::move (n));
	}

	uint_fast16_t access (CosNaming::Name& n)
	{
		check_name (n);
		Nirvana::Dir::_ref_type dir (resolve_root (n));
		if (dir) {
			n.erase (n.begin ());
			return dir->access (n);
		} else
			return 0;
	}

	void destroy ()
	{
		throw NotEmpty ();
	}

	// DirItem

	static FileType type () noexcept
	{
		return FileType::directory;
	}

	void stat (FileStat& st) const
	{}

	void remove ()
	{
		throw_BAD_OPERATION (make_minor_errno (ENOSYS));
	}

	static DirItemId id ()
	{
		return DirItemId (1, 0);
	}

	// NamingContextBase

	virtual void bind1 (CosNaming::Name& n, CORBA::Object::_ptr_type obj) override;
	virtual void rebind1 (CosNaming::Name& n, CORBA::Object::_ptr_type obj) override;
	virtual void bind_context1 (CosNaming::Name& n, CosNaming::NamingContext::_ptr_type nc) override;
	virtual void rebind_context1 (CosNaming::Name& n, CosNaming::NamingContext::_ptr_type nc) override;
	virtual CORBA::Object::_ref_type resolve1 (CosNaming::Name& n) override;
	virtual void unbind1 (CosNaming::Name& n) override;
	virtual CosNaming::NamingContext::_ref_type bind_new_context1 (CosNaming::Name& n) override;
	virtual void get_bindings (CosNaming::Core::IteratorStack& iter) const override;

	// FileSystem
	DirItem::_ref_type get_item (const DirItemId& id)
	{
		return get_reference (id);
	}

	static CosNaming::NamingContext::_ref_type new_context ()
	{
		throw_NO_IMPLEMENT ();
	}

private:
	Nirvana::Dir::_ref_type resolve_root (const CosNaming::Name& n);
	static CORBA::Object::_ref_type get_reference (const DirItemId& id, CORBA::Internal::String_in iid);

private:
	struct Root {
		GetRootId factory;
		Nirvana::Dir::_ref_type cached;

		Root (GetRootId f) :
			factory (f)
		{}
	};

	typedef Nirvana::Core::MapUnorderedUnstable <std::string, Root, std::hash <std::string>,
		std::equal_to <std::string>, Nirvana::Core::UserAllocator> RootMap;

	RootMap roots_;

	friend class PortableServer::Core::FileActivator;
	static Nirvana::Core::StaticallyAllocated <PortableServer::POA::_ref_type> adapter_;
};

}
}

#endif
