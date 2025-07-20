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

using testing::HasSubstr;
using testing::Not;

namespace znc_inttest {
namespace {

TEST_F(ZNCTest, StickyChannelModule) {
    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();

    client.Write("znc loadmod stickychan");
    client.ReadUntil("Loaded module");

    client.Write("PRIVMSG *stickychan :stick #sticky");
    client.ReadUntil("Stuck #stick");

    ircd.Write("001 nick Welcome");
    ircd.Write(":nick JOIN :#sticky");

    client.Write("PART #sticky :leaving");
    ircd.ReadUntil("JOIN #sticky");

    ASSERT_THAT(ircd.ReadRemainder().toStdString(), Not(HasSubstr("PART #sticky")));

    client.Write("PRIVMSG *controlpanel :GetChan InConfig $me $network #sticky");
    client.ReadUntil("InConfig = true");

    client.Write("JOIN #banned");
    ircd.ReadUntil("JOIN #banned");

    ircd.Write(":server 479 nick #banned :Cannot join channel (+b)");
    client.ReadUntil(":server 479 nick #banned :Cannot join channel (+b)");

    client.Write("PRIVMSG *controlpanel :GetChan InConfig $me $network #banned");
    client.ReadUntil("#banned: InConfig = false");
}


}  // namespace
}  // namespace znc_inttest

