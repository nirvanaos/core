// Nirvana project
// ProtDomain - protection domain implementation.

#ifndef _PROT_DOMAIN_H_
#define _PROT_DOMAIN_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SyncDomain.h"

#ifdef _WIN32
#include "Win32/ProtDomainBase.h"
#else
#error Undefined target system.
#endif

class ProtDomain : public ProtDomainBase
{
public:

  static int main (int argc, char* argv [], char* envp []);

  static ProtDomain* singleton ()
  {
    return (ProtDomain*)(FIXED_HEAP_PTR + HEAP_HEADER_SIZE);
  }

  SyncDomain* current () const
  {
    return static_cast <SyncDomain*> (ProtDomainBase::current ());
  }

private:

  void* operator new (size_t cb)
  {
    return singleton ();
  }

  void operator delete (void* p) {}

  ProtDomain ();

  void current (SyncDomain* p)
  {
    ProtDomainBase::current (p);
  }
    
private:

  SyncDomain m_main_sync_domain;
};

inline Memory* Nirvana::system_memory ()
{
  return ProtDomain::singleton ();
}

#endif
