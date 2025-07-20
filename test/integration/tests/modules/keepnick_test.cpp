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

TEST_F(ZNCTest, KeepNickModule) {
    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();
    client.Write("znc loadmod keepnick");
    client.ReadUntil("Loaded module");
    ircd.ReadUntil("NICK user");
    ircd.Write(":server 433 * nick :Nickname is already in use.");
    ircd.ReadUntil("NICK user_");
    ircd.Write(":server 001 user_ :Hello");
    client.ReadUntil("Connected!");
    ircd.ReadUntil("NICK user");
    ircd.Write(":server 435 user_ user #error :Nope :-P");
    client.ReadUntil(
        ":*keepnick!keepnick@znc.in PRIVMSG user_ "
        ":Unable to obtain nick user: Nope :-P, #error");
    // Disabled from above. Renable.
    client.Write("PRIVMSG *keepnick :state");
    client.ReadUntil("Currently disabled");
    client.Write("PRIVMSG *keepnick :enable");
    client.ReadUntil("Trying to get");
    client.Write("JOIN #test");
    ircd.ReadUntil("JOIN #test");
    ircd.Write(":server 353 nick #test :user_ user");
    ircd.Write(":server 366 nick #test :End of /NAMES list");
    ircd.Write(":user QUIT :Leaving");
    ircd.ReadUntil("NICK user");

}

}  // namespace
}  // namespace znc_inttest
