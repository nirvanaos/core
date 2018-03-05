// Nirvana Execution Domain implementation for Win32

#ifndef _EXEC_DOMAIN_BASE_
#define _EXEC_DOMAIN_BASE_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class ExecDomainBase
{
private:
  void* m_fiber;
};

#endif
