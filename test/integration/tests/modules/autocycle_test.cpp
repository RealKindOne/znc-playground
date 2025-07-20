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

// The autocycle module has a 15 second delay per attempt.
// Just use a seperate test for QUIT, PART, and KICK.

TEST_F(ZNCTest, AutoCycleModuleQuit) {
    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();

    client.Write("znc loadmod autocycle");
    client.ReadUntil("Loaded module");

    ircd.Write(":server 001 nick :Hello");
    ircd.Write(":nick JOIN :#test");
    ircd.Write(":server 353 nick #test :nick @other user2");
    ircd.Write(":server 366 nick #test :End of /NAMES list");

    ircd.Write(":other QUIT :Connection lost");
    ircd.Write(":user2 QUIT :Bye");

    ircd.ReadUntil("PART #test");
    ircd.ReadUntil("JOIN #test");
}

TEST_F(ZNCTest, AutoCycleModulePart) {
    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();

    client.Write("znc loadmod autocycle");
    client.ReadUntil("Loaded module");

    ircd.Write(":server 001 nick :Hello");
    ircd.Write(":nick JOIN :#test");
    ircd.Write(":server 353 nick #test :nick @other user2");
    ircd.Write(":server 366 nick #test :End of /NAMES list");

    ircd.Write(":other PART #test :Leaving");
    ircd.Write(":user2 PART #test :Also leaving");

    ircd.ReadUntil("PART #test");
    ircd.ReadUntil("JOIN #test");
}

TEST_F(ZNCTest, AutoCycleModuleKick) {
    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();

    client.Write("znc loadmod autocycle");
    client.ReadUntil("Loaded module");

    ircd.Write(":server 001 nick :Hello");
    ircd.Write(":nick JOIN :#test");
    ircd.Write(":server 353 nick #test :nick @other user2");
    ircd.Write(":server 366 nick #test :End of /NAMES list");

    ircd.Write(":other KICK #test nick :You're out");
    ircd.Write(":nick JOIN :#test");
    ircd.Write(":server 353 nick #test :nick other user2");
    ircd.Write(":other PART #test :Done kicking");
    ircd.Write(":user2 PART #test :Me too");

    ircd.ReadUntil("PART #test");
    ircd.ReadUntil("JOIN #test");
}


}  // namespace
}  // namespace znc_inttest
