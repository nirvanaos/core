#include <Nirvana/Nirvana.h>
#include <Nirvana/fnctl.h>
#include <gtest/gtest.h>

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

TEST_F (TestFile, var)
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

/*
TEST_F (TestFile, Open)
{
	char file_name [L_tmpnam_s];
	ASSERT_FALSE (tmpnam_s (file_name));
	FileAccessDirect::_ref_type fa = Static <file_factory>::ptr ()->open (file_name, O_CREAT | O_TRUNC);

	EXPECT_EQ (fa->size (), 0);
	std::vector <uint8_t> wbuf;
	wbuf.resize (1, 1);
	fa->write (0, wbuf);
	fa->flush ();
	std::vector <uint8_t> rbuf;
	fa->read (0, 1, rbuf);
	EXPECT_EQ (rbuf, wbuf);
	fa = nullptr;
}
*/
}
