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

#include "SrvComSocket.h"
#include <iostream>
#include "RealmListMgr.h"
#include "Log/Log.h"

using namespace RealmList2;

bool SrvComSocket::_HandleNullMsg(ServerComPacketUPtr pkt)
{
    sLog.outError("RegistrationSocket::_HandleNullMsg> Not handled %s received!", pkt->GetOpcodeName());
    return true;
}

bool SrvComSocket::_HandleRegisteringRequest(ServerComPacketUPtr pkt)
{
    bool added = false;
    if (m_status != CONNECTION_STATUS_NOT_REGISTERED)
    {
        sLog.outError("RegistrationSocket::_HandleRegisteringRequest> Server %u sent registration request while already registered!",
            m_realmID);
    }
    else
    {
        try
        {
            std::unique_ptr<RealmData> regData = std::unique_ptr<RealmData>(new RealmData());

            *pkt >> regData->Id;
            *pkt >> regData->Name;
            *pkt >> regData->GamePort;
            *pkt >> regData->MaxPlayers;
            *pkt >> regData->OnlinePlayers;
            *pkt >> regData->MinAccLevel;
            *pkt >> regData->Flags;
            *pkt >> regData->Type;
            *pkt >> regData->Zone;

            uint8 buildCount;
            *pkt >> buildCount;
            for (uint8 i = 0; i < buildCount; ++i)
            {
                uint32 buildId;
                *pkt >> buildId;
                regData->AcceptedBuilds.insert(buildId);
            }

            regData->Host = this->GetRemoteAddress();

            m_realmID = regData->Id;

            regData->ServerSocket = shared<MaNGOS::Socket>();

            sRealmListMgr.AddRealm(std::move(regData));

            added = true;
        }
        catch (...)
        {
            sLog.outError("RegistrationSocket::_HandleRegisteringRequest> Unable to add realm due to bad data provided!");
        }
    }

    SendRegisteringResponse(added);

    if (added)
        m_status = CONNECTION_STATUS_REGISTERED;

    return true;
}

bool SrvComSocket::_HandleHeartbeat(ServerComPacketUPtr pkt)
{
    return true;
}

bool SrvComSocket::_HandleStatusUpdate(ServerComPacketUPtr pkt)
{
    if (m_status != CONNECTION_STATUS_REGISTERED)
    {
        sLog.outError("RegistrationSocket::_HandleStatusUpdate> Server %u sent status update without being registered!",
            m_realmID);
    }
    else
    {
        uint8 flags;
        float populationLevel;
        *pkt >> flags;
        *pkt >> populationLevel;
        sRealmListMgr.UpdateRealmStatus(m_realmID, flags, populationLevel);
    }
    return true;
}

bool SrvComSocket::_HandleLogMessage(ServerComPacketUPtr pkt)
{
    if (m_status != CONNECTION_STATUS_REGISTERED)
    {
        sLog.outError("RegistrationSocket::_HandleStatusUpdate> Server %u sent status update without being registered!",
            m_realmID);
    }
    else
    {
        uint8 type;
        *pkt >> type;

        std::string logStr;
        *pkt >> logStr;

        sRealmListMgr.AddRealmLog(m_realmID, type, logStr);
    }
    return true;
}

// Responses
void SrvComSocket::SendRegisteringResponse(bool added)
{
    uint8 response = added ? 1 : 0;
    PacketHeader header(RMSG_REGISTRATION_RESPONSE, sizeof(uint8));
    Send(header, (char*)& response);
}
