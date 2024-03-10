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
#ifndef NIRVANA_CORE_MEMCONTEXT_H_
#define NIRVANA_CORE_MEMCONTEXT_H_
#pragma once

#include "Heap.h"
#include "DeadlinePolicyContext.h"
#include "RuntimeSupportContext.h"
#include "RuntimeGlobal.h"
#include "TLS_Context.h"
#include "FileDescriptorsContext.h"
#include "CurrentDirContext.h"
#include "UserObject.h"
#include <memory>

namespace Nirvana {
namespace Core {

/// Memory context.
/// Contains heap and some other stuff.
///
/// Unlike the heap, memory context is not thread-safe and and must be used by exactly one
/// execution context at the given time.
/// A number of the memory contexts can share the one heap.
/// MemContext can not be static.
class MemContext final
{
	template <class>
	friend class CORBA::servant_reference;

public:
	/// \returns Current memory context.
	///          If there is no current memory context, a new memory context will be created.
	/// 
	/// \throws CORBA::NO_MEMORY.
	static MemContext& current ();

	/// \returns Current memory context.
	///          If there is no current memory context, `null` will be returned.
	static MemContext* current_ptr () noexcept;

	/// Check if specific memory context is current.
	/// 
	/// \param context A memory context pointer.
	/// \returns `true` if \p context is current memory context.
	static bool is_current (const MemContext* context) noexcept;

	/// Create new heap.
	/// 
	/// For small systems just returns shared heap reference.
	/// 
	static Ref <Heap> create_heap ();

	/// Create new memory context with new heap.
	/// 
	/// \param class_library_init If `true` - create special memory context for class library initialization.
	/// \returns New memory context reference.
	/// 
	static Ref <MemContext> create (bool class_library_init = false)
	{
		return create (create_heap (), class_library_init);
	}

	/// Prevent error with implicit conversion of Heap* to bool.
	void create (void*) = delete;

	/// Create new memory context for existing heap.
	/// 
	/// \param heap Existing heap.
	/// \returns New memory context reference.
	/// 
	static Ref <MemContext> create (Heap& heap)
	{
		return create (&heap, false);
	}

	/// \returns Heap.
	Heap& heap () noexcept
	{
		return *heap_;
	}

	RuntimeSupportContext& runtime_support () noexcept
	{
		return data_holder_;
	}

	DeadlinePolicyContext* deadline_policy_ptr () noexcept
	{
		return data_holder_.deadline_policy_ptr ();
	}

	DeadlinePolicyContext& deadline_policy ()
	{
		return data_holder_.deadline_policy ();
	}

	TLS_Context* tls_ptr () const noexcept
	{
		return data_holder_.tls_ptr ();
	}

	TLS_Context& tls ()
	{
		return data_holder_.tls ();
	}

	FileDescriptorsContext* file_descriptors_ptr () const noexcept
	{
		return data_holder_.file_descriptors_ptr ();
	}

	FileDescriptorsContext& file_descriptors ()
	{
		return data_holder_.file_descriptors ();
	}

	RuntimeGlobal* runtime_global_ptr () const noexcept
	{
		return data_holder_.runtime_global_ptr ();
	}

	RuntimeGlobal& runtime_global ()
	{
		return data_holder_.runtime_global ();
	}

	CurrentDirContext* current_dir_ptr () const noexcept
	{
		return data_holder_.current_dir_ptr ();
	}

	CurrentDirContext& current_dir ()
	{
		return data_holder_.current_dir ();
	}

private:
	static Ref <MemContext> create (Ref <Heap>&& heap, bool class_library_init);

	~MemContext ();

	MemContext (Ref <Heap>&& heap, bool class_library_init) noexcept;

	MemContext (const MemContext&) = delete;

	MemContext& operator = (const MemContext&) = delete;

	void _add_ref () noexcept
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () noexcept;

private:
	class CreateRef : public Ref <MemContext>
	{
	public:
		CreateRef (MemContext* p) :
			Ref <MemContext> (p, false)
		{}
	};

	class DataHolder :
		public RuntimeSupportContext // sizeof (void*) or 0
	{
	public:
		DataHolder () :
			data_ (nullptr)
		{}

		DeadlinePolicyContext& deadline_policy ()
		{
			return deadline_policy_;
		}

		DeadlinePolicyContext* deadline_policy_ptr () noexcept
		{
			return &deadline_policy_;
		}

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
		class Data : public UserObject,
			public RuntimeGlobal,
			public TLS_Context,
			public CurrentDirContext,
			public FileDescriptorsContext
		{
		public:
			Data ()
			{}

			~Data ()
			{}

		};

		Data& data ()
		{
			if (!data_)
				data_.reset (new Data ());
			return *data_;
		}

		Data* data_ptr () const noexcept
		{
			return data_.get ();
		}

	private:
		DeadlinePolicyContext deadline_policy_; // 16 bytes
		std::unique_ptr <Data> data_; // sizeof (void*)
	};

	class Replacer;

private:
	DataHolder data_holder_;
	Ref <Heap> heap_;
	AtomicCounter <false> ref_cnt_;
	bool class_library_init_;
};

}
}

#endif
