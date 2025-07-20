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

TEST_F(ZNCTest, SendRawNonAdminSelfOnly) {
    auto znc = Run();
    auto ircd = ConnectIRCd();

    // Create admin user and non-admin user
    auto client1 = LoginClient();
    client1.Write("PRIVMSG *controlpanel :CloneUser user user2");
    client1.ReadUntil("User user2 added!");
    client1.Write("PRIVMSG *controlpanel :Set Admin user2 false");
    client1.ReadUntil("Admin = false");

    // Load send_raw module for both users
    client1.Write("PRIVMSG *controlpanel :LoadModule user send_raw");
    client1.ReadUntil("Loaded module");
    client1.Write("PRIVMSG *controlpanel :LoadModule user2 send_raw");
    client1.ReadUntil("Loaded module");

    // Connect as non-admin user
    auto client2 = ConnectClient();
    client2.Write("PASS :hunter2");
    client2.Write("NICK nick2");
    client2.Write("USER user2/test x x :x");

    // Test that non-admin user can send commands to themselves
    client2.Write("PRIVMSG *send_raw :Client user2 test PRIVMSG #test :hello");
    client2.ReadUntil("Sent [PRIVMSG #test :hello] to user2/test");

    client2.Write("PRIVMSG *send_raw :Server user2 test PING :test");
    client2.ReadUntil("Sent [PING :test] to IRC server of user2/test");

    // Test that non-admin user cannot send commands to other users
    client2.Write("PRIVMSG *send_raw :Client user test PRIVMSG #test :hello");
    client2.ReadUntil(":You need to have admin rights to modify other users");

    client2.Write("PRIVMSG *send_raw :Server user test PING :test");
    client2.ReadUntil(":You need to have admin rights to modify other users");

    // Test that admin can still send to anyone
    client1.Write("PRIVMSG *send_raw :Client user2 test PRIVMSG #test :admin_message");
    client1.ReadUntil("Sent [PRIVMSG #test :admin_message] to user2/test");

    // Test Current command works for non-admin (should always work since it's self-only)
    client2.Write("PRIVMSG *send_raw :Current PRIVMSG #test :current_test");
    client2.ReadUntil("PRIVMSG #test :current_test");
}


}  // namespace
}  // namespace znc_inttest