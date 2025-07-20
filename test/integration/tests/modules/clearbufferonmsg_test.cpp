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

TEST_F(ZNCTest, ClearBufferOnMsgModule) {  
    auto znc = Run();  
    auto ircd = ConnectIRCd();  
    auto client = LoginClient();  
      
    // Load the module  
    client.Write("znc loadmod clearbufferonmsg");  
    client.ReadUntil("Loaded module");  
      
    // Set up IRC connection  
    ircd.Write(":server 001 nick :Hello");  
    ircd.Write(":nick JOIN :#test");  
    ircd.Write(":server 353 nick #test :nick");  
    ircd.Write(":server 366 nick #test :End of /NAMES list");  
      
    // Disconnect client to allow buffer accumulation  
    client.Close();  
      
    // Send messages to create buffer content  
    ircd.Write(":someone!user@host PRIVMSG #test :message1");  
    ircd.Write(":someone!user@host PRIVMSG #test :message2");  
      
    // Reconnect client  
    auto client2 = LoginClient();  
      
    // Verify buffer playback occurs  
    client2.ReadUntil("message1");  
    client2.ReadUntil("message2");  
      
    // Send a message from client (should trigger buffer clear)  
    client2.Write("PRIVMSG #test :user message");  
      
    // Disconnect and reconnect to check if buffers were cleared  
    client2.Close();  
    auto client3 = LoginClient();  
      
    // Add new message to buffer  
    ircd.Write(":someone!user@host PRIVMSG #test :new message");  
      
    // Verify only new message is in buffer (old ones were cleared)  
    client3.ReadUntil("new message");  
    // Should not receive the previous messages  
}

}  // namespace
}  // namespace znc_inttest
