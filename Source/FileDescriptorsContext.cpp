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
#include "pch.h"
#include "FileDescriptorsContext.h"
#include "MapUnorderedUnstable.h"
#include <queue>

namespace Nirvana {
namespace Core {

class FileDescriptorsContext::DescriptorBuf : public DescriptorBase
{
public:
	DescriptorBuf (AccessBuf::_ref_type&& access) :
		access_ (std::move (access))
	{}

	virtual Access::_ref_type access () const override;
	virtual void close () const override;
	virtual size_t read (void* p, size_t size) override;
	virtual void write (const void* p, size_t size) override;
	virtual uint64_t seek (unsigned method, const int64_t& off) override;
	virtual unsigned flags () const override;
	virtual void flags (unsigned fl) override;
	virtual void flush () override;
	virtual bool isatty () const override;

private:
	const AccessBuf::_ref_type access_;
};

class FileDescriptorsContext::DescriptorChar : public DescriptorBase
{
public:
	DescriptorChar (AccessChar::_ref_type&& access) :
		access_ (std::move (access)),
		flags_ (access_->flags ())
	{
		if ((flags_ & O_ACCMODE) != O_WRONLY)
			access_->connect_pull_consumer (nullptr);
	}

	virtual Access::_ref_type access () const override;
	virtual void close () const override;
	virtual size_t read (void* p, size_t size) override;
	virtual void write (const void* p, size_t size) override;
	virtual uint64_t seek (unsigned method, const int64_t& off) override;
	virtual unsigned flags () const override;
	virtual void flags (unsigned fl) override;
	virtual void flush () override;
	virtual bool isatty () const override;

private:
	typedef std::queue <char> Buffer;
	const AccessChar::_ref_type access_;
	Buffer buffer_;
	unsigned flags_;
};

size_t FileDescriptorsContext::alloc_fd (unsigned start)
{
	size_t i;
	if (start >= (unsigned)STD_CNT)
		i = start - (unsigned)STD_CNT;
	else
		i = 0;
	if (i < file_descriptors_.size ()) {
		auto it = file_descriptors_.begin ();
		for (; it != file_descriptors_.end (); ++it) {
			if (it->closed ())
				break;
		}
		i = it - file_descriptors_.begin ();
	}
	if (i > (unsigned)std::numeric_limits <int16_t>::max () - (unsigned)STD_CNT)
		throw_IMP_LIMIT (make_minor_errno (EMFILE));
	if (i >= file_descriptors_.size ())
		file_descriptors_.resize (i + 1);
	return i;
}

FileDescriptorsContext::DescriptorInfo& FileDescriptorsContext::get_fd (unsigned ifd)
{
	if (ifd < StandardFileDescriptor::STD_CNT)
		return std_file_descriptors_ [ifd];
	ifd -= StandardFileDescriptor::STD_CNT;
	if (ifd >= file_descriptors_.size ())
		throw_BAD_PARAM (make_minor_errno (EBADF));
	return file_descriptors_ [ifd];
}

FileDescriptorsContext::DescriptorInfo& FileDescriptorsContext::get_open_fd (unsigned ifd)
{
	DescriptorInfo& fd = get_fd (ifd);
	if (fd.closed ())
		throw_BAD_PARAM (make_minor_errno (EBADF));
	return fd;
}

FileDescriptorsContext::DescriptorRef FileDescriptorsContext::make_fd (
	CORBA::AbstractBase::_ptr_type access)
{
	CORBA::ValueBase::_ref_type val = access->_to_value ();
	if (val) {
		AccessBuf::_ref_type ab = AccessBuf::_downcast (val);
		if (!ab)
			throw_UNKNOWN (make_minor_errno (EIO));
		return CORBA::make_reference <DescriptorBuf> (std::move (ab));
	} else {
		AccessChar::_ref_type ac = AccessChar::_narrow (access->_to_object ());
		if (!ac)
			throw_UNKNOWN (make_minor_errno (EIO));
		return CORBA::make_reference <DescriptorChar> (std::move (ac));
	}
}

void FileDescriptorsContext::set_inherited_files (const FileDescriptors& files)
{
	size_t max = 0;
	for (const auto& f : files) {
		for (auto d : f.descriptors ()) {
			if (max < d)
				max = d;
		}
	}
	if (max >= StandardFileDescriptor::STD_CNT)
		file_descriptors_.resize (max + 1 - StandardFileDescriptor::STD_CNT);
	for (const auto& f : files) {
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

FileDescriptors FileDescriptorsContext::get_inherited_files (unsigned* std_mask) const
{
	using Inherited = MapUnorderedUnstable <Descriptor*, IDL::Sequence <uint16_t>,
		std::hash <Descriptor*>, std::equal_to <Descriptor*>, UserAllocator>;

	Inherited inherited;
	unsigned std = 0;
	for (unsigned ifd = 0, end = StandardFileDescriptor::STD_CNT + file_descriptors_.size (); ifd < end; ++ifd) {
		const DescriptorInfo& d = get_fd (ifd);
		if (!d.closed () && !(d.fd_flags () & FD_CLOEXEC)) {
			inherited [d.ptr ()].push_back ((uint16_t)ifd);
			if (ifd < StandardFileDescriptor::STD_CNT)
				std |= 1 << ifd;
		}
	}

	FileDescriptors files;
	for (auto& f : inherited) {
		files.emplace_back (f.first->access ()->dup (0, 0), std::move (f.second));
	}

	if (std_mask)
		*std_mask = std;

	return files;
}

Access::_ref_type FileDescriptorsContext::DescriptorBuf::access () const
{
	return access_;
}

Access::_ref_type FileDescriptorsContext::DescriptorChar::access () const
{
	return access_;
}

void FileDescriptorsContext::DescriptorBuf::close () const
{
	access_->close ();
}

void FileDescriptorsContext::DescriptorChar::close () const
{
	access_->close ();
}

size_t FileDescriptorsContext::DescriptorBuf::read (void* p, size_t size)
{
	return access_->read (p, size);
}

size_t FileDescriptorsContext::DescriptorChar::read (void* p, size_t size)
{
	if ((flags_ & O_ACCMODE) == O_WRONLY)
		throw_NO_PERMISSION (make_minor_errno (EBADF));

	if (!size)
		return 0;

	size_t cb = 0;
	for (;;) {
		while (size && !buffer_.empty ()) {
			*(char*)p = buffer_.front ();
			buffer_.pop ();
			++cb;
			--size;
		}

		if (cb)
			break;

		CORBA::Any evt;
		if (flags_ & O_NONBLOCK) {
			bool has_event;
			evt = access_->try_pull (has_event);
			if (!has_event)
				throw_TRANSIENT (make_minor_errno (EAGAIN));
		} else
			evt = access_->pull ();

		const IDL::String* ps;
		if (evt >>= ps) {
			if (ps->empty ())
				break;
			for (char c : *ps) {
				buffer_.push (c);
			}
		} else {
			AccessChar::Error error;
			if (evt >>= error)
				throw_COMM_FAILURE (make_minor_errno (error.error ()));
		}
	}

	return cb;
}

void FileDescriptorsContext::DescriptorBuf::write (const void* p, size_t size)
{
	access_->write (p, size);
}

void FileDescriptorsContext::DescriptorChar::write (const void* p, size_t size)
{
	access_->write (CORBA::Internal::StringView <char> ((const char*)p, size));
}

uint64_t FileDescriptorsContext::DescriptorBuf::seek (unsigned method, const int64_t& off)
{
	FileSize pos;
	switch (method) {
	case SEEK_SET:
		pos = 0;
		break;

	case SEEK_CUR:
		pos = access_->position ();
		break;

	case SEEK_END:
		pos = access_->direct ()->size ();
		break;

	default:
		throw_BAD_PARAM (make_minor_errno (EINVAL));
	}

	if (off < 0) {
		if (pos < (FileSize)(-off))
			throw_BAD_PARAM (make_minor_errno (EOVERFLOW));
	} else {
		if (std::numeric_limits <FileSize>::max () - pos < (FileSize)off)
			throw_BAD_PARAM (make_minor_errno (EOVERFLOW));
	}

	access_->position (pos += off);
	return pos;
}

uint64_t FileDescriptorsContext::DescriptorChar::seek (unsigned method, const int64_t& off)
{
	throw_BAD_OPERATION (make_minor_errno (ESPIPE));
}

unsigned FileDescriptorsContext::DescriptorBuf::flags () const
{
	return access_->flags ();
}

void FileDescriptorsContext::DescriptorBuf::flags (unsigned fl)
{
	access_->set_flags (POSIX_CHANGEABLE_FLAGS, fl);
}

unsigned FileDescriptorsContext::DescriptorChar::flags () const
{
	return flags_;
}

void FileDescriptorsContext::DescriptorChar::flags (unsigned fl)
{
	unsigned chg = (fl ^ flags_) & POSIX_CHANGEABLE_FLAGS;
	if (chg != O_NONBLOCK)
		access_->set_flags (POSIX_CHANGEABLE_FLAGS, fl);
	flags_ = (flags_ & ~POSIX_CHANGEABLE_FLAGS) | (fl & POSIX_CHANGEABLE_FLAGS);
}

void FileDescriptorsContext::DescriptorBuf::flush ()
{
	access_->flush ();
}

void FileDescriptorsContext::DescriptorChar::flush ()
{
}

bool FileDescriptorsContext::DescriptorBuf::isatty () const
{
	return false;
}

bool FileDescriptorsContext::DescriptorChar::isatty () const
{
	return true;
}

}
}
