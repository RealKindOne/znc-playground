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

TEST_F(ZNCTest, AutoAttachModule) {
    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();
    InstallModule("testmod.cpp", R"(
        #include <znc/Client.h>
        #include <znc/Modules.h>
        class TestModule : public CModule {
          public:
            MODCONSTRUCTOR(TestModule) {}
            EModRet OnChanBufferPlayMessage(CMessage& Message) override {
                PutIRC("TEST " + Message.GetClient()->GetNickMask());
                return CONTINUE;
            }
        };
        MODULEDEFS(TestModule, "Test")
    )");
    client.Write("znc loadmod testmod");
    client.Write("PRIVMSG *controlpanel :Set AutoClearChanBuffer $me no");
    client.Write("znc loadmod autoattach");
    client.Write("PRIVMSG *autoattach :Add * * *");
    client.ReadUntil("Added to list");
    ircd.Write(":server 001 nick :Hello");
    ircd.Write(":nick JOIN :#znc");
    ircd.Write(":server 353 nick #znc :nick");
    ircd.Write(":server 366 nick #znc :End of /NAMES list");
    ircd.Write(":foo PRIVMSG #znc :hi");
    client.ReadUntil(":foo PRIVMSG");
    client.Write("detach #znc");
    client.ReadUntil("Detached");
    ircd.Write(":foo PRIVMSG #znc :hello");
    ircd.ReadUntil("TEST");
    client.ReadUntil("hello");
}

TEST_F(ZNCTest, AutoAttachSwapCommand) {
    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();

    client.Write("znc loadmod autoattach");
    client.ReadUntil("Loaded module");

    client.Write("PRIVMSG *autoattach :Add #test1 * *");
    client.ReadUntil("Added to list");
    client.Write("PRIVMSG *autoattach :Add #test2 * *");
    client.ReadUntil("Added to list");
    client.Write("PRIVMSG *autoattach :Add #test3 * *");
    client.ReadUntil("Added to list");

    client.Write("PRIVMSG *autoattach :List");
    client.ReadUntil("| 1     |     | #test1 |");
    client.ReadUntil("| 2     |     | #test2 |");
    client.ReadUntil("| 3     |     | #test3 |");

    client.Write("PRIVMSG *autoattach :Swap 1 3");
    client.ReadUntil("Swapped entries 1 and 3");

    client.Write("PRIVMSG *autoattach :List");
    client.ReadUntil("| 1     |     | #test3 |");
    client.ReadUntil("| 2     |     | #test2 |");
    client.ReadUntil("| 3     |     | #test1 |");

}

}  // namespace
}  // namespace znc_inttest
