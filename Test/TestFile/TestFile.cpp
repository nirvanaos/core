#include <Nirvana/Nirvana.h>
#include <Nirvana/DirectoryIterator.h>
#include <fnctl.h>
#include <gtest/gtest.h>
#include <sys/stat.h>

using namespace Nirvana;
using namespace CORBA;
using namespace CosNaming;

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
	Object::_ref_type obj = naming_service_->resolve_str ("\\//var");
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
	Object::_ref_type obj = naming_service_->resolve_str ("\\//var/tmp");
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
	Object::_ref_type obj = naming_service_->resolve_str ("\\//mnt");
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
	Object::_ref_type obj = naming_service_->resolve_str ("\\//var/tmp");
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
	fa->write (0, wbuf);

	EXPECT_EQ (fa->size (), 1);

	// Flush buffer
	fa->flush ();

	// Read
	std::vector <uint8_t> rbuf;
	fa->read (0, 1, rbuf);
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
	fa->read (0, 1, rbuf);
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

TEST_F (TestFile, DirectoryIterator)
{
	// Obtain temporary directory object
	Object::_ref_type obj = naming_service_->resolve_str ("\\//home");
	ASSERT_TRUE (obj);
	Dir::_ref_type dir = Dir::_narrow (obj);
	ASSERT_TRUE (dir);

	DirectoryIterator iter (dir);
	while (const DirEntry* p = iter.readdir ()) {
		EXPECT_FALSE (p->name ().empty ());
	}
}

/*
TEST_F (TestFile, Buf)
{
	char file_name [L_tmpnam_s];
	ASSERT_FALSE (tmpnam_s (file_name));

	uint_fast16_t flags = FILE_SHARE_DENY_WRITE;

	AccessBuf::_ref_type fa = AccessBuf::_downcast (
		g_system->open_file (file_name, O_CREAT | O_TRUNC | O_RDWR | flags, 0600)->_to_value ());

	ASSERT_TRUE (fa);
	EXPECT_EQ (fa->size (), 0);
	uint8_t wbuf [16];
	wbuf [0] = 1;
	fa->write (wbuf, 1);
	fa->seek (SeekMethod::SM_BEG, 0);
	uint8_t rbuf [16];
	EXPECT_EQ (fa->read (rbuf, 1), 1);
	EXPECT_EQ (rbuf[0], wbuf[0]);

	File::_ref_type file = fa->file ();
	fa->close ();
	fa = nullptr;

	ASSERT_TRUE (file);
	fa = AccessBuf::_downcast (file->open (flags, 0)->_to_value ());
	ASSERT_TRUE (fa);
	EXPECT_EQ (fa->read (rbuf, 1), 1);
	EXPECT_EQ (rbuf [0], wbuf [0]);
	fa->close ();
	fa = nullptr;

	g_system->remove (file_name);
}
*/
}
