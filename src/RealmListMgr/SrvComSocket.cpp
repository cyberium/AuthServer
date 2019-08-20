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
#include "Log/Log.h"
#include "Utilities/ByteConverter.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include "RealmListMgr.h"

using namespace RealmList2;
namespace pt = boost::property_tree;

SrvComSocket::SrvComSocket(boost::asio::io_service& service, std::function<void(Socket*)> closeHandler) :
    MaNGOS::Socket(service, std::move(closeHandler)), m_status(CONNECTION_STATUS_NOT_REGISTERED), m_realmID(0),
    m_deadlineTimer(service), m_heartbeatTimer(service), m_heartbeatCommand(uint8(MSG_HEARTBEAT_COMMAND), 0)
{
}

SrvComSocket::~SrvComSocket()
{
}

void SrvComSocket::StartHeartbeat()
{
    if (IsClosed())
        return;

    // Start an asynchronous operation to send a heartbeat message.
    boost::asio::async_write(GetAsioSocket(), boost::asio::buffer(&m_heartbeatCommand, sizeof(m_heartbeatCommand)),
        boost::bind(&SrvComSocket::HandleHeartbeatWrite, shared<SrvComSocket>(), _1));
}

void SrvComSocket::HandleHeartbeatWrite(const boost::system::error_code& ec)
{
    if (IsClosed())
        return;

    if (!ec)
    {
        DEBUG_LOG("Heartbeat sent... ");

        // Wait 10 seconds before sending the next heartbeat.
        m_heartbeatTimer.expires_from_now(boost::posix_time::seconds(HEARTBEAT_INTERVAL));
        m_heartbeatTimer.async_wait(boost::bind(&SrvComSocket::StartHeartbeat, shared<SrvComSocket>()));
    }
    else
    {
        std::cout << "Error on Heartbeat: " << ec.message() << "\n";

        Close();
    }
}

void SrvComSocket::CheckDeadline()
{
    if (IsClosed())
        return;

    // Check whether the deadline has passed. We compare the deadline against
    // the current time since a new asynchronous operation may have moved the
    // deadline before this actor had a chance to run.
    if (m_deadlineTimer.expires_at() <= boost::asio::deadline_timer::traits_type::now())
    {
        // The deadline has passed. The socket is closed so that any outstanding
        // asynchronous operations are canceled.
        DEBUG_LOG("RegistrationSocket::CheckDeadline> No hearbeat since %usec from server id(%u), closing connection!", uint32(DEADLINE_RESPONSE_TIME), m_realmID);
        Close();

        // There is no longer an active deadline. The expiry is set to positive
        // infinity so that the actor takes no action until a new deadline is set.
        m_deadlineTimer.expires_at(boost::posix_time::pos_infin);
    }

    // Put the actor back to sleep.
    m_deadlineTimer.async_wait(boost::bind(&SrvComSocket::CheckDeadline, shared<SrvComSocket>()));
}

int32 SrvComSocket::GetPayLoadLength()
{
    char buffer[2];
    Read(&buffer[0], sizeof(buffer));
    uint16* pktSize = static_cast<uint16*>(static_cast<void*>(&buffer[0]));
    EndianConvert(*pktSize);

    if (ReadLengthRemaining() < *pktSize)
        return -1;

    return int32(*pktSize);
}

bool SrvComSocket::ProcessIncomingData()
{
    DEBUG_LOG("RegistrationSocket::ProcessIncomingData");

    bool result = true;
    bool malFormedPacket = false;
    if (ReadLengthRemaining() >= sizeof(PacketHeader))
    {
        char buffer[2];
        Read(&buffer[0], 1);
        const SrvComOpcodes cmd = static_cast<SrvComOpcodes>(buffer[0]);

        Read(&buffer[0], 2);
        uint16* pktSize = static_cast<uint16*>(static_cast<void*>(&buffer[0]));
        EndianConvert(*pktSize);
        if (cmd < SRV_OPCODES_MAX && ReadLengthRemaining() >= *pktSize)
        {
            ServerComPacketUPtr pkt= ServerComPacketUPtr(new ServerComPacket(cmd, *pktSize));
            if (*pktSize > 0)
            {
                pkt->append(InPeak(), *pktSize);
                ReadSkip(*pktSize);
            }

            // call handler
            result = (*this.*SrvComOpcodesTable[cmd].handler)(std::move(pkt));

            if (ReadLengthRemaining() > 0)
                sLog.outError("Some remaining data are not processed for %s", SrvComOpcodesTable[cmd].name);

            /*switch (cmd)
            {
                case SMSG_REGISTERING_REQUEST:
                    result = _HandleRegisteringRequest();
                    break;

                case MSG_HEARTBEAT_COMMAND:
                    DEBUG_LOG("Heartbeat received... ");
                    ReadSkip(ReadLengthRemaining());
                    break;

                case SMSG_USER_CONFIRMATION_REQUEST:
                case SMSG_SECURITY_LEVEL_UPDATE:
                case SMSG_POPULATION_UPDATE:
                case SMSG_STATUS_UPDATE:
                {
                    ReadSkip(ReadLengthRemaining());
                    DEBUG_LOG("Not Handled yet");
                    break;
                }
                default:
                {
                    ReadSkip(ReadLengthRemaining());
                    DEBUG_LOG("Bad command!");
                    break;
                }
            }*/
        }
        else
            malFormedPacket = true;
    }
    else
        malFormedPacket = true;

    if (malFormedPacket)
    {
        DEBUG_LOG("RegistrationSocket::ProcessIncomingData> recived mal formed packet!");
        ReadSkip(ReadLengthRemaining());
    }
    m_deadlineTimer.expires_from_now(boost::posix_time::seconds(DEADLINE_RESPONSE_TIME));
    return result;
}

bool SrvComSocket::Open()
{
    if (!Socket::Open())
        return false;

    DEBUG_LOG("RegistrationSocket> Incoming connection from %s.", m_address.c_str());

    StartHeartbeat();

    m_deadlineTimer.expires_from_now(boost::posix_time::seconds(DEADLINE_RESPONSE_TIME));
    CheckDeadline();

    return true;
}

void SrvComSocket::Close()
{
    m_deadlineTimer.cancel();
    m_heartbeatTimer.cancel();
    Socket::Close();
    sRealmListMgr.SetRealmOnlineStatus(m_realmID, false);
    DEBUG_LOG("RegistrationSocket> Connection was closed.");
}

void SrvComSocket::Send(PacketHeader const& header, char pData[])
{
    Write((char const*)&header, sizeof(header), pData, header.packetSize);
    m_heartbeatTimer.expires_from_now(boost::posix_time::seconds(HEARTBEAT_INTERVAL));
}
