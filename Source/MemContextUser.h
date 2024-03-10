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
#ifndef NIRVANA_CORE_MEMCONTEXTUSER_H_
#define NIRVANA_CORE_MEMCONTEXTUSER_H_
#pragma once

#include "MemContext.h"
#include "RuntimeSupportContext.h"
#include "RuntimeGlobal.h"
#include "TLS_Context.h"
#include "FileDescriptorsContext.h"
#include "CurrentDirContext.h"
#include "UserObject.h"
#include <memory>

namespace Nirvana {
namespace Core {

/// \brief Memory context implementation.
class MemContextUser final : public MemContext,
	public RuntimeSupportContext
{
public:
	/// Creates a new memory context with defaults.
	static Ref <MemContext> create (bool class_library_init);

	/// \returns Current user memory context.
	/// \throws CORBA::NO_IMPL If current context is not an user context.
	static MemContextUser& current ();

	static MemContextUser* current_ptr () noexcept
	{
		return cast (MemContext::current_ptr ());
	}

	/// Cast MemContext pointer to MemContextUser pointer.
	static MemContextUser* cast (MemContext* p) noexcept
	{
		return p ? p->user_context () : nullptr;
	}

	CosNaming::Name get_current_dir_name () const;
	void chdir (const IDL::String& path);

	TLS_Context* tls_ptr () const noexcept
	{
		return data_ptr ();
	}

	TLS_Context& tls ()
	{
		return data ();
	}

	FileDescriptorsContext* file_descriptors_ptr () const noexcept
	{
		return data_ptr ();
	}

	FileDescriptorsContext& file_descriptors ()
	{
		return data ();
	}

	RuntimeGlobal* runtime_global_ptr () const noexcept
	{
		return data_ptr ();
	}

	RuntimeGlobal& runtime_global ()
	{
		return data ();
	}

	CurrentDirContext* current_dir_ptr () const noexcept
	{
		return data_ptr ();
	}

	CurrentDirContext& current_dir ()
	{
		return data ();
	}

private:
	friend class MemContext;

	MemContextUser ();

	~MemContextUser ()
	{}

	// In the most cases we don't need the Data.
	// It needed only when we use one of:
	// * Runtime proxies for iterator debugging.
	// * POSIX API.
	// So we create Data on demand.
	class Data : public UserObject,
		public RuntimeGlobal,
		public TLS_Context,
		public CurrentDirContext,
		public FileDescriptorsContext
	{
	public:
		static Data* create ();

		~Data ();

	private:
		Data ()
		{}
	};

	Data& data ();
	Data* data_ptr () const noexcept
	{
		return data_.get ();
	}

private:
	std::unique_ptr <Data> data_;
};

inline MemContextUser* MemContext::user_context () noexcept
{
	if (type_ > MC_CORE)
		return static_cast <MemContextUser*> (this);
	else
		return nullptr;
}

}
}

#endif
