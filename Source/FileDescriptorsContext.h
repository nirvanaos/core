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
#ifndef NIRVANA_CORE_FILEDESCRIPTORSCONTEXT_H_
#define NIRVANA_CORE_FILEDESCRIPTORSCONTEXT_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Nirvana/File.h>
#include <Nirvana/posix_defs.h>
#include "UserObject.h"
#include "UserAllocator.h"
#include <Nirvana/ProcessFactory.h> // For InheritedFiles

namespace Nirvana {
namespace Core {

/// Context for the POSIX file descriptor operations.
class FileDescriptorsContext
{
	static const unsigned POSIX_CHANGEABLE_FLAGS = O_APPEND | O_NONBLOCK;

public:
	FileDescriptorsContext ()
	{}

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
		const DescriptorInfo& src = get_open_fd (ifd);
		size_t i = alloc_fd (start);
		file_descriptors_ [i].dup (src);
		return (unsigned)(i + STD_CNT);
	}

	void dup2 (unsigned ifd_src, unsigned ifd_dst)
	{
		const DescriptorInfo& src = get_open_fd (ifd_src);
		DescriptorInfo& dst = get_fd (ifd_dst);
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

	void set_inherited_files (const InheritedFiles& inh)
	{
		size_t max = 0;
		for (const auto& f : inh) {
			for (auto d : f.descriptors ()) {
				if (max < d)
					max = d;
			}
		}
		if (max >= StandardFileDescriptor::STD_CNT)
			file_descriptors_.resize (max + 1 - StandardFileDescriptor::STD_CNT);
		for (const auto& f : inh) {
			DescriptorRef fd = make_fd (f.access ());
			fd->remove_descriptor_ref ();
			for (auto d : f.descriptors ()) {
				DescriptorInfo& fdr = get_fd (d);
				if (!fdr.closed ())
					throw_BAD_PARAM ();
				fdr.assign (fd);
			}
		}
	}

private:
	class NIRVANA_NOVTABLE Descriptor : public UserObject
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
		Descriptor () :
			descriptor_ref_cnt_ (1),
			error_ (false),
			eof_ (false),
			push_back_cnt_ (0)
		{}

		virtual ~Descriptor ()
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

	typedef ImplDynamicSync <Descriptor> DescriptorBase;
	typedef Ref <DescriptorBase> DescriptorRef;

	class DescriptorInfo
	{
	public:
		DescriptorInfo () noexcept :
			fd_flags_ (0)
		{}

		void close ()
		{
			if (0 == ref_->remove_descriptor_ref ())
				ref_->close ();
			ref_ = nullptr;
			fd_flags_ = 0;
		}

		void attach (DescriptorRef&& fd) noexcept
		{
			assert (!ref_);
			ref_ = std::move (fd);
		}

		void assign (const DescriptorRef& fd) noexcept
		{
			assert (!ref_);
			(ref_ = fd)->add_descriptor_ref ();
		}

		void dup (const DescriptorInfo& src) noexcept
		{
			assert (!ref_);
			(ref_ = src.ref_)->add_descriptor_ref ();
		}

		bool closed () const noexcept
		{
			return !ref_;
		}

		DescriptorRef ref () const noexcept
		{
			return ref_;
		}

		Descriptor* ptr () const noexcept
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
		DescriptorRef ref_;
		unsigned fd_flags_;
	};

	class DescriptorBuf;
	class DescriptorChar;

private:
	static DescriptorRef make_fd (CORBA::AbstractBase::_ptr_type access);
	DescriptorInfo& get_fd (unsigned fd);
	DescriptorInfo& get_open_fd (unsigned fd);
	size_t alloc_fd (unsigned start = 0);

private:
	enum StandardFileDescriptor
	{
		STD_IN,
		STD_OUT,
		STD_ERR,

		STD_CNT
	};

	DescriptorInfo std_file_descriptors_ [StandardFileDescriptor::STD_CNT];
	typedef std::vector <DescriptorInfo, UserAllocator <DescriptorInfo> > Descriptors;
	Descriptors file_descriptors_;
};

}
}

#endif
