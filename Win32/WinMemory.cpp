// Pi Memory Service emulation for Win32

#include "ProtDomain.h"

#if (HEAP_HEADER_SIZE + HEAP_SIZE < 0x810000)
#pragma comment (linker, "/base:0xC10000")
#else
#error Need to change image base
#endif

#pragma warning (disable:4355)  // 'this' : used in base member initializer list

WinMemInit::WinMemInit (WinMemory* pmain)
{
  WinMemory tmp (pmain);
}

WinMemory::WinMemory () :
WinMemInit (this)
{
  InitializeCriticalSection (&m_critical_section);

  // Create fiber
  m_fiber_param.obj_ptr = this;
  m_fiber = CreateFiber (0, (LPFIBER_START_ROUTINE)fiber_proc, &m_fiber_param);

  // Prepare current thread
  thread_prepare ();
}

WinMemory::WinMemory (WinMemory* pmain) :
WinMemInit ()
{
  m_map = 0;
  m_fiber = 0;

  InitializeCriticalSection (&m_critical_section);

  init ();

  if (!allocate ((Pointer)FIXED_HEAP_PTR, HEAP_HEADER_SIZE + HEAP_SIZE, READ_WRITE | RESERVED | EXACTLY))
    throw INITIALIZE ();
  commit (ProtDomain::singleton (), sizeof (ProtDomain));

  pmain->m_address_space_end = m_address_space_end;
  pmain->m_map = m_map;
  m_map = 0;
}

inline void WinMemory::init ()
{
  // Obtain system parameters
  SYSTEM_INFO sysinfo;
  GetSystemInfo (&sysinfo);
  
  // Check architecture
  if ((PAGE_SIZE != sysinfo.dwPageSize) || (LINE_SIZE != sysinfo.dwAllocationGranularity))
    throw INITIALIZE ();
  
  m_address_space_end = sysinfo.lpMaximumApplicationAddress;
  
  UWord max_lines = (UWord)sysinfo.lpMaximumApplicationAddress / LINE_SIZE;
  
  // Reserve mapping table
  if (!(m_map = (LogicalLine*)VirtualAlloc (0, sizeof (LogicalLine) * max_lines, MEM_RESERVE, PAGE_NOACCESS)))
    throw INITIALIZE ();
}

WinMemory::~WinMemory ()
{
  if ((this > (Pointer)FIXED_HEAP_PTR) && (this < (Pointer)(FIXED_HEAP_PTR + HEAP_HEADER_SIZE + HEAP_SIZE))) {
    //DeleteCriticalSection (&m_critical_section);
    WinMemory tmp (*this);
    m_map = 0;
    m_fiber = 0;
  } else {
    
    // Delete fiber
    if (m_fiber)
      DeleteFiber (m_fiber);
  
    // Release mapping table
    if (m_map) {
      release ((Pointer)FIXED_HEAP_PTR, HEAP_HEADER_SIZE + HEAP_SIZE);
      VirtualFree (m_map, 0, MEM_RELEASE);
    }
    DeleteCriticalSection (&m_critical_section);
  }
}

Word WinMemory::query (Pointer p, QueryParam q)
{
  switch (q) {

  case ALLOCATION_SPACE_BEGIN:
    {
      SYSTEM_INFO sysinfo;
      GetSystemInfo (&sysinfo);
      return (Word)sysinfo.lpMinimumApplicationAddress;
    }

  case ALLOCATION_SPACE_END:
    return (Word)m_address_space_end;
    
  case ALLOCATION_UNIT:
  case SHARING_UNIT:
  case GRANULARITY:
    return LINE_SIZE;

  case PROTECTION_UNIT:
    return PAGE_SIZE;

  case SHARING_ASSOCIATIVITY:
    return 1;

  case FLAGS:
    return 
      ACCESS_CHECK | 
      HARDWARE_PROTECTION | 
      COPY_ON_WRITE |
      SPACE_RESERVATION;
  }

  throw BAD_PARAM ();
}
