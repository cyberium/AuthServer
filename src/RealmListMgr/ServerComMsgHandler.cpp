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

#include "RegSocket.h"
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "RealmListMgr.h"

using namespace RealmList2;
namespace pt = boost::property_tree;


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
        std::stringstream json;
        *pkt >> json;
        pt::ptree receivedJSON;

        try
        {
            pt::read_json(json, receivedJSON);

            if (sLog.HasLogLevelOrHigher(LOG_LVL_DEBUG))
            {
                pt::write_json(std::cout, receivedJSON);
            }

            std::unique_ptr<RealmData> regData = std::unique_ptr<RealmData>(new RealmData());

            regData->Name = receivedJSON.get<std::string>("Name");
            regData->Id = receivedJSON.get<uint32>("Id");
            regData->Type = static_cast<RealmType>(receivedJSON.get<uint32>("Type"));
            regData->Flags = receivedJSON.get<uint32>("Flags");
            regData->TimeZone = static_cast<RealmZone>(receivedJSON.get<uint32>("TimeZone"));
            regData->AllowedSecLevel = static_cast<AccountTypes>(receivedJSON.get<uint32>("AllowedSecLevel"));
            regData->PopulationLevel = receivedJSON.get<float>("PopulationLevel");

            for (const auto& buildItr : receivedJSON.get_child("AcceptedBuilds"))
                regData->AcceptedBuilds.insert(buildItr.second.get_value<uint32>());

            m_realmID = regData->Id;

            regData->ServerSocket = shared<MaNGOS::Socket>();

            sRealmListMgr.AddRealm(std::move(regData));

            added = true;
        }
        catch (const pt::ptree_error& e)
        {
            sLog.outError("RegistrationSocket::_HandleRegisteringRequest> %s", e.what());
        }
        catch (...)
        {
            sLog.outError("RegistrationSocket::_HandleRegisteringRequest> Unable to parse JSON!");
        }
    }

    SendRegisteringResponse(added);

    if (pkt->rpos() < pkt->wpos())
    {
        sLog.outError("RegistrationSocket::_HandleRegisteringRequest> Packet received contain unprocessed datas!");
    }

    return true;
}

bool SrvComSocket::_HandleHeartbeat(ServerComPacketUPtr pkt)
{
    return true;
}

// Responses
void SrvComSocket::SendRegisteringResponse(bool added)
{
    uint8 response = added ? 1 : 0;
    PacketHeader header(RMSG_REGISTRATION_RESPONSE, sizeof(uint8));
    Send(header, (char*)& response);
}
