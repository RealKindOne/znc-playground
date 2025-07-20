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

TEST_F(ZNCTest, WatchModule) {
    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();

    client.Write("znc loadmod watch");
    client.ReadUntil("Loaded module");

    client.Write("PRIVMSG *watch :add *");
    client.ReadUntil("Adding entry:");

    client.Write("PRIVMSG *watch :add testuser!*@* $testuser");
    client.ReadUntil("Adding entry:");

    client.Write("PRIVMSG *watch :add * *pattern *important*");
    client.ReadUntil("Adding entry:");

    ircd.Write(":server 001 nick :Hello");
    ircd.Write(":nick JOIN :#znc");
    ircd.Write(":nick JOIN :#test");

    // OnJoinMessage()
    ircd.Write(":testuser!test@host JOIN :#znc");
    client.ReadUntil(":$*!watch@znc.in PRIVMSG nick :* testuser (test@host) joins #znc");
    client.ReadUntil(":$testuser!watch@znc.in PRIVMSG nick :* testuser (test@host) joins #znc");

    // OnPartMessage()
    ircd.Write(":testuser!test@host PART #znc :Leaving");
    client.ReadUntil(":$*!watch@znc.in PRIVMSG nick :* testuser (test@host) parts #znc(Leaving)");
    client.ReadUntil(":$testuser!watch@znc.in PRIVMSG nick :* testuser (test@host) parts #znc(Leaving)");

    // OnQuitMessage()
    ircd.Write(":testuser!test@host JOIN :#znc");
    ircd.Write(":testuser!test@host QUIT :Goodbye");
    client.ReadUntil(":$*!watch@znc.in PRIVMSG nick :* Quits: testuser (test@host) (Goodbye)");
    client.ReadUntil(":$testuser!watch@znc.in PRIVMSG nick :* Quits: testuser (test@host) (Goodbye)");

    // OnNickMessage()
    ircd.Write(":anotheruser!test@host NICK :newname");
    client.ReadUntil(":$*!watch@znc.in PRIVMSG nick :* anotheruser is now known as newname");

    // OnPrivMessage()
    ircd.Write(":testuser!test@host PRIVMSG nick :Hello there");
    client.ReadUntil(":$*!watch@znc.in PRIVMSG nick :<testuser> Hello there");
    client.ReadUntil(":$testuser!watch@znc.in PRIVMSG nick :<testuser> Hello there");

    // OnChanMessage()
    ircd.Write(":someuser!test@host PRIVMSG #znc :This is important stuff");
    client.ReadUntil(":$*!watch@znc.in PRIVMSG nick :<someuser:#znc> This is important stuff");
    client.ReadUntil(":*pattern!watch@znc.in PRIVMSG nick :<someuser:#znc> This is important stuff");

    // OnPrivNoticeMessage()
    ircd.Write(":testuser!test@host NOTICE nick :Private notice");
    client.ReadUntil(":$*!watch@znc.in PRIVMSG nick :-testuser- Private notice");
    client.ReadUntil(":$testuser!watch@znc.in PRIVMSG nick :-testuser- Private notice");

    // OnChanNoticeMessage()
    ircd.Write(":testuser!test@host NOTICE #znc :Channel notice");
    client.ReadUntil(":$*!watch@znc.in PRIVMSG nick :-testuser:#znc- Channel notice");
    client.ReadUntil(":$testuser!watch@znc.in PRIVMSG nick :-testuser:#znc- Channel notice");

    // OnChanCTCPMessage()
    ircd.Write(":testuser!test@host PRIVMSG #znc :\001VERSION\001");
    client.ReadUntil(":$*!watch@znc.in PRIVMSG nick :* CTCP: testuser [VERSION] to [#znc]");
    client.ReadUntil(":$testuser!watch@znc.in PRIVMSG nick :* CTCP: testuser [VERSION] to [#znc]");

    // OnPrivCTCPMessage()
    ircd.Write(":testuser!test@host PRIVMSG nick :\001TIME\001");
    client.ReadUntil(":$*!watch@znc.in PRIVMSG nick :* CTCP: testuser [TIME]");
    client.ReadUntil(":$testuser!watch@znc.in PRIVMSG nick :* CTCP: testuser [TIME]");

    // OnRawMode()
    ircd.Write(":testuser!test@host MODE #znc +o nick");
    client.ReadUntil(":$*!watch@znc.in PRIVMSG nick :* testuser sets mode: +o nick on #znc");
    client.ReadUntil(":$testuser!watch@znc.in PRIVMSG nick :* testuser sets mode: +o nick on #znc");

    // OnKickMessage()
    ircd.Write(":testuser!test@host KICK #znc person :Test Kick");
    client.ReadUntil(":$*!watch@znc.in PRIVMSG nick :* testuser kicked person from #znc because [Test Kick]");
    client.ReadUntil(":$testuser!watch@znc.in PRIVMSG nick :* testuser kicked person from #znc because [Test Kick]");

    client.Write("PRIVMSG *watch :disable 2");
    client.ReadUntil("Id 2 disabled");

    // Test that disabled entry doesn't trigger
    ircd.Write(":testuser!test@host PRIVMSG nick :Another message");
    client.ReadUntil(":$*!watch@znc.in PRIVMSG nick :<testuser> Another message");
    // Should not get notification from disabled $testuser entry

    client.Write("PRIVMSG *watch :SetDetachedClientOnly 1 true");
    client.ReadUntil("Id 1 set to Yes");

    client.Write("PRIVMSG *watch :SetSources 3 #znc !#test");
    client.ReadUntil("Sources set for Id 3");

    // Fix for #451 - Ignore Quit if channel is excluded.
    ircd.Write(":testuser!test@host JOIN #test");
    ircd.Write(":testuser!test@host QUIT :Test important Test");
    ASSERT_THAT(client.ReadRemainder().toStdString(),
        Not(HasSubstr("*pattern!watch@znc.in PRIVMSG nick :* Quits: testuser")));

    client.Write("PRIVMSG *watch :list");
    client.ReadUntil("| 3  |");




}

}  // namespace
}  // namespace znc_inttest
