// Nirvana project
// ProtDomain - protection domain implementation.

#include "ProtDomain.h"
#include "SyncDomain.h"
#include "test/memtest.h"

int main (int argc, char* argv [], char* envp [])
{
  return ProtDomain::main (argc, argv, envp);
}

int ProtDomain::main (int argc, char* argv [], char* envp [])
{
  new ProtDomain ();

  memtest ();

  delete singleton ();

  return 0;
}

ProtDomain::ProtDomain ()
{
  Heap h;
  h.create ((Pointer)FIXED_HEAP_PTR);
  h.allocate (0, sizeof (ProtDomain), READ_WRITE);
  m_main_sync_domain.init_heap (h);
  current (&m_main_sync_domain);
}

void* Nirvana::get_service (Service s)
{
  return ProtDomain::singleton ()->current ()->get_service (s);
}