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
#include "MemContextUser.h"
#include "NameService/FileSystem.h"
#include "ORB/Services.h"
#include "virtual_copy.h"
#include "BinderMemory.h"
#include <queue>

using namespace CosNaming;
using CORBA::Core::Services;

namespace Nirvana {
namespace Core {

const unsigned MemContextUser::POSIX_CHANGEABLE_FLAGS = O_APPEND | O_NONBLOCK;

Ref <MemContext> MemContextUser::create (bool class_library_init)
{
	if (class_library_init)
		return MemContext::create <MemContextUser> (BinderMemory::heap (), create_heap (), MC_CLASS_LIBRARY);
	else
		return MemContext::create <MemContextUser> (create_heap (), MC_USER);
}

size_t MemContextUser::FileDescriptor::push_back_read (void*& p, size_t& size) noexcept
{
	size_t cnt = push_back_cnt_;
	if (cnt) {
		const uint8_t* top = push_back_buf_ + cnt;
		if (cnt > size)
			cnt = size;
		const uint8_t* end = top - cnt;
		uint8_t* dst = (uint8_t*)p;
		while (top != end) {
			*(dst++) = *(--top);
		}
		size -= cnt;
		p = dst;
	}
	return cnt;
}

class MemContextUser::FileDescriptorBuf : public FileDescriptorBase
{
public:
	FileDescriptorBuf (AccessBuf::_ref_type&& access) :
		access_ (std::move (access))
	{}

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

class MemContextUser::FileDescriptorChar : public FileDescriptorBase
{
public:
	FileDescriptorChar (AccessChar::_ref_type&& access) :
		access_ (std::move (access)),
		flags_ (access_->flags ())
	{
		if ((flags_ & O_ACCMODE) != O_WRONLY)
			access_->connect_pull_consumer (nullptr);
	}

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

inline
MemContextUser::Data* MemContextUser::Data::create ()
{
	return new Data;
}

MemContextUser::Data::~Data ()
{}

Name MemContextUser::Data::default_dir ()
{
	Name home (2);
	home.back ().id ("home");
	return home;
}

Name MemContextUser::Data::get_name_from_path (const IDL::String& path) const
{
	IDL::String translated;
	const IDL::String* ppath;
	if (Port::FileSystem::translate_path (path, translated))
		ppath = &translated;
	else
		ppath = &path;

	Name name;
	if (!FileSystem::is_absolute (*ppath))
		name = current_dir_.empty () ? default_dir () : current_dir_;
	FileSystem::append_path (name, *ppath);
	return name;
}

NamingContext::_ref_type MemContextUser::Data::name_service ()
{
	return NamingContext::_narrow (Services::bind (Services::NameService));
}

inline void MemContextUser::Data::chdir (const IDL::String& path)
{
	if (path.empty ()) {
		current_dir_.clear ();
		return;
	}

	Name new_dir = get_name_from_path (path);

	// Check that new directory exists
	Dir::_ref_type dir = Dir::_narrow (name_service ()->resolve (new_dir));
	if (!dir)
		throw_INTERNAL (make_minor_errno (ENOTDIR));

	if (dir->_non_existent ())
		throw_OBJECT_NOT_EXIST (make_minor_errno (ENOENT));

	current_dir_ = std::move (new_dir);
}

size_t MemContextUser::Data::alloc_fd (unsigned start)
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

MemContextUser::FileDescriptorInfo& MemContextUser::Data::get_fd (unsigned ifd)
{
	if (ifd < StandardFileDescriptor::STD_CNT)
		return std_file_descriptors_ [ifd];
	ifd -= StandardFileDescriptor::STD_CNT;
	if (ifd >= file_descriptors_.size ())
		throw_BAD_PARAM (make_minor_errno (EBADF));
	return file_descriptors_ [ifd];
}

MemContextUser::FileDescriptorInfo& MemContextUser::Data::get_open_fd (unsigned ifd)
{
	FileDescriptorInfo& fd = get_fd (ifd);
	if (fd.closed ())
		throw_BAD_PARAM (make_minor_errno (EBADF));
	return fd;
}

MemContextUser::FileDescriptorRef MemContextUser::Data::make_fd (CORBA::AbstractBase::_ptr_type access)
{
	CORBA::ValueBase::_ref_type val = access->_to_value ();
	if (val) {
		AccessBuf::_ref_type ab = AccessBuf::_downcast (val);
		if (!ab)
			throw_UNKNOWN (make_minor_errno (EIO));
		return CORBA::make_reference <FileDescriptorBuf> (std::move (ab));
	} else {
		AccessChar::_ref_type ac = AccessChar::_narrow (access->_to_object ());
		if (!ac)
			throw_UNKNOWN (make_minor_errno (EIO));
		return CORBA::make_reference <FileDescriptorChar> (std::move (ac));
	}
}

MemContextUser& MemContextUser::current ()
{
	MemContextUser* p = MemContext::current ().user_context ();
	if (!p)
		throw_BAD_OPERATION (); // Unavailable for core contexts
	return *p;
}

MemContextUser::MemContextUser ()
{}

MemContextUser::Data& MemContextUser::data ()
{
	if (!data_)
		data_.reset (Data::create ());
	return *data_;
}

RuntimeProxy::_ref_type MemContextUser::runtime_proxy_get (const void* obj)
{
	return data ().runtime_proxy_get (obj);
}

void MemContextUser::runtime_proxy_remove (const void* obj) noexcept
{
	if (data_)
		data_->runtime_proxy_remove (obj);
}

CosNaming::Name MemContextUser::get_current_dir_name () const
{
	CosNaming::Name cur_dir;
	if (data_)
		cur_dir = data_->current_dir ();
	if (cur_dir.empty ())
		cur_dir = Data::default_dir ();
	return cur_dir;
}

void MemContextUser::chdir (const IDL::String& path)
{
	data ().chdir (path);
}

MemContextUser::Data& MemContextUser::data_for_fd () const
{
	if (!data_)
		throw_BAD_PARAM (make_minor_errno (EBADF));
	return *data_;
}

void MemContextUser::FileDescriptorBuf::close () const
{
	access_->close ();
}

void MemContextUser::FileDescriptorChar::close () const
{
	access_->close ();
}

size_t MemContextUser::FileDescriptorBuf::read (void* p, size_t size)
{
	error_ = false;
	eof_ = false;
	size_t cb = push_back_read (p, size);
	if (size) {
		try {
			size_t cbr = access_->read (p, size);
			if (cbr < size)
				eof_ = true;
			cb += cbr;
		} catch (...) {
			error_ = true;
			throw;
		}
	}
	return cb;
}

size_t MemContextUser::FileDescriptorChar::read (void* p, size_t size)
{
	if ((flags_ & O_ACCMODE) == O_WRONLY)
		throw_NO_PERMISSION (make_minor_errno (EBADF));

	error_ = false;
	eof_ = false;

	if (!size)
		return 0;

	size_t cb = push_back_read (p, size);
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
			if (!has_event) {
				eof_ = true;
				throw_TRANSIENT (make_minor_errno (EAGAIN));
			}
		} else
			evt = access_->pull ();

		const IDL::String* ps;
		if (evt >>= ps) {
			if (ps->empty ()) {
				eof_ = true;
				break;
			}
			for (char c : *ps) {
				buffer_.push (c);
			}
		} else {
			AccessChar::Error error;
			if (evt >>= error) {
				error_ = true;
				throw_COMM_FAILURE (make_minor_errno (error.error ()));
			}
		}
	}

	return cb;
}

void MemContextUser::FileDescriptorBuf::write (const void* p, size_t size)
{
	push_back_reset ();
	access_->write (p, size);
}

void MemContextUser::FileDescriptorChar::write (const void* p, size_t size)
{
	push_back_reset ();
	access_->write (CORBA::Internal::StringView <char> ((const char*)p, size));
}

uint64_t MemContextUser::FileDescriptorBuf::seek (unsigned method, const int64_t& off)
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
		pos = access_->size ();
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

	if (0 == off)
		pos -= push_back_cnt ();
	else
		push_back_reset ();

	access_->position (pos += off);
	return pos;
}

uint64_t MemContextUser::FileDescriptorChar::seek (unsigned method, const int64_t& off) 
{
	throw_BAD_OPERATION (make_minor_errno (ESPIPE));
}

unsigned MemContextUser::FileDescriptorBuf::flags () const
{
	return access_->flags ();
}

void MemContextUser::FileDescriptorBuf::flags (unsigned fl)
{
	access_->set_flags (POSIX_CHANGEABLE_FLAGS, fl);
}

unsigned MemContextUser::FileDescriptorChar::flags () const
{
	return flags_;
}

void MemContextUser::FileDescriptorChar::flags (unsigned fl)
{
	unsigned chg = (fl ^ flags_) & POSIX_CHANGEABLE_FLAGS;
	if (chg != O_NONBLOCK)
		access_->set_flags (POSIX_CHANGEABLE_FLAGS, fl);
	flags_ = (flags_ & ~POSIX_CHANGEABLE_FLAGS) | (fl & POSIX_CHANGEABLE_FLAGS);
}

void MemContextUser::FileDescriptorBuf::flush ()
{
	push_back_reset ();
	access_->flush ();
}

void MemContextUser::FileDescriptorChar::flush ()
{
	push_back_reset ();
}

bool MemContextUser::FileDescriptorBuf::isatty () const
{
	return false;
}

bool MemContextUser::FileDescriptorChar::isatty () const
{
	return true;
}

}
}
