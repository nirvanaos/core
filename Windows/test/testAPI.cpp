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

TEST_F (TestAPI, MappingHandle)
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

TEST_F (TestAPI, Allocate)
{
	SYSTEM_INFO si;
	GetSystemInfo (&si);
	EXPECT_EQ (si.dwAllocationGranularity, LINE_SIZE);
	EXPECT_EQ (((size_t)si.lpMaximumApplicationAddress + 1) % LINE_SIZE, 0);

	HANDLE mh = CreateFileMapping (0, 0, PAGE_READWRITE | SEC_RESERVE, 0, LINE_SIZE, 0);
	ASSERT_TRUE (mh);
	char* p = (char*)MapViewOfFile (mh, FILE_MAP_ALL_ACCESS, 0, 0, LINE_SIZE);
	ASSERT_TRUE (p);

	// Commit 2 pages.
	EXPECT_TRUE (VirtualAlloc (p, PAGE_SIZE * 2, MEM_COMMIT, PAGE_READWRITE));

	EXPECT_FALSE (VirtualAlloc (p, LINE_SIZE, MEM_RESERVE, PAGE_READWRITE));

	DWORD err = GetLastError ();
	EXPECT_EQ (err, ERROR_INVALID_ADDRESS);

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

TEST_F (TestAPI, Sharing)
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

// SparseMapping on 64 bit system over 10 times faster than SharedMapping
TEST_F (TestAPI, SparseMapping)
{
	WCHAR dir [MAX_PATH + 1];
	ASSERT_TRUE (GetCurrentDirectoryW (_countof (dir), dir));

	HANDLE file = CreateFileW (L"mapping.tmp", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0,
														 CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, 0);
	ASSERT_NE (file, INVALID_HANDLE_VALUE);

	{
		FILE_SET_SPARSE_BUFFER sb;
		sb.SetSparse = TRUE;
		DWORD cb;
		EXPECT_TRUE (DeviceIoControl (file, FSCTL_SET_SPARSE, &sb, sizeof (sb), 0, 0, &cb, 0));
	}

	HANDLE mapping;
	HANDLE* table;
	{
		SYSTEM_INFO si;
		GetSystemInfo (&si);
		size_t size = ((size_t)si.lpMaximumApplicationAddress + LINE_SIZE) / LINE_SIZE * sizeof (HANDLE);

		FILE_ZERO_DATA_INFORMATION zdi;
		zdi.FileOffset.QuadPart = 0;
		zdi.BeyondFinalZero.QuadPart = size;
		DWORD cb;
		EXPECT_TRUE (DeviceIoControl (file, FSCTL_SET_ZERO_DATA, &zdi, sizeof (zdi), 0, 0, &cb, 0));
		mapping = CreateFileMapping (file, 0, PAGE_READWRITE, zdi.BeyondFinalZero.HighPart, zdi.BeyondFinalZero.LowPart, 0);
		EXPECT_TRUE (mapping);

		table = (HANDLE*)MapViewOfFile (mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		EXPECT_TRUE (table);
	}

	HANDLE sfile = CreateFileW (L"mapping.tmp", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0,
															OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, 0);
	EXPECT_NE (sfile, INVALID_HANDLE_VALUE);

	LARGE_INTEGER fsize;
	EXPECT_TRUE (GetFileSizeEx (sfile, &fsize));

	HANDLE smapping = CreateFileMapping (sfile, 0, PAGE_READWRITE, fsize.HighPart, fsize.LowPart, 0);
	EXPECT_TRUE (smapping);
	HANDLE* stable = (HANDLE*)MapViewOfFile (smapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	table [LINE_SIZE / sizeof (HANDLE)] = INVALID_HANDLE_VALUE;
	EXPECT_EQ (stable [LINE_SIZE / sizeof (HANDLE)], INVALID_HANDLE_VALUE);

	EXPECT_TRUE (UnmapViewOfFile (stable));
	EXPECT_TRUE (CloseHandle (smapping));
	EXPECT_TRUE (CloseHandle (sfile));

	EXPECT_TRUE (UnmapViewOfFile (table));
	EXPECT_TRUE (CloseHandle (mapping));
	EXPECT_TRUE (CloseHandle (file));
}

// Best directory implementation for 32-bit system.
TEST_F (TestAPI, SharedMapping)
{
	SYSTEM_INFO si;
	GetSystemInfo (&si);
	LARGE_INTEGER size;
	size.QuadPart = ((size_t)si.lpMaximumApplicationAddress + LINE_SIZE) / LINE_SIZE * sizeof (HANDLE);

	HANDLE mapping = CreateFileMappingW (INVALID_HANDLE_VALUE, 0, PAGE_READWRITE | SEC_RESERVE, size.HighPart, size.LowPart, L"NirvanaMapping");
	ASSERT_TRUE (mapping);
	HANDLE* table = (HANDLE*)MapViewOfFile (mapping, FILE_MAP_ALL_ACCESS, 0, 0, (SIZE_T)size.QuadPart);
	EXPECT_TRUE (table);

	HANDLE smapping = OpenFileMappingW (FILE_MAP_ALL_ACCESS, FALSE, L"NirvanaMapping");
	ASSERT_TRUE (smapping);

	HANDLE* stable = (HANDLE*)MapViewOfFile (smapping, FILE_MAP_ALL_ACCESS, 0, 0, (SIZE_T)size.QuadPart);
	EXPECT_TRUE (stable);

	EXPECT_TRUE (VirtualAlloc (table + LINE_SIZE / sizeof (HANDLE), PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE));
	table [LINE_SIZE / sizeof (HANDLE)] = INVALID_HANDLE_VALUE;

	EXPECT_TRUE (VirtualAlloc (stable + LINE_SIZE / sizeof (HANDLE), PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE));
	EXPECT_EQ (stable [LINE_SIZE / sizeof (HANDLE)], INVALID_HANDLE_VALUE);

	EXPECT_TRUE (UnmapViewOfFile (stable));
	EXPECT_TRUE (CloseHandle (smapping));

	EXPECT_TRUE (UnmapViewOfFile (table));
	EXPECT_TRUE (CloseHandle (mapping));
}

// Best directory implementation for 64-bit system.
TEST_F (TestAPI, SharedMapping2)
{
	SYSTEM_INFO si;
	GetSystemInfo (&si);
	LARGE_INTEGER size;
	size.QuadPart = ((size_t)si.lpMaximumApplicationAddress + LINE_SIZE) / LINE_SIZE * sizeof (HANDLE);

	HANDLE mapping = CreateFileMappingW (INVALID_HANDLE_VALUE, 0, PAGE_READWRITE | SEC_RESERVE, size.HighPart, size.LowPart, L"NirvanaMapping");
	ASSERT_TRUE (mapping);

	static const size_t SECOND_LEVEL_SIZE = LINE_SIZE / sizeof (HANDLE);

	size_t root_size = ((size_t)size.QuadPart + LINE_SIZE - 1) / LINE_SIZE;

	HANDLE** root_dir = (HANDLE**)VirtualAlloc (0, root_size, MEM_RESERVE, PAGE_READWRITE);
	ASSERT_TRUE (root_dir);

	size_t idx = LINE_SIZE / sizeof (HANDLE);
	size_t i1 = idx / SECOND_LEVEL_SIZE;
	size_t i2 = idx % SECOND_LEVEL_SIZE;

	ASSERT_TRUE (VirtualAlloc (root_dir + i1, sizeof (HANDLE**), MEM_COMMIT, PAGE_READWRITE));

	LARGE_INTEGER offset;
	offset.QuadPart = LINE_SIZE * i1;
	EXPECT_TRUE (root_dir [i1] = (HANDLE*)MapViewOfFile (mapping, FILE_MAP_ALL_ACCESS, offset.HighPart, offset.LowPart, LINE_SIZE));
	ASSERT_TRUE (VirtualAlloc (root_dir[i1] + i2, sizeof (HANDLE), MEM_COMMIT, PAGE_READWRITE));

	HANDLE smapping = OpenFileMappingW (FILE_MAP_ALL_ACCESS, FALSE, L"NirvanaMapping");
	ASSERT_TRUE (smapping);

	HANDLE** sroot_dir = (HANDLE**)VirtualAlloc (0, root_size, MEM_RESERVE, PAGE_READWRITE);
	ASSERT_TRUE (sroot_dir);

	ASSERT_TRUE (VirtualAlloc (sroot_dir + i1, sizeof (HANDLE**), MEM_COMMIT, PAGE_READWRITE));

	EXPECT_TRUE (sroot_dir [i1] = (HANDLE*)MapViewOfFile (smapping, FILE_MAP_ALL_ACCESS, offset.HighPart, offset.LowPart, LINE_SIZE));
	ASSERT_TRUE (VirtualAlloc (sroot_dir [i1] + i2, sizeof (HANDLE), MEM_COMMIT, PAGE_READWRITE));

	root_dir [i1][i2] = INVALID_HANDLE_VALUE;
	EXPECT_EQ (sroot_dir [i1][i2], INVALID_HANDLE_VALUE);

	EXPECT_TRUE (UnmapViewOfFile (sroot_dir [i1]));
	EXPECT_TRUE (CloseHandle (smapping));

	EXPECT_TRUE (UnmapViewOfFile (root_dir [i1]));
	EXPECT_TRUE (CloseHandle (mapping));
}

}
