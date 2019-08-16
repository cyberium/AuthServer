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

enum ServerRegistrationCommands : uint8
{
    SRC_REGISTERING_REQUEST       = 1,
    SRC_USER_CONFIRMATION_REQUEST = 2,
    SRC_SECURITY_LEVEL_UPDATE     = 3,
    SRC_POPULATION_UPDATE         = 4,
    SRC_STATUS_UPDATE             = 5,

    SRC_HEARTBEAT_COMMAND         = 255
};

enum ServerRegistrationResponse : uint8
{
    SRR_REGISTRATION_RESPONSE      = 1,
    SRR_USER_CONFIRMATION_RESPONSE = 2,
    SRR_SECURITY_LEVEL_RESPONSE    = 3,

    SRR_HEARTBEAT_COMMAND          = 255
};

#endif