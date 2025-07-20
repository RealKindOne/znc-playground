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

TEST_F(ZNCTest, ChanSaverModule) {
    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();

    client.Write("PRIVMSG *controlpanel :GetChan InConfig $me $network #test");
    client.ReadUntil("No channels matching [#test] found");

    client.Write("JOIN #test");
    ircd.Write(":server 001 nick :Hello");
    ircd.ReadUntil("JOIN #test");
    ircd.Write(":nick JOIN :#test");

    client.Write("PRIVMSG *controlpanel :GetChan InConfig $me $network #test");
    client.ReadUntil("InConfig = true");

    client.Write("PART #test");
    ircd.ReadUntil("PART #test");
    ircd.Write(":nick PART #test");

    client.Write("PRIVMSG *controlpanel :GetChan InConfig $me $network #test");
    client.ReadUntil("No channels matching [#test] found");
}

}  // namespace

}  // namespace znc_inttest
