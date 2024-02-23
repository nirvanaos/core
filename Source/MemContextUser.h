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
#include "RuntimeSupport.h"
#include "RuntimeGlobal.h"
#include <Nirvana/File.h>
#include <memory>
#include <fnctl.h>

namespace Nirvana {

class InheritedFile;
typedef IDL::Sequence <InheritedFile> InheritedFiles;

namespace Core {

/// \brief Memory context full implementation.
class MemContextUser : public MemContext
{
	static const unsigned POSIX_CHANGEABLE_FLAGS;

public:
	/// Creates a new memory context with defaults.
	static Ref <MemContext> create ()
	{
		return MemContext::create <MemContextUser> ();
	}

	/// \returns Current user memory context.
	/// \throws CORBA::NO_IMPL If current context is not an user context.
	static MemContextUser& current ();

	/// POSIX run-time library global state
	RuntimeGlobal& runtime_global () noexcept
	{
		return runtime_global_;
	}

	/// Search map for runtime proxy for object \p obj.
	/// If proxy exists, returns it. Otherwise creates a new one.
	/// 
	/// \param obj Pointer used as a key.
	/// \returns RuntimeProxy for obj.
	virtual RuntimeProxy::_ref_type runtime_proxy_get (const void* obj) override;

	/// Remove runtime proxy for object \p obj.
	/// 
	/// \param obj Pointer used as a key.
	virtual void runtime_proxy_remove (const void* obj) noexcept override;

	CosNaming::Name get_current_dir_name () const;
	void chdir (const IDL::String& path);

	unsigned fd_add (Access::_ptr_type access)
	{
		return data ().fd_add (access);
	}

	void close (unsigned ifd)
	{
		data_for_fd ().close (ifd);
	}

	size_t read (unsigned ifd, void* p, size_t size)
	{
		return data_for_fd ().read (ifd, p, size);
	}

	void write (unsigned ifd, const void* p, size_t size)
	{
		data_for_fd ().write (ifd, p, size);
	}

	uint64_t seek (unsigned ifd, const int64_t& off, unsigned method)
	{
		return data_for_fd ().seek (ifd, off, method);
	}

	unsigned fcntl (unsigned ifd, int cmd, unsigned arg)
	{
		unsigned ret = 0;
		Data& data = data_for_fd ();
		switch (cmd) {
		case F_DUPFD:
			ret = data.dup (ifd, arg);
			break;

		case F_GETFD:
			ret = data.fd_flags (ifd);
			break;

		case F_SETFD:
			data.fd_flags (ifd, arg);
			break;

		case F_GETFL:
			ret = data.flags (ifd);
			break;

		case F_SETFL:
			data.flags (ifd, arg);
			break;

		default:
			throw_BAD_PARAM (make_minor_errno (EINVAL));
		}
		return ret;
	}

	void flush (unsigned ifd)
	{
		data_for_fd ().flush (ifd);
	}

	void dup2 (unsigned src, unsigned dst)
	{
		data_for_fd ().dup2 (src, dst);
	}

	bool isatty (unsigned ifd)
	{
		return data_for_fd ().isatty (ifd);
	}

	void push_back (unsigned ifd, int c)
	{
		data_for_fd ().push_back (ifd, c);
	}

	bool ferror (unsigned ifd)
	{
		return data_for_fd ().ferror (ifd);
	}

	bool feof (unsigned ifd)
	{
		return data_for_fd ().feof (ifd);
	}

	void clearerr (unsigned ifd)
	{
		data_for_fd ().clearerr (ifd);
	}

protected:
	friend class MemContext;

	MemContextUser (Ref <Heap>&& heap = create_heap ()) :
		MemContext (std::move (heap), true)
	{}

	MemContextUser (Heap& heap, const InheritedFiles& inh);
	~MemContextUser ();

	void clear () noexcept
	{
		data_.reset ();
		runtime_global_.clear ();
	}

private:
	class NIRVANA_NOVTABLE FileDescriptor : public UserObject
	{
		static const unsigned PUSH_BACK_MAX = 3;

	public:
		virtual void close () const = 0;
		virtual size_t read (void* p, size_t size) = 0;
		virtual void write (const void* p, size_t size) = 0;
		virtual uint64_t seek (unsigned method, const int64_t& off) = 0;
		virtual unsigned flags () const = 0;
		virtual void flags (unsigned fl) = 0;
		virtual void flush () = 0;
		virtual bool isatty () const = 0;

		void push_back (int c)
		{
			if (push_back_cnt_ < PUSH_BACK_MAX)
				push_back_buf_ [push_back_cnt_++] = (uint8_t)c;
			else
				throw_IMP_LIMIT (ERANGE);
		}

		bool error () const noexcept
		{
			return error_;
		}

		bool eof () const noexcept
		{
			return eof_;
		}

		void clearerr () noexcept
		{
			error_ = false;
			eof_ = false;
		}

		void add_descriptor_ref () noexcept
		{
			++descriptor_ref_cnt_;
		}

		unsigned remove_descriptor_ref () noexcept
		{
			assert (descriptor_ref_cnt_);
			return --descriptor_ref_cnt_;
		}

	protected:
		FileDescriptor () :
			descriptor_ref_cnt_ (1),
			error_ (false),
			eof_ (false),
			push_back_cnt_ (0)
		{}

		virtual ~FileDescriptor ()
		{}

		size_t push_back_read (void*& p, size_t& size) noexcept;

		unsigned push_back_cnt () const noexcept
		{
			return push_back_cnt_;
		}

		void push_back_reset () noexcept
		{
			push_back_cnt_ = 0;
		}

	private:
		unsigned descriptor_ref_cnt_;

	protected:
		bool error_;
		bool eof_;

	private:
		uint8_t push_back_cnt_;
		uint8_t push_back_buf_ [PUSH_BACK_MAX];
	};

	typedef ImplDynamicSync <FileDescriptor> FileDescriptorBase;
	typedef Ref <FileDescriptorBase> FileDescriptorRef;

	class FileDescriptorInfo
	{
	public:
		FileDescriptorInfo () noexcept :
			fd_flags_ (0)
		{}

		void close ()
		{
			if (0 == ref_->remove_descriptor_ref ())
				ref_->close ();
			ref_ = nullptr;
			fd_flags_ = 0;
		}

		void attach (FileDescriptorRef&& fd) noexcept
		{
			assert (!ref_);
			ref_ = std::move (fd);
		}

		void assign (const FileDescriptorRef& fd) noexcept
		{
			assert (!ref_);
			(ref_ = fd)->add_descriptor_ref ();
		}

		void dup (const FileDescriptorInfo& src) noexcept
		{
			assert (!ref_);
			(ref_ = src.ref_)->add_descriptor_ref ();
		}

		bool closed () const noexcept
		{
			return !ref_;
		}

		FileDescriptorRef ref () const noexcept
		{
			return ref_;
		}

		FileDescriptor* ptr () const noexcept
		{
			return ref_;
		}

		unsigned fd_flags () const noexcept
		{
			return fd_flags_;
		}

		void fd_flags (unsigned fl) noexcept
		{
			fd_flags_ = fl;
		}

	private:
		FileDescriptorRef ref_;
		unsigned fd_flags_;
	};

	class FileDescriptorBuf;
	class FileDescriptorChar;

	// In the most cases we don't need the Data.
	// It needed only when we use one of:
	// * Runtime proxies for iterator debugging.
	// * POSIX API.
	// So we create Data on demand.
	class Data : public UserObject
	{
	public:
		static Data* create ();

		~Data ();

		RuntimeProxy::_ref_type runtime_proxy_get (const void* obj)
		{
			return runtime_support_.runtime_proxy_get (obj);
		}

		void runtime_proxy_remove (const void* obj) noexcept
		{
			runtime_support_.runtime_proxy_remove (obj);
		}

		const CosNaming::Name& current_dir () const
		{
			return current_dir_;
		}

		void chdir (const IDL::String& path);

		static CosNaming::Name default_dir ();

		unsigned fd_add (Access::_ptr_type access)
		{
			// Allocate file descriptor cell
			size_t i = alloc_fd ();
			file_descriptors_ [i].attach (make_fd (access));

			return (unsigned)(i + STD_CNT);
		}

		void close (unsigned ifd)
		{
			get_open_fd (ifd).close ();
			while (!file_descriptors_.empty () && file_descriptors_.back ().closed ()) {
				file_descriptors_.pop_back ();
			}
		}

		size_t read (unsigned ifd, void* p, size_t size)
		{
			return get_open_fd (ifd).ref ()->read (p, size);
		}

		void write (unsigned ifd, const void* p, size_t size)
		{
			get_open_fd (ifd).ref ()->write (p, size);
		}

		uint64_t seek (unsigned ifd, const int64_t& off, unsigned method)
		{
			return get_open_fd (ifd).ref ()->seek (method, off);
		}

		unsigned dup (unsigned ifd, unsigned start)
		{
			const FileDescriptorInfo& src = get_open_fd (ifd);
			size_t i = alloc_fd (start);
			file_descriptors_ [i].dup (src);
			return (unsigned)(i + STD_CNT);
		}

		void dup2 (unsigned ifd_src, unsigned ifd_dst)
		{
			const FileDescriptorInfo& src = get_open_fd (ifd_src);
			FileDescriptorInfo& dst = get_fd (ifd_dst);
			if (dst.closed ())
				dst.close ();
			dst.dup (src);
		}

		unsigned fd_flags (unsigned ifd)
		{
			return get_open_fd (ifd).fd_flags ();
		}

		void fd_flags (unsigned ifd, unsigned flags)
		{
			if (flags & ~FD_CLOEXEC)
				throw_INV_FLAG (make_minor_errno (EINVAL));

			get_open_fd (ifd).fd_flags (flags);
		}

		unsigned flags (unsigned ifd)
		{
			return get_open_fd (ifd).ptr ()->flags ();
		}

		void flags (unsigned ifd, unsigned flags)
		{
			return get_open_fd (ifd).ref ()->flags (flags);
		}

		void flush (unsigned ifd)
		{
			get_open_fd (ifd).ref ()->flush ();
		}

		bool isatty (unsigned ifd)
		{
			return get_open_fd (ifd).ptr ()->isatty ();
		}

		void push_back (unsigned ifd, int c)
		{
			get_open_fd (ifd).ptr ()->push_back (c);
		}

		bool ferror (unsigned ifd)
		{
			return get_open_fd (ifd).ptr ()->error ();
		}

		bool feof (unsigned ifd)
		{
			return get_open_fd (ifd).ptr ()->eof ();
		}

		void clearerr (unsigned ifd)
		{
			get_open_fd (ifd).ptr ()->clearerr ();
		}

		Data (const InheritedFiles& inh);

	private:
		Data ()
		{}

		CosNaming::Name get_name_from_path (const IDL::String& path) const;
		static CosNaming::NamingContext::_ref_type name_service ();
		static FileDescriptorRef make_fd (CORBA::AbstractBase::_ptr_type access);
		FileDescriptorInfo& get_fd (unsigned fd);
		FileDescriptorInfo& get_open_fd (unsigned fd);
		size_t alloc_fd (unsigned start = 0);

		RuntimeSupportImpl runtime_support_;
		CosNaming::Name current_dir_;

		enum StandardFileDescriptor
		{
			STD_IN,
			STD_OUT,
			STD_ERR,

			STD_CNT
		};

		FileDescriptorInfo std_file_descriptors_ [StandardFileDescriptor::STD_CNT];
		typedef std::vector <FileDescriptorInfo, UserAllocator <FileDescriptorInfo> > FileDescriptors;
		FileDescriptors file_descriptors_;
	};

	Data& data ();
	Data& data_for_fd () const;

private:
	std::unique_ptr <Data> data_;
	RuntimeGlobal runtime_global_;
};

inline MemContextUser* MemContext::user_context () noexcept
{
	if (user_)
		return static_cast <MemContextUser*> (this);
	else
		return nullptr;
}

}
}

#endif
