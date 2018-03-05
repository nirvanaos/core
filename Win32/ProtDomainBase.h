// Nirvana Protection Domain implementation for Win32

#ifndef _PROT_DOMAIN_BASE_
#define _PROT_DOMAIN_BASE_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WinMemory.h"

class ProtDomainBase : public WinMemory
{
public:

  ProtDomainBase ()
  {
    m_tls = TlsAlloc ();
  }

  ~ProtDomainBase ()
  {
    TlsFree (m_tls);
  }

protected:

  void* current () const
  {
    return TlsGetValue (m_tls);
  }

  void current (void* p)
  {
    TlsSetValue (m_tls, p);
  }

private:
  DWORD m_tls;
};

#endif


