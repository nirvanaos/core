// Nirvana project
// ExecDomain - execution domain implementation.

#ifndef _EXEC_DOMAIN_H_
#define _EXEC_DOMAIN_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _WIN32
#include "Win32/ExecDomainBase.h"
#else
#error Undefined target system.
#endif

class ExecDomain : public ExecDomainBase
{
public:

  ExecDomain ();

  void* get_service (unsigned s) const
  {
    if (s <= MAX_SERVICE)
      return m_service_table [s];
    return 0;
  }

  void* set_service (unsigned s, void*p)
  {
    if (s <= MAX_SERVICE) {
      void* r = m_service_table [s];
      m_service_table [s] = p;
      return r;
    } else
      throw BAD_PARAM ();
  }

private:
  enum {
    MAX_SERVICE = SERVICE_RTL_CONTEXT - 0x10000
  };
  
  void* m_service_table [MAX_SERVICE + 1];
};

#endif
