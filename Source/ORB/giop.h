/// \file
/// GIOP - General Inter-ORB Protocol
/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#ifndef NIRVANA_ORB_GIOP_H_
#define NIRVANA_ORB_GIOP_H_

#include "iop.h"

namespace GIOP {

struct Version
{
  uint8_t major;
  uint8_t minor;
};

#if MAX_GIOP_VERSION_NUMBER == 0
// GIOP 1.0
enum MsgType_1_0
{ // Renamed from MsgType
  Request, Reply, CancelRequest,
  LocateRequest, LocateReply,
  CloseConnection, MessageError
};
#else
// GIOP 1.1
enum MsgType_1_1
{
  Request, Reply, CancelRequest,
  LocateRequest, LocateReply,
  CloseConnection, MessageError,
  Fragment // GIOP 1.1 addition
};
#endif // MAX_GIOP_VERSION_NUMBER

typedef char Magicn [4];

struct MessageHeader_1_1
{
  Magicn magic;
  Version GIOP_version;
  uint8_t flags; // GIOP 1.1 change
  uint8_t message_type;
  unsigned long message_size;
};

#ifndef GIOP_1_2
  
  // GIOP 1.0 and 1.1
  enum ReplyStatusType_1_0 : int32_t { // Renamed from ReplyStatusType
    NO_EXCEPTION,
    USER_EXCEPTION,
    SYSTEM_EXCEPTION,
    LOCATION_FORWARD
  };

  // GIOP 1.0
  struct ReplyHeader_1_0 { // Renamed from ReplyHeader
    IOP::ServiceContextList service_context;
    uint32_t request_id;
    ReplyStatusType_1_0 reply_status;
  };

  // GIOP 1.1
  typedef ReplyHeader_1_0 ReplyHeader_1_1;
  // Same Header contents for 1.0 and 1.1

#else

  // GIOP 1.2
  enum ReplyStatusType_1_2 : int32_t {
    NO_EXCEPTION,
    USER_EXCEPTION,
    SYSTEM_EXCEPTION,
    LOCATION_FORWARD,
    LOCATION_FORWARD_PERM,// new value for 1.2
    NEEDS_ADDRESSING_MODE // new value for 1.2
  };

  struct ReplyHeader_1_2 {
    uint32_t request_id;
    ReplyStatusType_1_2 reply_status;
    IOP::ServiceContextList service_context;
  };

#endif // GIOP_1_2

} // namespace GIOP

#endif // _GIOP_H_
