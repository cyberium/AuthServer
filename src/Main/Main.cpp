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

#include "GlobalDefinitions.h"
#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "Config/Config.h"
#include "Log/Log.h"
#include "RealmList.h"
#include "AuthSocket.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <openssl/opensslv.h>
#include <openssl/crypto.h>
#include <mysql_version.h>
#include "Network/Listener.hpp"

bool stopEvent = false;                                     ///< Setting it to true stops the server
DatabaseType LoginDatabase;                                 ///< Accessor to the realm server database

/// Handle termination signals
/** Put the global variable stopEvent to 'true' if a termination signal is caught **/
void OnSignal(int s)
{
    switch (s)
    {
        case SIGINT:
        case SIGTERM:
            stopEvent = true;
            break;
#ifdef _WIN32
        case SIGBREAK:
            stopEvent = true;
            break;
#endif
    }

    signal(s, OnSignal);
}

/// Initialize connection to the database
bool StartDB()
{
    std::string dbstring = sConfig.GetStringDefault("LoginDatabaseInfo");
    if (dbstring.empty())
    {
        sLog.outError("Database not specified");
        return false;
    }

    if (!LoginDatabase.Initialize(dbstring.c_str()))
    {
        sLog.outError("Cannot connect to database");
        return false;
    }

    if (!LoginDatabase.CheckRequiredField("db_version", REVISION_DB_AUTHSERVER))
    {
        ///- Wait for already started DB delay threads to end
        LoginDatabase.HaltDelayThread();
        return false;
    }

    sLog.outString("MySQL client library: %s", LoginDatabase.GetClientInfo().c_str());
    sLog.outString("MySQL server ver: %s ", LoginDatabase.GetServerInfo().c_str());

    return true;
}

/// Define hook 'OnSignal' for all termination signals
void HookSignals()
{
    signal(SIGINT, OnSignal);
    signal(SIGTERM, OnSignal);
#ifdef _WIN32
    signal(SIGBREAK, OnSignal);
#endif
}

/// Unhook the signals before leaving
void UnhookSignals()
{
    signal(SIGINT, 0);
    signal(SIGTERM, 0);
#ifdef _WIN32
    signal(SIGBREAK, 0);
#endif
}

int main(int argc, char* argv[])
{
    if (!sConfig.SetSource(CONFIG_DEFAULT_FILENAME))
    {
        std::cout << "Could not find configuration file" << CONFIG_DEFAULT_FILENAME << ".";
        Log::WaitBeforeContinueIfNeed();
        return 1;
    }

    sLog.Initialize();
    sLog.outString("Use <Ctrl-C> to stop.");
    sLog.outString("%s AuthServer v%s", _PACKAGENAME, AUTH_VERSION);
    sLog.outString("Build date: %s", BUILD_DATE);
    sLog.outString("Git head hash: %s, (date: %s)", REVISION_ID, REVISION_DATE);
    sLog.outString("Platform (%s), Architecture (%s)", _ENDIAN_STRING, ARCHITECTURE);

    sLog.outString("Using configuration file %s.", CONFIG_DEFAULT_FILENAME);
    ///- Check the version of the configuration file
    uint32 confVersion = sConfig.GetIntDefault("ConfVersion", 0);
    if (confVersion < AUTH_CONFIG_MINIMAL_REVISION)
    {
        sLog.outError("*****************************************************************************");
        sLog.outError(" WARNING: Your realmd.conf version indicates your conf file is out of date!");
        sLog.outError("          Please check for updates, as your current default values may cause");
        sLog.outError("          strange behavior.");
        sLog.outError("*****************************************************************************");
        Log::WaitBeforeContinueIfNeed();
    }

    sLog.outString("%s (Library: %s)", OPENSSL_VERSION_TEXT, SSLeay_version(SSLEAY_VERSION));
    if (SSLeay() < 0x009080bfL)
    {
        DETAIL_LOG("WARNING: Outdated version of OpenSSL lib. Logins to server may not work!");
        DETAIL_LOG("WARNING: Minimal required version [OpenSSL 0.9.8k]");
    }

    sLog.outString("Using Boost version: %i.%i.%i", BOOST_VERSION / 100000, BOOST_VERSION / 100 % 1000, BOOST_VERSION % 100);

    // Initialize the database connection
    if (!StartDB())
    {
        Log::WaitBeforeContinueIfNeed();
        return 1;
    }

    ///- Get the list of realms for the server
    sRealmList.Initialize(sConfig.GetIntDefault("RealmsStateUpdateDelay", 20));
    if (sRealmList.size() == 0)
    {
        sLog.outError("No valid realms specified.");
        Log::WaitBeforeContinueIfNeed();
        return 1;
    }

    // cleanup query
    // set expired bans to inactive
    LoginDatabase.BeginTransaction();
    LoginDatabase.Execute("DELETE FROM banned_account WHERE UnBanDate<=UNIX_TIMESTAMP()");
    LoginDatabase.Execute("DELETE FROM banned_ip WHERE UnBanDate<=UNIX_TIMESTAMP()");
    LoginDatabase.CommitTransaction();

    // FIXME - more intelligent selection of thread count is needed here.  config option?
    MaNGOS::Listener<AuthSocket> listener(sConfig.GetStringDefault("BindIP", "0.0.0.0"), sConfig.GetIntDefault("RealmServerPort", DEFAULT_REALMSERVER_PORT), 1);

    ///- Catch termination signals
    HookSignals();

    ///- Handle affinity for multiple processors and process priority on Windows
#ifdef _WIN32
    {
        HANDLE hProcess = GetCurrentProcess();

        uint32 Aff = sConfig.GetIntDefault("UseProcessors", 0);
        if (Aff > 0)
        {
            ULONG_PTR appAff;
            ULONG_PTR sysAff;

            if (GetProcessAffinityMask(hProcess, &appAff, &sysAff))
            {
                ULONG_PTR curAff = Aff & appAff;            // remove non accessible processors

                if (!curAff)
                {
                    sLog.outError("Processors marked in UseProcessors bitmask (hex) %x not accessible for realmd. Accessible processors bitmask (hex): %x", Aff, appAff);
                }
                else
                {
                    if (SetProcessAffinityMask(hProcess, curAff))
                        sLog.outString("Using processors (bitmask, hex): %x", curAff);
                    else
                        sLog.outError("Can't set used processors (hex): %x", curAff);
                }
            }
            sLog.outString();
        }

        bool Prio = sConfig.GetBoolDefault("ProcessPriority", false);

        if (Prio)
        {
            if (SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS))
                sLog.outString("realmd process priority class set to HIGH");
            else
                sLog.outError("Can't set realmd process priority class.");
            sLog.outString();
        }
    }
#endif

    // server has started up successfully => enable async DB requests
    LoginDatabase.AllowAsyncTransactions();

    // maximum counter for next ping
    auto const numLoops = sConfig.GetIntDefault("MaxPingTime", 30) * MINUTE * 10;
    uint32 loopCounter = 0;

#ifndef _WIN32
    detachDaemon();
#endif
    ///- Wait for termination signal
    while (!stopEvent)
    {
        if ((++loopCounter) == numLoops)
        {
            loopCounter = 0;
            DETAIL_LOG("Ping MySQL to keep connection alive");
            LoginDatabase.Ping();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    ///- Wait for the delay thread to exit
    LoginDatabase.HaltDelayThread();

    ///- Remove signal handling before leaving
    UnhookSignals();

    std::cout << "Press enter to exit...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return 0;
}
