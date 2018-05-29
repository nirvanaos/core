// Testing Windows API memory management functions

#include <windows.h>
#include <gtest/gtest.h>

#define PAGE_SIZE 4096
#define LINE_SIZE (16 * PAGE_SIZE)

namespace unittests {

typedef BOOL (__stdcall* HandlesEqual) (HANDLE h0, HANDLE h1);
HandlesEqual fn_handles_equal = (HandlesEqual)GetProcAddress (LoadLibraryW (L"Kernelbase.dll"), "CompareObjectHandles");

BOOL handles_equal (HANDLE h0, HANDLE h1)
{
	return (*fn_handles_equal) (h0, h1);
}

class TestAPI :
	public ::testing::Test
{
protected:
	TestAPI ()
	{}

	virtual ~TestAPI ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
		ASSERT_TRUE (fn_handles_equal);
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
	}
};

TEST (TestAPI, MappingHandle)
{
	HANDLE mh = CreateFileMapping (0, 0, PAGE_READWRITE | SEC_RESERVE, 0, LINE_SIZE, 0);
	ASSERT_TRUE (mh);
	HANDLE mh1;
	HANDLE process = GetCurrentProcess ();
	EXPECT_TRUE (DuplicateHandle (process, mh, process, &mh1, 0, FALSE, DUPLICATE_SAME_ACCESS));
	EXPECT_TRUE (handles_equal (mh, mh1));
	EXPECT_TRUE (CloseHandle (mh1));
	EXPECT_TRUE (CloseHandle (mh));
}

TEST (TestAPI, Allocate)
{
	SYSTEM_INFO si;
	GetSystemInfo (&si);
	EXPECT_EQ (si.dwAllocationGranularity, LINE_SIZE);

	HANDLE mh = CreateFileMapping (0, 0, PAGE_READWRITE | SEC_RESERVE, 0, LINE_SIZE, 0);
	ASSERT_TRUE (mh);
	char* p = (char*)MapViewOfFile (mh, FILE_MAP_ALL_ACCESS, 0, 0, LINE_SIZE);
	ASSERT_TRUE (p);

	// Commit 2 pages.
	EXPECT_TRUE (VirtualAlloc (p, PAGE_SIZE * 2, MEM_COMMIT, PAGE_READWRITE));
	
	// Write to 2 pages.
	EXPECT_NO_THROW (p [0] = 1);
	EXPECT_NO_THROW (p [PAGE_SIZE] = 2);
	
	// Decommit first page. We can't use VirtualFree with mapped memory.

	DWORD old;
	EXPECT_TRUE (VirtualProtect (p, PAGE_SIZE, PAGE_NOACCESS, &old));
	EXPECT_TRUE (VirtualAlloc (p, PAGE_SIZE, MEM_RESET, PAGE_NOACCESS));
	/*
	OfferVirtualMemory (p, PAGE_SIZE, VmOfferPriorityVeryLow);
	*/

	char x;
	EXPECT_ANY_THROW (x = p [0]);

	// Recommit first page
	EXPECT_TRUE (VirtualAlloc (p, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE));
	EXPECT_NO_THROW (x = p [0]);

	EXPECT_TRUE (UnmapViewOfFile (p));
	EXPECT_TRUE (CloseHandle (mh));
}

TEST (TestAPI, Sharing)
{
	HANDLE mh = CreateFileMapping (0, 0, PAGE_READWRITE | SEC_RESERVE, 0, LINE_SIZE, 0);
	ASSERT_TRUE (mh);
	char* p = (char*)MapViewOfFile (mh, FILE_MAP_ALL_ACCESS, 0, 0, LINE_SIZE);
	ASSERT_TRUE (p);
	EXPECT_TRUE (VirtualAlloc (p, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE));
	strcpy (p, "test");

	HANDLE process = GetCurrentProcess ();

	char* copies [10];
	HANDLE handles [10];

	memset (copies, 0, sizeof (copies));
	memset (handles, 0, sizeof (copies));
	for (int i = 0; i < _countof (copies); ++i) {
		HANDLE mh1;
		ASSERT_TRUE (DuplicateHandle (process, mh, process, &mh1, 0, FALSE, DUPLICATE_SAME_ACCESS));
		handles [i] = mh1;
		char* p = (char*)MapViewOfFile (mh1, FILE_MAP_ALL_ACCESS, 0, 0, LINE_SIZE);
		EXPECT_TRUE (p);
		copies [i] = p;
		if (p) {
			EXPECT_TRUE (VirtualAlloc (p, PAGE_SIZE, MEM_COMMIT, PAGE_READONLY));
			EXPECT_STREQ (p, "test");
			EXPECT_ANY_THROW (strcpy (p, "test1"));
		}
	}

	for (int i = 0; i < _countof (copies); ++i) {
		char* p = copies [i];
		copies [i] = 0;
		if (p)
			EXPECT_TRUE (UnmapViewOfFile (p));
		HANDLE h = handles [i];
		handles [i] = 0;
		if (h)
			EXPECT_TRUE (CloseHandle (h));
	}

	for (int i = 0; i < _countof (copies); ++i) {
		HANDLE mh1;
		ASSERT_TRUE (DuplicateHandle (process, mh, process, &mh1, 0, FALSE, DUPLICATE_SAME_ACCESS));
		handles [i] = mh1;
		char* p = (char*)MapViewOfFile (mh1, FILE_MAP_ALL_ACCESS, 0, 0, LINE_SIZE);
		EXPECT_TRUE (p);
		copies [i] = p;
		if (p) {
			EXPECT_TRUE (VirtualAlloc (p, PAGE_SIZE, MEM_COMMIT, PAGE_WRITECOPY));
			EXPECT_STREQ (p, "test");

			char buf [16];
			_itoa (i, buf, 10);
			EXPECT_NO_THROW (strcpy (p + 4, buf));
		}
	}

	for (int i = 0; i < _countof (copies); ++i) {
		char* p = copies [i];
		char buf [16] = "test";
		_itoa (i, buf + 4, 10);
		EXPECT_STREQ (p, buf);
	}

	for (int i = 0; i < _countof (copies); ++i) {
		char* p = copies [i];
		copies [i] = 0;
		if (p)
			EXPECT_TRUE (UnmapViewOfFile (p));
		HANDLE h = handles [i];
		handles [i] = 0;
		if (h)
			EXPECT_TRUE (CloseHandle (h));
	}

	EXPECT_STREQ (p, "test");
	EXPECT_TRUE (UnmapViewOfFile (p));
	EXPECT_TRUE (CloseHandle (mh));
}

}
