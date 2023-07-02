#include <Nirvana/Nirvana.h>
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
	Object::_ref_type obj = naming_service_->resolve_str ("\\//var");
	ASSERT_TRUE (obj);
	Dir::_ref_type dir = Dir::_narrow (obj);
	ASSERT_TRUE (dir);
	BindingIterator::_ref_type it;
	BindingList bindings;
	dir->list (10, bindings, it);
	EXPECT_FALSE (it);
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
	Object::_ref_type obj = naming_service_->resolve_str ("\\//var/tmp");
	ASSERT_TRUE (obj);
	Dir::_ref_type dir = Dir::_narrow (obj);
	ASSERT_TRUE (dir);
	BindingIterator::_ref_type it;
	BindingList bindings;
	dir->list (0, bindings, it);
	if (it) {
		while (it->next_n (10, bindings)) {
			for (const auto& b : bindings) {
				ASSERT_FALSE (b.binding_name ().empty ());
				const NameComponent& nc = b.binding_name ().front ();
				EXPECT_FALSE (nc.id ().empty () && nc.kind ().empty ());
			}
		}
	}
}

TEST_F (TestFile, Mnt)
{
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
		} catch (const CORBA::SystemException& ex) {
			EXPECT_TRUE (CORBA::NO_PERMISSION::_downcast (&ex)) << ' ' << ex._name () << " Error: " << get_minor_errno (ex.minor ());
		}
		BindingList dir;
		try {
			drive->list (10, dir, it);
		} catch (const CORBA::SystemException& ex) {
			EXPECT_TRUE (CORBA::NO_PERMISSION::_downcast (&ex)) << ' ' << ex._name () << " Error: " << get_minor_errno (ex.minor ());
		}
	}
}

TEST_F (TestFile, Direct)
{
	char file_name [L_tmpnam_s];
	ASSERT_FALSE (tmpnam_s (file_name));

	uint_fast16_t flags = O_DIRECT | FILE_SHARE_DENY_WRITE;

	AccessDirect::_ref_type fa = AccessDirect::_narrow (
		g_system->open_file (file_name, O_CREAT | O_TRUNC | O_RDWR | flags, 0600)->_to_object ());

	ASSERT_TRUE (fa);
	EXPECT_EQ (fa->size (), 0);
	std::vector <uint8_t> wbuf;
	wbuf.resize (1, 1);
	fa->write (0, wbuf);
	fa->flush ();
	std::vector <uint8_t> rbuf;
	fa->read (0, 1, rbuf);
	EXPECT_EQ (rbuf, wbuf);

	File::_ref_type file = fa->file ();
	fa->close ();
	fa = nullptr;

	ASSERT_TRUE (file);
	fa = AccessDirect::_narrow (file->open (flags)->_to_object ());
	ASSERT_TRUE (fa);
	fa->read (0, 1, rbuf);
	EXPECT_EQ (rbuf, wbuf);
	fa->close ();
	fa = nullptr;

	g_system->remove (file_name);
}

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
	fa = AccessBuf::_downcast (file->open (flags)->_to_value ());
	ASSERT_TRUE (fa);
	EXPECT_EQ (fa->read (rbuf, 1), 1);
	EXPECT_EQ (rbuf [0], wbuf [0]);
	fa->close ();
	fa = nullptr;

	g_system->remove (file_name);
}

}
