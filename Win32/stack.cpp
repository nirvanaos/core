#include "MemInfo.h"

NT_TIB* WinMemory::current_tib ()
{
  NT_TIB* ptib;
  
  __asm {
    mov eax, fs:[18h]
    mov [ptib],eax
  }
  
  return ptib;
}

void WinMemory::thread_prepare ()
{
  // Prepare stack of current thread to share

  // Obtain stack size
  ShareStackParam param;
  const NT_TIB* ptib = current_tib ();
  param.stack_base = ptib->StackBase;
  param.stack_limit = ptib->StackLimit;

  // Call share_stack in fiber
  verify (ConvertThreadToFiber (0));
  call_in_fiber ((FiberMethod)stack_prepare, &param);
}

void WinMemory::thread_unprepare ()
{
}

void WinMemory::stack_prepare (const ShareStackParam* param_ptr)
{
  // Copy parameter from source stack
  // The source stack will become temporary unaccessible
  ShareStackParam param = *param_ptr;

  // Temporary copy committed stack
  UWord cur_stack_size = (Octet*)param.stack_base - (Octet*)param.stack_limit;
  void* ptmp = VirtualAlloc (0, cur_stack_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!ptmp)
    throw NO_MEMORY ();

  //memcpy (ptmp, param.stack_limit, cur_stack_size);
  real_copy ((const UWord*)param.stack_limit, (const UWord*)param.stack_base, (UWord*)ptmp);

  // Get stack region
  // The first page before param.m_stack_limit is a guard page
  MemoryBasicInformation mbi ((Page*)param.stack_limit - 1);

  assert (MEM_COMMIT == mbi.State);
  assert ((PAGE_GUARD | PAGE_READWRITE) == mbi.Protect);
  assert (PAGE_SIZE == mbi.RegionSize);
  
  // Decommit Windows memory
  VirtualFree (mbi.BaseAddress, (Octet*)param.stack_base - (Octet*)mbi.BaseAddress, MEM_DECOMMIT);

#ifdef _DEBUG
  {
    MemoryBasicInformation dmbi (mbi.AllocationBase);

    assert (dmbi.State == MEM_RESERVE);
    assert (mbi.AllocationBase == dmbi.AllocationBase);
    assert ((((Octet*)mbi.AllocationBase) + dmbi.RegionSize) == (Octet*)param.stack_base);
  }
#endif // _DEBUG

  Line* stack_begin = mbi.allocation_base ();
  assert (!((UWord)param.stack_base % LINE_SIZE));
  Line* stack_end = (Line*)param.stack_base;
  Line* line = stack_begin;

  try {
    
    // Map our memory (but not commit)
    do
      map (line);
    while (++line < stack_end);

  } catch (...) {

    // Unmap
    while (line > stack_begin)
      unmap (--line);

    // Commit current stack
    VirtualAlloc (param.stack_limit, cur_stack_size, MEM_COMMIT, PAGE_READWRITE);
    
    // Commit guard page(s)
    VirtualAlloc (mbi.BaseAddress, mbi.RegionSize, MEM_COMMIT, PAGE_GUARD | PAGE_READWRITE);

    // Release temporary buffer
    VirtualFree (ptmp, 0, MEM_RELEASE);

    throw;
  }

  // Commit current stack
  VirtualAlloc (param.stack_limit, cur_stack_size, MEM_COMMIT, PAGE_READWRITE);

  // Commit guard page(s)
  VirtualAlloc ((Page*)param.stack_limit - 1, mbi.RegionSize, MEM_COMMIT, PAGE_GUARD | PAGE_READWRITE);
  
  // Copy stack data back
  //memcpy (param.stack_limit, ptmp, cur_stack_size);
  real_copy ((const UWord*)ptmp, (const UWord*)((Octet*)ptmp + cur_stack_size), (UWord*)param.stack_limit);

  // Release temporary buffer
  VirtualFree (ptmp, 0, MEM_RELEASE);

  assert (!memcmp (&param, param_ptr, sizeof (ShareStackParam)));
}

void WinMemory::call_in_fiber (FiberMethod method, void* param)
{
  // Only one thread can use fiber call
  Lock lock (m_critical_section);

  m_fiber_param.source_fiber = GetCurrentFiber ();
  m_fiber_param.method = method;
  m_fiber_param.param = param;
  SwitchToFiber (m_fiber);
  if (m_fiber_param.failed)
    ((SystemException*)m_fiber_param.system_exception)->_raise ();
}

void WinMemory::fiber_proc (FiberParam* param)
{
  for (;;) {

    try {
      param->failed = false;
      (param->obj_ptr->*(param->method)) (param->param);
    } catch (const SystemException& ex) {
      param->failed = true;
      real_copy ((const UWord*)&ex, (const UWord*)(&ex + 1), param->system_exception);
    } catch (...) {
      param->failed = true;

      UNKNOWN ue;
      real_copy ((const UWord*)&ue, (const UWord*)(&ue + 1), param->system_exception);
    }

    SwitchToFiber (param->source_fiber);
  }
}

