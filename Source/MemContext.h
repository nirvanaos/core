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
#include "LocaleContext.h"
#include "UserObject.h"
#include "LocaleImpl.h"
#include "ORB/ValueList.h"
#include <memory>

namespace Nirvana {
namespace Core {

class ExecDomain;

/// Memory context. Isolated computation environment.
/// 
/// Memory context contains reference to a heap.
/// Unlike the heap, memory context is not thread-safe and and must be used by exactly one
/// execution context at the given time.
/// A number of the memory contexts can share the one heap.
/// 
/// MemContext can not be static.
/// 
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
		return create (create_heap (), class_library_init, false);
	}

	/// Prevent error with implicit conversion of Heap* to bool.
	void create (void*) = delete;

	/// Create new memory context for existing heap.
	/// 
	/// \param heap Existing heap.
	/// \param core_context `true` for core memory context
	/// \returns New memory context reference.
	/// 
	static Ref <MemContext> create (Heap& heap, bool core_context = false)
	{
		return create (&heap, false, core_context);
	}

	/// \returns Heap.
	Heap& heap () noexcept
	{
		return *heap_;
	}

	const RuntimeSupportContext* runtime_support_ptr () noexcept
	{
		return data_holder_.runtime_support_ptr ();
	}

	RuntimeSupportContext& runtime_support ()
	{
		return data_holder_.runtime_support ();
	}

	DeadlinePolicyContext* deadline_policy_ptr () noexcept
	{
		return data_holder_.deadline_policy_ptr ();
	}

	DeadlinePolicyContext& deadline_policy ()
	{
		return data_holder_.deadline_policy ();
	}

	TLS_Context* tls_ptr () noexcept
	{
		return data_holder_.tls_ptr ();
	}

	TLS_Context& tls ()
	{
		return data_holder_.tls ();
	}

	FileDescriptorsContext* file_descriptors_ptr () noexcept
	{
		return data_holder_.file_descriptors_ptr ();
	}

	FileDescriptorsContext& file_descriptors ()
	{
		return data_holder_.file_descriptors ();
	}

	RuntimeGlobal* runtime_global_ptr () noexcept
	{
		return data_holder_.runtime_global_ptr ();
	}

	RuntimeGlobal& runtime_global ()
	{
		return data_holder_.runtime_global ();
	}

	CurrentDirContext* current_dir_ptr () noexcept
	{
		return data_holder_.current_dir_ptr ();
	}

	CurrentDirContext& current_dir ()
	{
		return data_holder_.current_dir ();
	}

	bool core_context () noexcept
	{
		return core_context_;
	}

	FileDescriptors get_inherited_files (unsigned create_std_mask);

	CORBA::Core::ValueList& value_list () noexcept
	{
		return value_list_;
	}

	Nirvana::Locale::_ptr_type cur_locale () noexcept
	{
		Nirvana::Locale::_ptr_type ret = locale ();
		if (!ret)
			ret = global_locale ();
		return ret;
	}

	void set_global_locale (Nirvana::Locale::_ptr_type loc) noexcept
	{
		global_locale_ = loc;
	}

	typedef int locale_t;
	static const locale_t GLOBAL_LOCALE = -1;

	locale_t add_locale (Nirvana::Locale::_ptr_type loc)
	{
		return data_holder_.locale_context ().add_locale (loc);
	}

	locale_t duplocale (locale_t locobj)
	{
		return data_holder_.locale_context ().add_locale (get_locale (locobj));
	}

	void freelocale (locale_t locobj) noexcept
	{
		LocaleContext* lc = data_holder_.locale_context_ptr ();
		if (lc)
			lc->freelocale (locobj);
	}

	locale_t uselocale (locale_t locobj)
	{
		locale_t ret;
		LocaleContext* lc = data_holder_.locale_context_ptr ();
		if (lc) {
			if (0 == locobj)
				ret = lc->cur_locale ();
			else
				ret = lc->uselocale (GLOBAL_LOCALE == locobj ? 0 : locobj);
		}	else if (0 != locobj && GLOBAL_LOCALE != locobj)
			throw_BAD_PARAM (make_minor_errno (EINVAL));
		else
			ret = GLOBAL_LOCALE;
		return ret;
	}

	Nirvana::Locale::_ref_type get_locale (locale_t locobj);

private:
	static Ref <MemContext> create (Ref <Heap>&& heap, bool class_library_init, bool core_context);

	~MemContext ();

	void destroy (ExecDomain& cur_ed) noexcept;

	MemContext (Ref <Heap>&& heap, bool class_library_init, bool core_context) noexcept;

	MemContext (const MemContext&) = delete;
	MemContext& operator = (const MemContext&) = delete;

	void _add_ref () noexcept
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () noexcept;

	Nirvana::Locale::_ptr_type locale () noexcept;

	Nirvana::Locale::_ptr_type global_locale ()
	{
		if (!global_locale_)
			global_locale_ = locale_default ();
		return global_locale_;
	}

private:
	class CreateRef : public Ref <MemContext>
	{
	public:
		CreateRef (MemContext* p) :
			Ref <MemContext> (p, false)
		{}
	};

	class DataHolderSimple
	{
	public:
		RuntimeSupportContext& runtime_support ()
		{
			return data ();
		}

		RuntimeSupportContext* runtime_support_ptr () const noexcept
		{
			return data_ptr ();
		}

		DeadlinePolicyContext& deadline_policy ()
		{
			return data ();
		}

		DeadlinePolicyContext* deadline_policy_ptr () const noexcept
		{
			return data_ptr ();
		}

		TLS_Context* tls_ptr () noexcept
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

		LocaleContext* locale_context_ptr () const noexcept
		{
			return data_ptr ();
		}

	private:
		class Data : public UserObject,
			public RuntimeSupportContext,
			public DeadlinePolicyContext,
			public RuntimeGlobal,
			public TLS_Context,
			public CurrentDirContext,
			public FileDescriptorsContext,
			public LocaleContext
		{};

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
		std::unique_ptr <Data> data_;
	};

	class DataHolder32
	{
	public:
		RuntimeSupportContext& runtime_support ()
		{
			return data1 ();
		}

		RuntimeSupportContext* runtime_support_ptr () const noexcept
		{
			return data_ptr1 ();
		}

		DeadlinePolicyContext& deadline_policy ()
		{
			return data0 ();
		}

		DeadlinePolicyContext* deadline_policy_ptr () const noexcept
		{
			return data_ptr0 ();
		}

		TLS_Context* tls_ptr () const noexcept
		{
			return data_ptr0 ();
		}

		TLS_Context& tls ()
		{
			return data0 ();
		}

		FileDescriptorsContext* file_descriptors_ptr () const noexcept
		{
			return data_ptr1 ();
		}

		FileDescriptorsContext& file_descriptors ()
		{
			return data1 ();
		}

		RuntimeGlobal* runtime_global_ptr () const noexcept
		{
			return data_ptr1 ();
		}

		RuntimeGlobal& runtime_global ()
		{
			return data1 ();
		}

		CurrentDirContext* current_dir_ptr () const noexcept
		{
			return data_ptr0 ();
		}

		CurrentDirContext& current_dir ()
		{
			return data0 ();
		}

		LocaleContext* locale_context_ptr () const noexcept
		{
			return data_ptr1 ();
		}

		LocaleContext& locale_context ()
		{
			return data1 ();
		}

	private:
		class Data0 : public UserObject,
			public DeadlinePolicyContext,
			public TLS_Context,
			public CurrentDirContext
		{};

		class Data1 : public UserObject,
			public RuntimeSupportContext,
			public RuntimeGlobal,
			public FileDescriptorsContext,
			public LocaleContext
		{};

		Data0& data0 ()
		{
			if (!data0_)
				data0_.reset (new Data0 ());
			return *data0_;
		}

		Data0* data_ptr0 () const noexcept
		{
			return data0_.get ();
		}

		Data1& data1 ()
		{
			if (!data1_)
				data1_.reset (new Data1 ());
			return *data1_;
		}

		Data1* data_ptr1 () const noexcept
		{
			return data1_.get ();
		}

	private:
		std::unique_ptr <Data0> data0_;
		std::unique_ptr <Data1> data1_;
	};

	class DataHolder64
	{
	public:
		RuntimeSupportContext& runtime_support ()
		{
			return data1 ();
		}

		RuntimeSupportContext* runtime_support_ptr () const noexcept
		{
			return data_ptr1 ();
		}

		DeadlinePolicyContext& deadline_policy ()
		{
			return data0 ();
		}

		DeadlinePolicyContext* deadline_policy_ptr () const noexcept
		{
			return data_ptr0 ();
		}

		TLS_Context* tls_ptr () const noexcept
		{
			return data_ptr0 ();
		}

		TLS_Context& tls ()
		{
			return data0 ();
		}

		FileDescriptorsContext* file_descriptors_ptr () const noexcept
		{
			return data_ptr2 ();
		}

		FileDescriptorsContext& file_descriptors ()
		{
			return data2 ();
		}

		RuntimeGlobal* runtime_global_ptr () const noexcept
		{
			return data_ptr1 ();
		}

		RuntimeGlobal& runtime_global ()
		{
			return data1 ();
		}

		CurrentDirContext* current_dir_ptr () const noexcept
		{
			return data_ptr0 ();
		}

		CurrentDirContext& current_dir ()
		{
			return data0 ();
		}

		LocaleContext* locale_context_ptr () const noexcept
		{
			return data_ptr2 ();
		}

		LocaleContext& locale_context ()
		{
			return data2 ();
		}

	private:
		class Data0 : public UserObject,
			public DeadlinePolicyContext,
			public TLS_Context,
			public CurrentDirContext
		{};

		class Data1 : public UserObject,
			public RuntimeSupportContext,
			public RuntimeGlobal
		{};

		class Data2 : public UserObject,
			public FileDescriptorsContext,
			public LocaleContext
		{};

		Data0& data0 ()
		{
			if (!data0_)
				data0_.reset (new Data0 ());
			return *data0_;
		}

		Data0* data_ptr0 () const noexcept
		{
			return data0_.get ();
		}

		Data1& data1 ()
		{
			if (!data1_)
				data1_.reset (new Data1 ());
			return *data1_;
		}

		Data1* data_ptr1 () const noexcept
		{
			return data1_.get ();
		}

		Data2& data2 ()
		{
			if (!data2_)
				data2_.reset (new Data2 ());
			return *data2_;
		}

		Data2* data_ptr2 () const noexcept
		{
			return data2_.get ();
		}

	private:
		std::unique_ptr <Data0> data0_;
		std::unique_ptr <Data1> data1_;
		std::unique_ptr <Data2> data2_;
	};

	typedef std::conditional <sizeof (void*) < 8, DataHolder32, DataHolder64>::type DataHolder;

	class Replacer;

	class Deleter;

private:
	Ref <Heap> heap_;
	DataHolder data_holder_;
	Nirvana::Locale::_ref_type global_locale_;
	CORBA::Core::ValueList value_list_;
	AtomicCounter <false> ref_cnt_;
	bool class_library_init_;
	bool core_context_;
};

}
}

#endif
