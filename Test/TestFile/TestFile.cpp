/*
* Nirvana test suite.
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
#include <Nirvana/Nirvana.h>
#include <Nirvana/DirectoryIterator.h>
#include <Nirvana/System.h>
#include <fnctl.h>
#include <gtest/gtest.h>

using namespace Nirvana;
using namespace CORBA;
using namespace CosNaming;

// Test for filesystem objects.
namespace TestFile {

class TestFile :
	public ::testing::Test
{
protected:
	TestFile ()
	{}

	virtual ~TestFile ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
		naming_service_ = NamingContextExt::_narrow (g_ORB->resolve_initial_references ("NameService"));
		ASSERT_TRUE (naming_service_);
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
	}

protected:
	NamingContextExt::_ref_type naming_service_;
};

TEST_F (TestFile, Var)
{
	// Obtain "var" directory object
	Object::_ref_type obj = naming_service_->resolve_str ("/var");
	ASSERT_TRUE (obj);
	Dir::_ref_type dir = Dir::_narrow (obj);
	ASSERT_TRUE (dir);

	// Obtain all subdirectories
	BindingIterator::_ref_type it;
	BindingList bindings;
	dir->list (10, bindings, it);
	EXPECT_FALSE (it);

	// "var" must contain "log" and "tmp"
	bool log = false, tmp = false;
	for (const auto& b : bindings) {
		ASSERT_FALSE (b.binding_name ().empty ());
		EXPECT_EQ (b.binding_name ().size (), 1);
		EXPECT_EQ (b.binding_type (), BindingType::ncontext);
		const NameComponent& nc = b.binding_name ().front ();
		EXPECT_TRUE (nc.kind ().empty ());
		if (nc.id () == "log")
			log = true;
		else if (nc.id () == "tmp")
			tmp = true;
	}
	EXPECT_TRUE (log);
	EXPECT_TRUE (tmp);
}

TEST_F (TestFile, TmpIterator)
{
	// Obtain temporary directory object
	Object::_ref_type obj = naming_service_->resolve_str ("/var/tmp");
	ASSERT_TRUE (obj);
	Dir::_ref_type dir = Dir::_narrow (obj);
	ASSERT_TRUE (dir);

	// Iterate
	BindingIterator::_ref_type it;
	BindingList bindings;
	dir->list (0, bindings, it);
	if (it) {
		while (it->next_n (10, bindings)) {
			for (const auto& b : bindings) {
				ASSERT_EQ (b.binding_name ().size (), 1);
				const NameComponent& nc = b.binding_name ().front ();
				EXPECT_FALSE (nc.id ().empty () && nc.kind ().empty ());
			}
		}
	}
}

TEST_F (TestFile, Mnt)
{
	// Iterate all drives in "mnt"
	Object::_ref_type obj = naming_service_->resolve_str ("/mnt");
	ASSERT_TRUE (obj);
	Dir::_ref_type mnt = Dir::_narrow (obj);
	ASSERT_TRUE (mnt);

	BindingIterator::_ref_type it;
	BindingList drives;
	mnt->list (std::numeric_limits <uint32_t>::max (), drives, it);
	EXPECT_FALSE (it);
	for (const auto& b : drives) {
		ASSERT_EQ (b.binding_name ().size (), 1);
		EXPECT_EQ (b.binding_type (), BindingType::ncontext);
		const NameComponent& nc = b.binding_name ().front ();
		EXPECT_FALSE (nc.id ().empty ());
		EXPECT_TRUE (nc.kind ().empty ());
		Dir::_ref_type drive;
		try {
			drive = Dir::_narrow (mnt->resolve (b.binding_name ()));
			ASSERT_TRUE (drive);
		} catch (const CORBA::NO_PERMISSION&) {
		} catch (const CORBA::SystemException& ex) {
			ADD_FAILURE () << ex._name ();
		}
		BindingList dir;
		try {
			drive->list (10, dir, it);
		} catch (const CORBA::NO_PERMISSION&) {
		} catch (const CORBA::INTERNAL& ex) {
			EXPECT_EQ (get_minor_errno (ex.minor ()), EBUSY);
		} catch (const CORBA::SystemException& ex) {
			ADD_FAILURE () << ex._name ();
		}
	}
}

TEST_F (TestFile, Direct)
{
	// Obtain temporary directory object
	Object::_ref_type obj = naming_service_->resolve_str ("/var/tmp");
	ASSERT_TRUE (obj);
	Dir::_ref_type tmp_dir = Dir::_narrow (obj);
	ASSERT_TRUE (tmp_dir);

	// Create temporary file
	const char PATTERN [] = "XXXXXX.tmp";
	std::string file_name = PATTERN;
	AccessDirect::_ref_type fa = AccessDirect::_narrow (
		tmp_dir->mkostemps (file_name, 4, O_DIRECT)->_to_object ());
	ASSERT_TRUE (fa);
	EXPECT_NE (file_name, PATTERN);

	EXPECT_EQ (fa->size (), 0);

	// Write
	std::vector <uint8_t> wbuf;
	wbuf.resize (1, 1);
	fa->write (0, wbuf, FileLock (), false);

	EXPECT_EQ (fa->size (), 1);

	// Flush buffer
	fa->flush ();

	// Read
	std::vector <uint8_t> rbuf;
	fa->read (FileLock (), 0, 1, LockType::LOCK_NONE, true, rbuf);
	EXPECT_EQ (rbuf, wbuf);

	// Obtain file object
	File::_ref_type file = fa->file ();

	// Close file access
	fa->close ();
	fa = nullptr;

	// Open file for reading
	ASSERT_TRUE (file);
	fa = AccessDirect::_narrow (file->open (O_RDONLY | O_DIRECT, 0)->_to_object ());
	ASSERT_TRUE (fa);
	fa->read (FileLock (), 0, 1, LockType::LOCK_NONE, true, rbuf);
	EXPECT_EQ (rbuf, wbuf);

	// Close
	fa->close ();
	fa = nullptr;

	// Open buffered access
	AccessBuf::_ref_type fb = AccessBuf::_downcast (file->open (O_RDONLY, 0)->_to_value ());
	ASSERT_TRUE (fb);

	// Read from buffer
	rbuf.resize (1);
	EXPECT_EQ (fb->read (rbuf.data (), 1), 1);
	EXPECT_EQ (rbuf, wbuf);

	// Close
	fb->close ();
	fb = nullptr;

	// Remove file
	file->remove ();
	EXPECT_TRUE (file->_non_existent ());
}

TEST_F (TestFile, Buf)
{
	// Obtain temporary directory object
	Object::_ref_type obj = naming_service_->resolve_str ("/var/tmp");
	ASSERT_TRUE (obj);
	Dir::_ref_type tmp_dir = Dir::_narrow (obj);
	ASSERT_TRUE (tmp_dir);

	// Create temporary file
	const char PATTERN [] = "XXXXXX.tmp";
	std::string file_name = PATTERN;
	AccessBuf::_ref_type fa = AccessBuf::_downcast (
		tmp_dir->mkostemps (file_name, 4, 0)->_to_value ());
	ASSERT_TRUE (fa);
	EXPECT_NE (file_name, PATTERN);

	EXPECT_EQ (fa->size (), 0);

	uint8_t wbuf [16];
	wbuf [0] = 1;
	fa->write (wbuf, 1);
	EXPECT_EQ (fa->position (), 1);
	fa->position (0);
	EXPECT_EQ (fa->position (), 0);
	uint8_t rbuf [16];
	EXPECT_EQ (fa->read (rbuf, 1), 1);
	EXPECT_EQ (rbuf [0], wbuf [0]);

	File::_ref_type file = fa->file ();
	fa->close ();
	fa = nullptr;

	ASSERT_TRUE (file);
	fa = AccessBuf::_downcast (file->open (O_RDONLY, 0)->_to_value ());
	ASSERT_TRUE (fa);
	EXPECT_EQ (fa->size (), 1);
	EXPECT_EQ (fa->read (rbuf, 1), 1);
	EXPECT_EQ (rbuf [0], wbuf [0]);
	fa->close ();
	fa = nullptr;

	// Remove file
	file->remove ();
	EXPECT_TRUE (file->_non_existent ());
}

TEST_F (TestFile, DirectoryIterator)
{
	// Obtain directory object
	Object::_ref_type obj = naming_service_->resolve_str ("/home");
	ASSERT_TRUE (obj);
	Dir::_ref_type dir = Dir::_narrow (obj);
	ASSERT_TRUE (dir);

	DirectoryIterator iter (dir);
	while (const DirEntry* p = iter.readdir ()) {
		EXPECT_FALSE (p->name ().id ().empty () && p->name ().kind ().empty ());
	}
}

void clear_directory (Dir::_ptr_type dir)
{
	ASSERT_TRUE (dir);
	DirectoryIterator iter (dir);
	while (const DirEntry* p = iter.readdir ()) {
		Name name (1, p->name ());
		if (FileType::directory == (FileType)p->st ().type ())
			clear_directory (Dir::_narrow (dir->resolve (name)));
		dir->unbind (name);
	}
}

TEST_F (TestFile, Directory)
{
	// Make temporary directory
	Name tmp_dir_name;
	g_system->get_name_from_path (tmp_dir_name, "/tmp/test");
	Dir::_ref_type tmp_dir;
	try {
		tmp_dir = Dir::_narrow (naming_service_->bind_new_context (tmp_dir_name));
	} catch (const NamingContext::AlreadyBound&) {
	}
	if (!tmp_dir) {
		tmp_dir = Dir::_narrow (naming_service_->resolve (tmp_dir_name));
		ASSERT_TRUE (tmp_dir);
		clear_directory (tmp_dir);
	}

	// Create and close temporary file
	std::string tmp_file = "XXXXXX.tmp";
	tmp_dir->mkostemps (tmp_file, 4, 0)->close ();

	// Try to remove directory that is not empty
	bool thrown = false;
	try {
		naming_service_->unbind (tmp_dir_name);
	} catch (const SystemException& ex) {
		EXPECT_TRUE (INTERNAL::_downcast (&ex)) << ex._name ();
		EXPECT_EQ (get_minor_errno (ex.minor ()), ENOTEMPTY);
		thrown = true;
	}
	EXPECT_TRUE (thrown);

	// Delete file
	Name tmp_file_name;
	g_system->get_name_from_path (tmp_file_name, tmp_file);

	try {
		tmp_dir->unbind (tmp_file_name);
	} catch (const SystemException& ex) {
		ADD_FAILURE () << ex._name () << ' ' << get_minor_errno (ex.minor ());
	}

	// Remove empty directory
	EXPECT_NO_THROW (naming_service_->unbind (tmp_dir_name));

	EXPECT_TRUE (tmp_dir->_non_existent ());
}

}
