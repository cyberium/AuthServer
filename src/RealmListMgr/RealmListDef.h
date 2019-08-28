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
#ifndef _REALMLIST_DEFINITIONS_H
#define _REALMLIST_DEFINITIONS_H

#include "Common.h"
#include "TimedRingBuffer.h"

namespace RealmList2
{
// max line of log in buffer from servers
#define SERVER_LOG_BUFFER_SIZE 1024

    class MaNGOS::Socket;
    /// Type of server
    enum RealmType
    {
        REALM_TYPE_NORMAL        = 0,
        REALM_TYPE_PVP           = 1,
        REALM_TYPE_NORMAL2       = 4,
        REALM_TYPE_RP            = 6,
        REALM_TYPE_RPPVP         = 8,
        REALM_TYPE_FFA_PVP       = 16                       // custom, free for all pvp mode like arena PvP in all zones except rest activated places and sanctuaries
                                                            // replaced by REALM_PVP in realm list
    };

    enum RealmZone
    {
        REALM_ZONE_UNKNOWN       = 0,                       // any language
        REALM_ZONE_DEVELOPMENT   = 1,                       // any language
        REALM_ZONE_UNITED_STATES = 2,                       // extended-Latin
        REALM_ZONE_OCEANIC       = 3,                       // extended-Latin
        REALM_ZONE_LATIN_AMERICA = 4,                       // extended-Latin
        REALM_ZONE_TOURNAMENT_5  = 5,                       // basic-Latin at create, any at login
        REALM_ZONE_KOREA         = 6,                       // East-Asian
        REALM_ZONE_TOURNAMENT_7  = 7,                       // basic-Latin at create, any at login
        REALM_ZONE_ENGLISH       = 8,                       // extended-Latin
        REALM_ZONE_GERMAN        = 9,                       // extended-Latin
        REALM_ZONE_FRENCH        = 10,                      // extended-Latin
        REALM_ZONE_SPANISH       = 11,                      // extended-Latin
        REALM_ZONE_RUSSIAN       = 12,                      // Cyrillic
        REALM_ZONE_TOURNAMENT_13 = 13,                      // basic-Latin at create, any at login
        REALM_ZONE_TAIWAN        = 14,                      // East-Asian
        REALM_ZONE_TOURNAMENT_15 = 15,                      // basic-Latin at create, any at login
        REALM_ZONE_CHINA         = 16,                      // East-Asian
        REALM_ZONE_CN1           = 17,                      // basic-Latin at create, any at login
        REALM_ZONE_CN2           = 18,                      // basic-Latin at create, any at login
        REALM_ZONE_CN3           = 19,                      // basic-Latin at create, any at login
        REALM_ZONE_CN4           = 20,                      // basic-Latin at create, any at login
        REALM_ZONE_CN5           = 21,                      // basic-Latin at create, any at login
        REALM_ZONE_CN6           = 22,                      // basic-Latin at create, any at login
        REALM_ZONE_CN7           = 23,                      // basic-Latin at create, any at login
        REALM_ZONE_CN8           = 24,                      // basic-Latin at create, any at login
        REALM_ZONE_TOURNAMENT_25 = 25,                      // basic-Latin at create, any at login
        REALM_ZONE_TEST_SERVER   = 26,                      // any language
        REALM_ZONE_TOURNAMENT_27 = 27,                      // basic-Latin at create, any at login
        REALM_ZONE_QA_SERVER     = 28,                      // any language
        REALM_ZONE_CN9           = 29                       // basic-Latin at create, any at login
    };

    struct RealmBuildInfo
    {
        int build;
        int major_version;
        int minor_version;
        int bugfix_version;
        int hotfix_version;
    };
    typedef std::set<uint32> RealmBuilds;
    typedef std::unique_ptr<TimedRingBuffer<std::string>> TimedRingBufferUPtr;

    /// Storage object for a realm
    struct RealmData
    {
        RealmData() : ServerLog(new TimedRingBuffer<std::string>(SERVER_LOG_BUFFER_SIZE)) {}
        std::string Name;
        std::string Address;
        int Id;
        RealmType Type;
        uint8 Flags;                                        // realm status
        RealmZone TimeZone;
        AccountTypes AllowedSecLevel;                       // current allowed join security level (show as locked for not fit accounts)
        float PopulationLevel;
        std::set<uint32> AcceptedBuilds;                    // list of supported builds
        std::shared_ptr <MaNGOS::Socket> ServerSocket;
        TimedRingBufferUPtr ServerLog;                      //buffer that will be filled by servers events and logs
    };
    typedef std::unique_ptr<RealmData> RealmDataUPtr;
    typedef std::map<uint32, RealmDataUPtr> RealmMap;

}

#endif