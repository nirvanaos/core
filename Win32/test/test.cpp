#include <pidefs.h>

#define PAGE_SIZE 4096
#define LINE_SIZE (16 * PAGE_SIZE)

void test1 ()
{
  BYTE* p = (BYTE*)VirtualAlloc (0, LINE_SIZE, MEM_RESERVE, PAGE_NOACCESS);
  
  if (p) {
    HANDLE hm = CreateFileMapping (INVALID_HANDLE_VALUE, 0, PAGE_READWRITE | SEC_RESERVE, 0, 16 * PAGE_SIZE, 0);
    VirtualFree (p, 0, MEM_RELEASE);
    if (hm) {
      if (MapViewOfFileEx (hm, FILE_MAP_WRITE, 0, 0, 16 * PAGE_SIZE, p)) {
        if (VirtualAlloc (p + PAGE_SIZE, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE)) {
          p [5000] = 111;
          DWORD prot;
          if (VirtualProtect (p + PAGE_SIZE, PAGE_SIZE, PAGE_EXECUTE_WRITECOPY, &prot)) {
          BYTE* p1 = (BYTE*)VirtualAlloc (0, 16 * PAGE_SIZE, MEM_RESERVE, PAGE_NOACCESS);
            if (p1) {
              VirtualFree (p1, 0, MEM_RELEASE);
              if (MapViewOfFileEx (hm, FILE_MAP_WRITE, 0, 0, 16 * PAGE_SIZE, p1)) {
                if (VirtualAlloc (p1 + PAGE_SIZE, PAGE_SIZE, MEM_COMMIT, PAGE_EXECUTE_WRITECOPY)) {
                  MEMORY_BASIC_INFORMATION mbi;
                  
                  VirtualQuery (p + PAGE_SIZE, &mbi, sizeof (mbi));
                  VirtualQuery (p1 + PAGE_SIZE, &mbi, sizeof (mbi));
                  
                  ASSERT (p [5000] == 111);
                  ASSERT (p1 [5000] == 111);
                  p [5000] = 112;
                  ASSERT (p1 [5000] == 111);
                  p1 [5000] = 113;
                  ASSERT (p [5000] == 112);
                  
                  VirtualQuery (p + PAGE_SIZE, &mbi, PAGE_SIZE);
                  VirtualQuery (p1 + PAGE_SIZE, &mbi, PAGE_SIZE);
                }
              }
            }
          }
        }
        UnmapViewOfFile (p);
      }
      CloseHandle (hm);
    }
  }
}

void test2 ()
{
  HANDLE hm = CreateFileMapping (INVALID_HANDLE_VALUE, 0, 
    PAGE_READWRITE | SEC_RESERVE, 0, LINE_SIZE, TEXT ("test"));

  BYTE* p = (BYTE*)MapViewOfFile (hm, FILE_MAP_COPY, 0, 0, LINE_SIZE);
  VirtualAlloc (p, LINE_SIZE, MEM_COMMIT, PAGE_WRITECOPY);
  p [0] = '\x11';

  HANDLE hm1 = OpenFileMapping (FILE_MAP_COPY, FALSE, TEXT ("test"));

  BYTE* p1 = (BYTE*)MapViewOfFile (hm1, FILE_MAP_COPY, 0, 0, LINE_SIZE);
  VirtualAlloc (p1, LINE_SIZE, MEM_COMMIT, PAGE_WRITECOPY);
  
  ASSERT ('\x11' == p1 [0]);
  
  p1 [0] = '\x22';

  ASSERT ('\x11' == p [0]);

  p [0] = '\x33';
  
  ASSERT ('\x22' == p1 [0]);
  
  p [PAGE_SIZE] = '\x44';
  
  ASSERT ('\0' == p1 [4096]);
}

void test3 ()
{
  // Test for stack sharing

  HANDLE hm = CreateFileMapping (INVALID_HANDLE_VALUE, 0, 
    PAGE_READWRITE | SEC_RESERVE, 0, LINE_SIZE, 0);

  void* p = MapViewOfFile (hm, FILE_MAP_WRITE, 0, 0, LINE_SIZE);
  VirtualAlloc (p, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
  DWORD old;
  VirtualProtect (p, PAGE_SIZE, PAGE_EXECUTE_WRITECOPY, &old);
  VirtualAlloc (p, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);

  MEMORY_BASIC_INFORMATION mbi;
  VirtualQuery (p, &mbi, sizeof (mbi));

  ASSERT (PAGE_EXECUTE_WRITECOPY == mbi.Protect);
}

void test4 ()
{
  // Decommit / Recommit

  MEMORY_BASIC_INFORMATION mbi;

  // Reserve line
  void* p = VirtualAlloc (0, LINE_SIZE, MEM_RESERVE, PAGE_NOACCESS);
  VirtualQuery (p, &mbi, sizeof (mbi));

  // Reserve mapping
  HANDLE hm = CreateFileMapping (INVALID_HANDLE_VALUE, 0, 
    PAGE_READWRITE | SEC_RESERVE, 0, LINE_SIZE, 0);
  
  // Map
  VirtualFree (p, 0, MEM_RELEASE);
  MapViewOfFileEx (hm, FILE_MAP_WRITE, 0, 0, LINE_SIZE, p);
  VirtualQuery (p, &mbi, sizeof (mbi));

  // Commit two pages
  VirtualAlloc (p, PAGE_SIZE, MEM_COMMIT, PAGE_WRITECOPY);
  VirtualAlloc ((BYTE*)p + PAGE_SIZE * 2, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
  VirtualQuery (p, &mbi, sizeof (mbi));
  ASSERT (PAGE_WRITECOPY == mbi.Protect);
  VirtualQuery ((BYTE*)p + PAGE_SIZE, &mbi, sizeof (mbi));
  VirtualQuery ((BYTE*)p + PAGE_SIZE * 2, &mbi, sizeof (mbi));
  ASSERT (PAGE_READWRITE == mbi.Protect);
  
  // Write
  *((BYTE*)p) = 1;
  VirtualQuery (p, &mbi, sizeof (mbi));
  
  // Decommit
  if (!VirtualFree (p, LINE_SIZE, MEM_DECOMMIT)) {
    
    BYTE* pb = (BYTE*)p;
    BYTE* end = pb + LINE_SIZE;
    while (pb < end) {

      VirtualQuery (pb, &mbi, sizeof (mbi));
      BYTE* rg_end = (BYTE*)mbi.BaseAddress + mbi.RegionSize;
      if (rg_end > end)
        rg_end = end;

      if (MEM_COMMIT == mbi.State) {

        DWORD old;
        DWORD size = rg_end - pb;
        VirtualProtect (pb, size, PAGE_NOACCESS, &old);
        VirtualAlloc (pb, size, MEM_RESET, PAGE_NOACCESS);
      }
      pb = rg_end;
    }
  }
  VirtualQuery (p, &mbi, sizeof (mbi));
  VirtualQuery ((BYTE*)p + PAGE_SIZE, &mbi, sizeof (mbi));
  VirtualQuery ((BYTE*)p + PAGE_SIZE * 2, &mbi, sizeof (mbi));

  VirtualAlloc ((BYTE*)p + PAGE_SIZE, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);

  DWORD old;
  VirtualProtect (p, PAGE_SIZE * 3, PAGE_WRITECOPY, &old);
  VirtualQuery (p, &mbi, sizeof (mbi));

  // Ќе отображенные страницы вместо WRITECOPY получают READWRITE
  ASSERT (PAGE_READWRITE == mbi.Protect);

  // ќтображенные страницы получают WRITECOPY
  VirtualQuery ((BYTE*)p + PAGE_SIZE * 2, &mbi, sizeof (mbi));
  ASSERT (PAGE_WRITECOPY == mbi.Protect);
  
  *((BYTE*)p) = 2;

  // Clear
  UnmapViewOfFile (p);
  CloseHandle (hm);
}

void test5 ()
{
  MEMORY_BASIC_INFORMATION mbi;
  
  void* p = VirtualAlloc (0, LINE_SIZE, MEM_RESERVE, PAGE_NOACCESS);
  
  VirtualQuery (p, &mbi, sizeof (mbi));
  
  HANDLE hm = CreateFileMapping (INVALID_HANDLE_VALUE, 0, 
    PAGE_READWRITE | SEC_RESERVE, 0, LINE_SIZE, 0);
  
  VirtualFree (p, 0, MEM_RELEASE);
  MapViewOfFileEx (hm, FILE_MAP_WRITE, 0, 0, LINE_SIZE, p);
  VirtualQuery (p, &mbi, sizeof (mbi));
  
  VirtualAlloc (p, PAGE_SIZE, MEM_COMMIT, PAGE_WRITECOPY);
  
  VirtualQuery (p, &mbi, sizeof (mbi));

  ASSERT (mbi.Protect == PAGE_WRITECOPY);

  *((BYTE*)p) = 1;

  VirtualQuery (p, &mbi, sizeof (mbi));

  ASSERT (mbi.Protect == PAGE_READWRITE);
  
  // Commit of already committed pages does not change access

  VirtualAlloc (p, PAGE_SIZE, MEM_COMMIT, PAGE_WRITECOPY);
  VirtualQuery (p, &mbi, sizeof (mbi));
  ASSERT (mbi.Protect == PAGE_READWRITE);
}

void test6 ()
{
  // Test for reset write-copied pages back to mapping

  MEMORY_BASIC_INFORMATION mbi;
  
  // Reserve address
  void* p1 = VirtualAlloc (0, LINE_SIZE, MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  VirtualQuery (p1, &mbi, sizeof (mbi));
  ASSERT (mbi.AllocationProtect == PAGE_EXECUTE_READWRITE);
  
  // Create mapping
  HANDLE hm = CreateFileMapping (INVALID_HANDLE_VALUE, 0, 
    PAGE_READWRITE | SEC_RESERVE, 0, LINE_SIZE, 0);
  
  // Map to address as read-write
  VirtualFree (p1, 0, MEM_RELEASE);
  MapViewOfFileEx (hm, FILE_MAP_WRITE, 0, 0, LINE_SIZE, p1);
  VirtualQuery (p1, &mbi, sizeof (mbi));
  ASSERT (mbi.State == MEM_RESERVE);
  VirtualAlloc (p1, PAGE_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
  VirtualQuery (p1, &mbi, sizeof (mbi));
  ASSERT (mbi.Protect == PAGE_EXECUTE_READWRITE);
  
  // Write to address
  *((BYTE*)p1) = 1;
  
  // Change access to write-copy
  DWORD old;
  VirtualProtect (p1, PAGE_SIZE, PAGE_WRITECOPY, &old);
  VirtualQuery (p1, &mbi, sizeof (mbi));
  ASSERT (mbi.Protect == PAGE_WRITECOPY);
  ASSERT (1 == *((BYTE*)p1));
  
  // Allocate second address
  void* p2 = VirtualAlloc (0, LINE_SIZE, MEM_RESERVE, PAGE_EXECUTE_READWRITE);

  // Map second as write-copy
  VirtualFree (p2, 0, MEM_RELEASE);
  MapViewOfFileEx (hm, FILE_MAP_COPY, 0, 0, LINE_SIZE, p2);
  // Not need VirtualAlloc (p2, PAGE_SIZE, MEM_COMMIT, PAGE_WRITECOPY);
  VirtualQuery (p2, &mbi, sizeof (mbi));
  ASSERT (mbi.Protect == PAGE_WRITECOPY);
  ASSERT (1 == *((BYTE*)p2));

  // Change first address data
  *((BYTE*)p1) = 2;
  VirtualQuery (p1, &mbi, sizeof (mbi));
  ASSERT (mbi.Protect == PAGE_READWRITE);

  // Check second data not changed
  ASSERT (1 == *((BYTE*)p2));

  // Release second
  UnmapViewOfFile (p2);

  /////// DO RESET /////////////

  void* ptmp = MapViewOfFileEx (hm, FILE_MAP_WRITE, 0, 0, LINE_SIZE, 0);
  memcpy (ptmp, p1, PAGE_SIZE);
  UnmapViewOfFile (p1);
  MapViewOfFileEx (hm, FILE_MAP_ALL_ACCESS, 0, 0, LINE_SIZE, p1);
  //VirtualAlloc (p1, PAGE_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

  // Check data and access
  ASSERT (2 == *((BYTE*)p1));
  VirtualQuery (p1, &mbi, sizeof (mbi));
  ASSERT (mbi.Protect == PAGE_EXECUTE_READWRITE);
  
  // Change first
  *((BYTE*)p1) = 3;
  
  // Share again
  VirtualProtect (p1, PAGE_SIZE, PAGE_WRITECOPY, &old);

  // Map second again
  p2 = VirtualAlloc (0, LINE_SIZE, MEM_RESERVE, PAGE_NOACCESS);
  
  // Map second as write-copy
  VirtualFree (p2, 0, MEM_RELEASE);
  MapViewOfFileEx (hm, FILE_MAP_WRITE, 0, 0, LINE_SIZE, p2);
  VirtualAlloc (p2, PAGE_SIZE, MEM_COMMIT, PAGE_WRITECOPY);
  VirtualQuery (p2, &mbi, sizeof (mbi));
  ASSERT (mbi.Protect == PAGE_WRITECOPY);

  // Change first
  *((BYTE*)p1) = 4;
  
  // Check second data !!!
  ASSERT (3 == *((BYTE*)p2));
}

void test7 ()
{
  // Test for VirtualQuery result is limited to allocation region

  MEMORY_BASIC_INFORMATION mbi;

  // Reserve two lines

  void* p = VirtualAlloc (0, LINE_SIZE * 2, MEM_RESERVE, PAGE_READWRITE);

  VirtualQuery (p, &mbi, sizeof (mbi));
  ASSERT (LINE_SIZE * 2 == mbi.RegionSize);

  VirtualFree (p, 0, MEM_RELEASE);

  // Reserve two consecutive lines
  void* p1 = VirtualAlloc (p, LINE_SIZE, MEM_RESERVE, PAGE_READWRITE);
  void* p2 = VirtualAlloc ((BYTE*)p + LINE_SIZE, LINE_SIZE, MEM_RESERVE, PAGE_READWRITE);

  // Check for region limited to allocation region
  VirtualQuery (p1, &mbi, sizeof (mbi));
  ASSERT (LINE_SIZE == mbi.RegionSize);

  VirtualQuery (p2, &mbi, sizeof (mbi));
  ASSERT (LINE_SIZE == mbi.RegionSize);
}

void test8 ()
{
  // Test for execute access

  /*
  Ќа IA-32, Windows не отличает чтение от исполнени€. 
  Ёто позвол€ет отличать отображенные страницы от скопированных при записи. 
  ¬р€д ли стоит использовать эмул€тор под Win32 на других архитектурах.
  */

  MEMORY_BASIC_INFORMATION mbi;
  
  // Reserve address
  BYTE* p = (BYTE*)VirtualAlloc (0, LINE_SIZE, MEM_RESERVE, PAGE_READWRITE);
  
  // Create mapping
  HANDLE hm = CreateFileMapping (INVALID_HANDLE_VALUE, 0, 
    PAGE_READWRITE | SEC_RESERVE, 0, LINE_SIZE, 0);
  
  // Map to address as PAGE_READWRITE
  VirtualFree (p, 0, MEM_RELEASE);
  MapViewOfFileEx (hm, FILE_MAP_WRITE, 0, 0, LINE_SIZE, p);
  VirtualAlloc (p, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
  VirtualQuery (p, &mbi, sizeof (mbi));
  ASSERT (mbi.Protect == PAGE_READWRITE);
  
  // Write to address
  p [0] = 0x90; // nop
  p [1] = 0xC3; // ret

  // Execute address
  void (*pfn) () = (void (*) ())p;
  (*pfn) ();

  VirtualQuery (p, &mbi, sizeof (mbi));
  ASSERT (mbi.Protect == PAGE_READWRITE);

  // Change access to PAGE_EXECUTE
  DWORD old;
  VirtualProtect (p, PAGE_SIZE, PAGE_EXECUTE, &old);

  VirtualQuery (p, &mbi, sizeof (mbi));
  ASSERT (mbi.Protect == PAGE_EXECUTE);

  // Read address
  ASSERT (0x90 == p [0]);
  
  UnmapViewOfFile (p);
  CloseHandle (hm);
}

void test9 ()
{
  // Test for committing of shared page
  /*
  ≈сли резервированна€ страница отображена по нескольким адресам, как только 
  мы делаем MEM_COMMIT по одному адресу, по всем остальным адресам состо€ние 
  также мен€етс€ на MEM_COMMIT. ѕоэтому, после коммита по одному адресу, 
  нужно запретить доступ по остальным адресам.
  */

  MEMORY_BASIC_INFORMATION mbi;
  
  // Reserve address
  void* p1 = VirtualAlloc (0, LINE_SIZE, MEM_RESERVE, PAGE_EXECUTE_READWRITE);

  // Reserve second address
  void* p2 = VirtualAlloc (0, LINE_SIZE, MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  
  // Create mapping
  HANDLE hm = CreateFileMapping (INVALID_HANDLE_VALUE, 0, 
    PAGE_READWRITE | SEC_RESERVE, 0, LINE_SIZE, 0);
  
  // Map to first address
  VirtualFree (p1, 0, MEM_RELEASE);
  MapViewOfFileEx (hm, FILE_MAP_WRITE, 0, 0, LINE_SIZE, p1);
  VirtualQuery (p1, &mbi, sizeof (mbi));
  ASSERT (mbi.State == MEM_RESERVE);

  // Map to second address
  VirtualFree (p2, 0, MEM_RELEASE);
  MapViewOfFileEx (hm, FILE_MAP_WRITE, 0, 0, LINE_SIZE, p2);
  VirtualQuery (p2, &mbi, sizeof (mbi));
  ASSERT (mbi.State == MEM_RESERVE);
  
  // Commit first address
  VirtualAlloc (p1, PAGE_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
  VirtualQuery (p1, &mbi, sizeof (mbi));
  ASSERT (mbi.State == MEM_COMMIT);
  ASSERT (mbi.Protect == PAGE_EXECUTE_READWRITE);
  
  // Check for second address is committed also
  VirtualQuery (p2, &mbi, sizeof (mbi));
  ASSERT (mbi.State == MEM_COMMIT);
  ASSERT (mbi.Protect == PAGE_READWRITE);
}

int main ()
{

  SYSTEM_INFO sysinfo;
  GetSystemInfo (&sysinfo);
  
  //  test4 ();
  static s [8192];
  MEMORY_BASIC_INFORMATION mbi;
  void* p = (void*)(((DWORD)&s + 4096) & ~4095);
  
  VirtualQuery (p, &mbi, sizeof (mbi));
  
  //UnmapViewOfFile (p);
  VirtualFree (p, 4096, MEM_DECOMMIT);

  return 0;
}