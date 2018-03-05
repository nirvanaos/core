// Nirvana project
// SyncDomain - synchronization domain.

#ifndef _SYNC_DOMAIN_H_
#define _SYNC_DOMAIN_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _WIN32
#include "Win32/SyncDomainBase.h"
#else
#error Undefined target system.
#endif

#include "SyncDomainMemory.h"
#include "ExecDomain.h"

class SyncDomain : 
  public SyncDomainBase, 
  public SyncDomainMemory,
  public Services
{
public:

  SyncDomain ()
  {
    memset (m_service_table, 0, sizeof (m_service_table));
    m_service_table [SERVICE_MEMORY] = static_cast <Memory*> (this);
  }

  void* get_service (Service s) const
  {
    if ((unsigned)s <= MAX_SERVICE)
      return m_service_table [s];
    else if ((((int&)s) -= EXECUTION_DOMAIN_SERVICES) >= 0)
      return execution_domain ()->get_service ((unsigned)s);
    else
      return 0;
  }

  virtual Pointer set_service (Service s, Pointer p)
  {
    if ((unsigned)s <= MAX_SERVICE) {
      Pointer r = m_service_table [s];
      m_service_table [s] = p;
      return r;
    } else if (s >= EXECUTION_DOMAIN_SERVICES)
      return execution_domain ()->set_service ((unsigned)s, p);
    else
      throw BAD_PARAM ();
  }

  Memory* memory () const
  {
    return (Memory*)m_service_table [SERVICE_MEMORY];
  }

  ExecDomain* execution_domain () const
  {
    return (ExecDomain*)m_service_table [SERVICE_EXECUTION_DOMAIN];
  }

private:

  enum {
    MAX_SERVICE = SERVICE_SERVICES,
    EXECUTION_DOMAIN_SERVICES = 0x10000
  };

  void* m_service_table [MAX_SERVICE + 1];
};

#endif
