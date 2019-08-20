/*
 * This file is part of the CMaNGOS Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _REGISTRATION_OPCODES_H
#define _REGISTRATION_OPCODES_H
#include "ByteBuffer/ByteBuffer.h"

// this enum should be incremental values to fit the static table
enum SrvComOpcodes : uint8
{
    MSG_NULL_ACTION                 = 0,
    SMSG_REGISTERING_REQUEST        = 1,
    SMSG_USER_CONFIRMATION_REQUEST  = 2,
    SMSG_SECURITY_LEVEL_UPDATE      = 3,
    SMSG_POPULATION_UPDATE          = 4,
    SMSG_STATUS_UPDATE              = 5,

    RMSG_REGISTRATION_RESPONSE      = 6,
    RMSG_USER_CONFIRMATION_RESPONSE = 7,
    RMSG_SECURITY_LEVEL_RESPONSE    = 8,

    MSG_HEARTBEAT_COMMAND           = 9,                    // warning should always be last message
};

const uint8 SRV_OPCODES_MAX = ((uint8)MSG_HEARTBEAT_COMMAND) + 1;

enum SrvComSecReq
{
    SRV_COM_SECURITY_FREE      = 0,
    SRV_COM_SECURITY_NEED_AUTH = 1
};

#endif