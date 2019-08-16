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

#ifndef _REGISTRATION_SOCKET_H
#define _REGISTRATION_SOCKET_H

#include "Network/Socket.hpp"
#include "RegOpcodes.h"
#include "RealmListDef.h"
#include <boost/asio/deadline_timer.hpp>

namespace RealmList2
{

// consider the server connection have to be closed if nothing received after deadline response time (in seconds)
#define DEADLINE_RESPONSE_TIME 30
#define HEARTBEAT_INTERVAL 10

    enum SocketStatus
    {
        CONNECTION_STATUS_NOT_REGISTERED = 0,
        CONNECTION_STATUS_REGISTERED = 1
    };

#pragma pack(push,1)
    struct PacketHeader
    {
        PacketHeader(uint8 cmd, uint16 size) : command(cmd), packetSize(size) {}

        uint8 command;
        uint16 packetSize;
    };
#pragma pack(pop)

    struct membuf : std::streambuf
    {
        membuf(char* begin, char* end) { this->setg(begin, begin, end); }
    };

    class imemstream : private virtual membuf, public std::istream
    {
    public:
        imemstream(char* begin, char* end) : membuf(begin, end), std::istream(static_cast<std::streambuf*>(this)) {}
    };

    class RegistrationSocket : public MaNGOS::Socket
    {
    private:
        virtual bool ProcessIncomingData() override;
        void Send(PacketHeader const& header, char pData[]);
        int32 GetPayLoadLength();

        void StartHeartbeat();
        void HandleHeartbeatWrite(const boost::system::error_code& ec);
        void CheckDeadline();

        bool _HandleRegisteringRequest();
        bool _HandleUserConfirmationRequest() { return false; }
        bool _HandleSecurityLevelUpdate() { return false; }
        bool _HandlePopulationUpdate() { return false; }
        bool _HandleStatusUpdate() { return false; }

        void SendRegisteringResponse(bool added);
        
        SocketStatus m_status;
        std::string m_input;
        uint32 m_realmID;
        PacketHeader m_heartbeatCommand;

        boost::asio::deadline_timer m_deadlineTimer;
        boost::asio::deadline_timer m_heartbeatTimer;

    public:
        RegistrationSocket(boost::asio::io_service& service, std::function<void(Socket*)> closeHandler);
        virtual ~RegistrationSocket();
        virtual bool Open() override;
        virtual void Close() override;
    };
}

#endif
