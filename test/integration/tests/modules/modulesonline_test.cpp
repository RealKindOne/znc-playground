/*
 * Copyright (C) 2004-2025 ZNC, see the NOTICE file for details.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "znctest.h"

namespace znc_inttest {
namespace {

TEST_F(ZNCTest, ModulesOnlineModule) {
    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();
    client.Write("znc loadmod modules_online");
    client.ReadUntil("Loaded module modules_online");
    client.Write("znc loadmod --type=global log");
    client.ReadUntil("moddata/log/$USER/$NETWORK/$WINDOW/%Y-%m-%d.log");
    client.Write("znc loadmod --type=user log");
    client.ReadUntil("moddata/log/$NETWORK/$WINDOW/%Y-%m-%d.log");
    client.Write("znc loadmod --type=network log");
    client.ReadUntil("moddata/log/$WINDOW/%Y-%m-%d.log");
    client.Write("whois *log");
    client.ReadUntil(":*log (global,user,network)");
    client.Write("znc unloadmod --type=global log");
    client.ReadUntil("Module log unloaded.");
    client.Write("whois *log");
    client.ReadUntil(":*log (user,network)");
    client.Write("znc unloadmod --type=user log");
    client.ReadUntil("Module log unloaded.");
    client.Write("whois *log");
    client.ReadUntil(":*log (network)");
    client.Write("znc unloadmod --type=network log");
    client.ReadUntil("Module log unloaded.");
    client.Write("whois *log");
    client.ReadUntil("*log :No such nick/channel");
    client.Write("PRIVMSG *controlpanel :GetChan InConfig $me $network #test");
    client.ReadUntil("No channels matching [#test] found");
}

}  // namespace

}  // namespace znc_inttest
