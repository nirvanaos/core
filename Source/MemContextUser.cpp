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
#include <Nirvana/Legacy/Legacy_Process.h>
#include <fnctl.h>
#include "virtual_copy.h"
#include "CharFileAdapter.h"

using namespace CosNaming;
using CORBA::Core::Services;

namespace Nirvana {
namespace Core {

const unsigned MemContextUser::POSIX_CHANGEABLE_FLAGS = O_APPEND | O_NONBLOCK;

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
	AccessBuf::_ref_type access_;
};

class MemContextUser::FileDescriptorChar : public FileDescriptorBase
{
public:
	FileDescriptorChar (AccessChar::_ref_type&& access) :
		access_ (std::move (access)),
		flags_ (access_->flags ())
	{
		if ((flags_ & O_ACCMODE) != O_WRONLY)
			adapter_ = CORBA::make_reference <CharFileAdapter> (AccessChar::_ptr_type (access_), (flags_ & O_NONBLOCK) != 0);
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
	AccessChar::_ref_type access_;
	Ref <CharFileAdapter> adapter_;
	unsigned flags_;
};

MemContextUser::Data* MemContextUser::Data::create ()
{
	return new Data;
}

MemContextUser::Data::~Data ()
{}

inline
MemContextUser::Data::Data (const InheritedFiles& inh)
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
		FileDescriptorRef fd = make_fd (f.access ());
		for (auto d : f.descriptors ()) {
			FileDescriptorInfo& fdr = get_fd (d);
			if (fdr.ref)
				throw_BAD_PARAM ();
			fdr.ref = fd;
		}
	}
}

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
			if (!it->ref)
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

inline
unsigned MemContextUser::Data::fd_add (Access::_ptr_type access)
{
	// Allocate file descriptor cell
	size_t i = alloc_fd ();
	file_descriptors_ [i].ref = make_fd (access);

	return (unsigned)(i + STD_CNT);
}

inline
void MemContextUser::Data::close (unsigned ifd)
{
	get_open_fd (ifd).close ();
	while (!file_descriptors_.empty () && !file_descriptors_.back ().ref) {
		file_descriptors_.pop_back ();
	}
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
	if (!fd.ref)
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

inline
size_t MemContextUser::Data::read (unsigned ifd, void* p, size_t size)
{
	return get_open_fd (ifd).ref->read (p, size);
}

inline
void MemContextUser::Data::write (unsigned ifd, const void* p, size_t size)
{
	get_open_fd (ifd).ref->write (p, size);
}

inline
uint64_t MemContextUser::Data::seek (unsigned ifd, const int64_t& off, unsigned method)
{
	return get_open_fd (ifd).ref->seek (method, off);
}

inline 
unsigned MemContextUser::Data::dup (unsigned ifd, unsigned start)
{
	const FileDescriptorInfo& src = get_open_fd (ifd);
	size_t i = alloc_fd (start);
	file_descriptors_ [i].ref = src.ref;
	return (unsigned)(i + STD_CNT);
}

inline
void MemContextUser::Data::dup2 (unsigned fd_src, unsigned fd_dst)
{
	const FileDescriptorInfo& src = get_open_fd (fd_src);
	FileDescriptorInfo& dst = get_fd (fd_dst);
	if (dst.ref)
		dst.close ();
	dst.ref = src.ref;
}

inline
unsigned MemContextUser::Data::fd_flags (unsigned ifd)
{
	return get_open_fd (ifd).fd_flags;
}

inline
void MemContextUser::Data::fd_flags (unsigned ifd, unsigned flags)
{
	if (flags & ~FD_CLOEXEC)
		throw_INV_FLAG (make_minor_errno (EINVAL));

	get_open_fd (ifd).fd_flags = flags;
}

inline
unsigned MemContextUser::Data::flags (unsigned ifd)
{
	return get_open_fd (ifd).ref->flags ();
}

inline
void MemContextUser::Data::flags (unsigned ifd, unsigned flags)
{
	return get_open_fd (ifd).ref->flags (flags);
}

inline
void MemContextUser::Data::flush (unsigned ifd)
{
	get_open_fd (ifd).ref->flush ();
}

inline
bool MemContextUser::Data::isatty (unsigned ifd)
{
	return get_open_fd (ifd).ref->isatty ();
}

inline
void MemContextUser::Data::push_back (unsigned ifd, int c)
{
	get_open_fd (ifd).ref->push_back (c);
}

MemContextUser& MemContextUser::current ()
{
	MemContextUser* p = MemContext::current ().user_context ();
	if (!p)
		throw_BAD_OPERATION (); // Unavailable for core contexts
	return *p;
}

MemContextUser::MemContextUser (Heap& heap, const InheritedFiles& inh) :
	MemContext (&heap, true)
{
	if (!inh.empty ())
		data_.reset (new Data (inh));
}

MemContextUser::~MemContextUser ()
{}

inline
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

void MemContextUser::on_object_construct (MemContextObject& obj) noexcept
{
	object_list_.push_back (obj);
}

void MemContextUser::on_object_destruct (MemContextObject& obj) noexcept
{
	// The object itself will remove from list. Nothing to do.
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

unsigned MemContextUser::fd_add (Access::_ptr_type access)
{
	return data ().fd_add (access);
}

void MemContextUser::close (unsigned fd)
{
	if (!data_)
		throw_BAD_PARAM (make_minor_errno (EBADF));
	data_->close (fd);
}

size_t MemContextUser::read (unsigned fd, void* p, size_t size)
{
	return data ().read (fd, p, size);
}

void MemContextUser::write (unsigned fd, const void* p, size_t size)
{
	data ().write (fd, p, size);
}

uint64_t MemContextUser::seek (unsigned fd, const int64_t& off, unsigned method)
{
	return data ().seek (fd, off, method);
}

unsigned MemContextUser::fcntl (unsigned ifd, int cmd, unsigned arg)
{
	switch (cmd) {
		case F_DUPFD:
			return data ().dup (ifd, arg);

		case F_GETFD:
			return data ().fd_flags (ifd);

		case F_SETFD:
			data ().fd_flags (ifd, arg);

		case F_GETFL:
			return data ().flags (ifd);

		case F_SETFL:
			data ().flags (ifd, arg);
	}
	throw_BAD_PARAM (make_minor_errno (EINVAL));
}

void MemContextUser::flush (unsigned ifd)
{
	data ().flush (ifd);
}

void MemContextUser::dup2 (unsigned src, unsigned dst)
{
	data ().dup2 (src, dst);
}

bool MemContextUser::isatty (unsigned ifd)
{
	return data ().isatty (ifd);
}

void MemContextUser::push_back (unsigned ifd, int c)
{
	data ().push_back (ifd, c);
}

void MemContextUser::FileDescriptorBuf::close () const
{
	access_->close ();
}

void MemContextUser::FileDescriptorChar::close () const
{
	if (adapter_)
		adapter_->disconnect_push_consumer ();
	access_->close ();
}

size_t MemContextUser::FileDescriptorBuf::read (void* p, size_t size)
{
	size_t cb = push_back_read (p, size);
	if (size) {
		Nirvana::AccessBuf::_ref_type hold = access_;
		cb += hold->read (p, size);
	}
	return cb;
}

size_t MemContextUser::FileDescriptorChar::read (void* p, size_t size)
{
	if (!adapter_)
		throw_NO_PERMISSION (make_minor_errno (EBADF));
	size_t cb = push_back_read (p, size);
	if (size) {
		Ref <CharFileAdapter> hold = adapter_;
		cb += hold->read (p, size);
	}
	return cb;
}

void MemContextUser::FileDescriptorBuf::write (const void* p, size_t size)
{
	push_back_reset ();
	Nirvana::AccessBuf::_ref_type hold = access_;
	hold->write (p, size);
}

void MemContextUser::FileDescriptorChar::write (const void* p, size_t size)
{
	push_back_reset ();
	Nirvana::AccessChar::_ref_type hold = access_;
	hold->write (CORBA::Internal::StringView <char> ((const char*)p, size));
}

uint64_t MemContextUser::FileDescriptorBuf::seek (unsigned method, const int64_t& off)
{
	FileSize pos;
	Nirvana::AccessBuf::_ref_type hold = access_;
	switch (method) {
	case SEEK_SET:
		pos = 0;
		break;

	case SEEK_CUR:
		pos = hold->position ();
		break;

	case SEEK_END:
		pos = hold->size ();
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

	hold->position (pos += off);
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
	access_->flush ();
}

void MemContextUser::FileDescriptorChar::flush ()
{
	throw_BAD_OPERATION (make_minor_errno (EINVAL));
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
