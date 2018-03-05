// Nirvana project
// Message sending/receiving

#ifndef _MESSAGES_H_
#define _MESSAGES_H_

#include "core.h"
#include "PriorityQueue.h"

namespace Nirvana {

// Hardware domain - relative object key
struct ObjectKey
{
  Pointer prot_domain;
  Pointer sync_domain;
  Pointer object;
};

class MessageTarget;

class MessageHeader;

class MessageHeader : public PriorityQueue <MessageHeader>::Item
{
  
};

class Message
{
public:
  Message (MessageTarget& target);
  void send ();

  void append_data (const void* p, UWord cb, bool release_source);
};

} // namespace Nirvana

#endif
