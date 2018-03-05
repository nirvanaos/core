// Nirvana project
// Nirvana ESIOP - Environment-Specific Inter-ORB Protocol

#include <Nirvana.h>

using namespace Nirvana;

namespace {

// Repository Id table for system exceptions in alphabetic order
// Prefix is: "IDL:omg.org/CORBA/"; Postfix is ":1.0"

const char* const system_exception_rids [] = {
  "BAD_CONTEXT",
  "BAD_INV_ORDER",
  "BAD_OPERATION",
  "BAD_PARAM",
  "BAD_TYPECODE",
  "COMM_FAILURE",
  "DATA_CONVERSION",
  "FREE_MEM"
  "IMP_LIMIT",
  "INITIALIZE",
  "INTERNAL",
  "INTF_REPOS",
  "INVALID_TRANSACTION",
  "INV_FLAG",
  "INV_IDENT",
  "INV_OBJREF",
  "MARSHAL",
  "NO_IMPLEMENT"
  "NO_MEMORY",
  "NO_PERMISSION",
  "NO_RESOURCES",
  "NO_RESPONSE",
  "OBJECT_NOT_EXIST",
  "OBJ_ADAPTER",
  "PERSIST_STORE",
  "TRANSACTION_REQUIRED",
  "TRANSACTION_ROLLEDBACK",
  "TRANSIENT",
  "UNKNOWN"
};

} // namespace

void EnvironmentSE::exception (const SystemException& ex)
{
  m_status = SYSTEM_EXCEPTION;
  m_system_exception.m_major = ex.__code ();
  m_system_exception.m_completed = ex.completed ();
}

void EnvironmentSE::unlisted_user_exception ()
{
  m_status = SYSTEM_EXCEPTION;
  m_system_exception.m_major = EC_UNKNOWN;
  m_system_exception.m_minor = MAKE_OMG_MINOR (1);
  m_system_exception.m_completed = COMPLETED_NO;
}

void EnvironmentSE::_raise ()
{
  switch (m_status) {

  case SYSTEM_EXCEPTION:
    m_system_exception.raise ();

  case USER_EXCEPTION:  // ????
    throw UNKNOWN (MAKE_OMG_MINOR (1), COMPLETED_NO);
  }
}

void SystemExceptionInfo::raise ()
{
  switch (m_major) {
    
  case EC_UNKNOWN:
    throw UNKNOWN (m_minor, m_completed);
    
  case EC_BAD_PARAM:
    throw BAD_PARAM (m_minor, m_completed);
    
  case EC_NO_MEMORY:
    throw NO_MEMORY (m_minor, m_completed);
    
  case EC_IMP_LIMIT:
    throw IMP_LIMIT (m_minor, m_completed);
    
  case EC_COMM_FAILURE:
    throw COMM_FAILURE (m_minor, m_completed);

  case EC_INV_OBJREF:
    throw INV_OBJREF (m_minor, m_completed);

  case EC_NO_PERMISSION:
    throw NO_PERMISSION (m_minor, m_completed);

  case EC_INTERNAL:
    throw INTERNAL (m_minor, m_completed);

  case EC_MARSHAL:
    throw MARSHAL (m_minor, m_completed);

  case EC_INITIALIZE:
    throw INITIALIZE (m_minor, m_completed);

  case EC_NO_IMPLEMENT:
    throw NO_IMPLEMENT (m_minor, m_completed);

  case EC_BAD_TYPECODE:
    throw BAD_TYPECODE (m_minor, m_completed);

  case EC_BAD_OPERATION:
    throw BAD_OPERATION (m_minor, m_completed);

  case EC_NO_RESOURCES:
    throw NO_RESOURCES (m_minor, m_completed);

  case EC_NO_RESPONSE:
    throw NO_RESPONSE (m_minor, m_completed);

  case EC_PERSIST_STORE:
    throw PERSIST_STORE (m_minor, m_completed);

  case EC_BAD_INV_ORDER:
    throw BAD_INV_ORDER (m_minor, m_completed);

  case EC_TRANSIENT:
    throw TRANSIENT (m_minor, m_completed);

  case EC_FREE_MEM:
    throw FREE_MEM (m_minor, m_completed);

  case EC_INV_IDENT:
    throw INV_IDENT (m_minor, m_completed);

  case EC_INV_FLAG:
    throw INV_FLAG (m_minor, m_completed);

  case EC_INTF_REPOS:
    throw INTF_REPOS (m_minor, m_completed);

  case EC_BAD_CONTEXT:
    throw BAD_CONTEXT (m_minor, m_completed);

  case EC_OBJ_ADAPTER:
    throw OBJ_ADAPTER (m_minor, m_completed);

  case EC_DATA_CONVERSION:
    throw DATA_CONVERSION (m_minor, m_completed);

  case EC_OBJECT_NOT_EXIST:
    throw OBJECT_NOT_EXIST (m_minor, m_completed);

  case EC_TRANSACTION_REQUIRED:
    throw TRANSACTION_REQUIRED (m_minor, m_completed);

  case EC_TRANSACTION_ROLLEDBACK:
    throw TRANSACTION_ROLLEDBACK (m_minor, m_completed);

  case EC_INVALID_TRANSACTION:
    throw INVALID_TRANSACTION (m_minor, m_completed);

  default:
    throw UNKNOWN (MAKE_OMG_MINOR (2), COMPLETED_NO);
  }
}
