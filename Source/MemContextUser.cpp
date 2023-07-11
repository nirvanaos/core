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
#include "MemContextUser.h"
#include "NameService/FileSystem.h"
#include "ORB/Services.h"
#include <fnctl.h>
#include "virtual_copy.h"

using namespace CosNaming;
using CORBA::Core::Services;

namespace Nirvana {
namespace Core {

class MemContextUser::FileDescriptorBuf : public FileDescriptorBase
{
public:
	FileDescriptorBuf (AccessBuf::_ref_type&& access) :
		access_ (std::move (access))
	{}

	virtual void close () const override;
	virtual size_t read (void* p, size_t size) const override;
	virtual void write (const void* p, size_t size) const override;
	virtual uint64_t seek (unsigned method, const int64_t& off) const override;

private:
	AccessBuf::_ref_type access_;
};

class MemContextUser::FileDescriptorChar : public FileDescriptorBase
{
public:
	FileDescriptorChar (AccessChar::_ref_type&& access) :
		access_ (std::move (access))
	{}

	virtual void close () const override;
	virtual size_t read (void* p, size_t size) const override;
	virtual void write (const void* p, size_t size) const override;
	virtual uint64_t seek (unsigned method, const int64_t& off) const override;

private:
	AccessChar::_ref_type access_;
};

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
	FileSystem::get_name_from_path (name, *ppath);
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
		throw RuntimeError (ENOTDIR);

	if (dir->_non_existent ())
		throw RuntimeError (ENOENT);

	current_dir_ = std::move (new_dir);
}

inline
unsigned MemContextUser::Data::fd_open (const IDL::String& path, uint_fast16_t flags, mode_t mode)
{
	// Open file access
	Name name = get_name_from_path (path);
	name.erase (name.begin ());
	Dir::_ref_type root = Dir::_narrow (name_service ()->resolve (Name ()));
	Access::_ref_type access = root->open (name, flags & ~O_DIRECT, mode);

	// Create file descriptor
	FileDescriptorRef pfd;
	CORBA::ValueBase::_ref_type vb = access->_to_value ();
	if (vb) {
		AccessBuf::_ptr_type ab = AccessBuf::_downcast (vb);
		if (ab)
			pfd = FileDescriptorRef::create <FileDescriptorBuf> (ab);
	} else {
		AccessChar::_ref_type ac = AccessChar::_narrow (access->_to_object ());
		if (ac)
			pfd = FileDescriptorRef::create <FileDescriptorChar> (std::move (ac));
	}

	if (!pfd)
		throw RuntimeError (EIO);

	// Allocate descriptor cell
	auto it = file_descriptors_.begin ();
	for (; it != file_descriptors_.end (); ++it) {
		if (!*it)
			break;
	}

	size_t fd = it - file_descriptors_.begin ();
	if (it != file_descriptors_.end ())
		*it = std::move (pfd);
	else
		file_descriptors_.push_back (std::move (pfd));

	return (unsigned)fd;
}

inline
void MemContextUser::Data::fd_close (unsigned fd)
{
	if (fd >= file_descriptors_.size ())
		throw RuntimeError (EBADF);
	auto it = file_descriptors_.begin () + fd;
	FileDescriptorRef pfd = std::move (*it);
	if (!pfd)
		throw RuntimeError (EBADF);
	if (pfd->_refcount_value () == 1)
		pfd->close ();
}

const MemContextUser::FileDescriptor& MemContextUser::Data::get_fd (unsigned fd) const
{
	if (fd >= file_descriptors_.size ())
		throw RuntimeError (EBADF);
	const FileDescriptorRef& pfd = file_descriptors_ [fd];
	if (!pfd)
		throw RuntimeError (EBADF);
	return *pfd;
}

inline
size_t MemContextUser::Data::fd_read (unsigned fd, void* p, size_t size) const
{
	return get_fd (fd).read (p, size);
}

inline
void MemContextUser::Data::fd_write (unsigned fd, const void* p, size_t size) const
{
	get_fd (fd).write (p, size);
}

inline
uint64_t MemContextUser::Data::fd_seek (unsigned fd, const int64_t& off, unsigned method) const
{
	return get_fd (fd).seek (method, off);
}

MemContextUser& MemContextUser::current ()
{
	MemContextUser* p = MemContext::current ().user_context ();
	if (!p)
		throw CORBA::NO_IMPLEMENT ();
	return *p;
}

MemContextUser::MemContextUser () :
	MemContext (true)
{}

MemContextUser::MemContextUser (Heap& heap) noexcept :
	MemContext (heap, true)
{}

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

unsigned MemContextUser::fd_open (const IDL::String& path, uint_fast16_t flags, mode_t mode)
{
	return data ().fd_open (path, flags, mode);
}

void MemContextUser::fd_close (unsigned fd)
{
	if (!data_)
		throw RuntimeError (EBADF);
	data_->fd_close (fd);
}

size_t MemContextUser::fd_read (unsigned fd, void* p, size_t size)
{
	return data ().fd_read (fd, p, size);
}

void MemContextUser::fd_write (unsigned fd, const void* p, size_t size)
{
	data ().fd_write (fd, p, size);
}

uint64_t MemContextUser::fd_seek (unsigned fd, const int64_t& off, unsigned method)
{
	return data ().fd_seek (fd, off, method);
}

void MemContextUser::FileDescriptorBuf::close () const
{
	access_->close ();
}

size_t MemContextUser::FileDescriptorBuf::read (void* p, size_t size) const
{
	return access_->read (p, size);
}

void MemContextUser::FileDescriptorBuf::write (const void* p, size_t size) const
{
	access_->write (p, size);
}

uint64_t MemContextUser::FileDescriptorBuf::seek (unsigned method, const int64_t& off) const
{
	return access_->seek ((SeekMethod)method, off);
}

void MemContextUser::FileDescriptorChar::close () const
{
	access_->close ();
}

size_t MemContextUser::FileDescriptorChar::read (void* p, size_t size) const
{
	if (sizeof (size_t) > 4 && size > std::numeric_limits <uint32_t>::max ())
		throw RuntimeError (ENXIO);
	IDL::String buf;
	access_->read ((uint32_t)size, buf);
	size_t readed = buf.size ();
	if (readed > size)
		throw RuntimeError (EIO);
	Port::Memory::copy (p, const_cast <char*> (buf.data ()), readed, Memory::SRC_DECOMMIT);
	return readed;
}

void MemContextUser::FileDescriptorChar::write (const void* p, size_t size) const
{
	access_->write (CORBA::Internal::StringView <char> ((const char*)p, size));
}

uint64_t MemContextUser::FileDescriptorChar::seek (unsigned method, const int64_t& off) const
{
	throw RuntimeError (ESPIPE);
}

}
}
