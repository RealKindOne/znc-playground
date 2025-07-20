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

#include <QSslSocket>
#include <QTcpServer>

using testing::HasSubstr;
using testing::Not;

namespace znc_inttest {
namespace {

TEST_F(ZNCTest, NotifyConnectModule) {
    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();
    client.Write("znc loadmod notify_connect");
    client.ReadUntil("Loaded module");

    auto client2 = ConnectClient();
    client2.Write("PASS :hunter2");
    client2.Write("NICK nick");
    client2.Write("USER user/test x x :x");
    client.ReadUntil("NOTICE nick :*** user attached from ");

    auto client3 = ConnectClient();
    client3.Write("PASS :hunter2");
    client3.Write("NICK nick");
    client3.Write("USER user@identifier/test x x :x");
    client.ReadUntil(
        "NOTICE nick :*** user@identifier attached from ");
    client2.ReadUntil(
        "NOTICE nick :*** user@identifier attached from ");

    client2.Write("QUIT");
    client.ReadUntil("NOTICE nick :*** user detached from ");

    client3.Close();
    client.ReadUntil(
        "NOTICE nick :*** user@identifier detached from ");
}

TEST_F(ZNCTest, ClientNotifyModule) {
    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();
    client.Write("znc loadmod clientnotify");
    client.ReadUntil("Loaded module");

    auto check_not_sent = [](Socket& client, QString wrongAnswer) {
        QString result = QString::fromUtf8(client.ReadRemainder());
        QRegularExpression expr(wrongAnswer);
        QRegularExpressionMatch match = expr.match(result);
        EXPECT_FALSE(match.hasMatch())
            << "Got an answer from the ClientNotifyModule even though we didnt "
               "want one with the given configuration: "
            << wrongAnswer.toStdString() << result.toStdString();
    };

    auto client2 = LoginClient();
    client.ReadUntilRe(R"(:Another client \((localhost)?\) authenticated as your user. Use the 'ListClients' command to see all 2 clients.)");
    auto client3 = LoginClient();
    client.ReadUntilRe(R"(:Another client \((localhost)?\) authenticated as your user. Use the 'ListClients' command to see all 3 clients.)");

    // disable notifications for every message
    client.Write("PRIVMSG *clientnotify :NewOnly on");

    // check that we do not ge a notification after connecting from a know ip
    auto client4 = LoginClient();
    check_not_sent(client, ":Another client (.*) authenticated as your user. Use the 'ListClients' command to see all 4 clients.");

    // choose to notify only on new client ids
    client.Write("PRIVMSG *clientnotify :NotifyOnNewID on");

    auto client5 = LoginClient("identifier123");
    client.ReadUntilRe(R"(:Another client \((localhost)? / identifier123\) authenticated as your user. Use the 'ListClients' command to see all 5 clients.)");
    auto client6 = LoginClient("identifier123");
    check_not_sent(client, ":Another client (.* / identifier123) authenticated as your user. Use the 'ListClients' command to see all 6 clients.");

    auto client7 = LoginClient("not_identifier123");
    client.ReadUntilRe(R"(:Another client \((localhost)? / not_identifier123\) authenticated as your user. Use the 'ListClients' command to see all 7 clients.)");

    // choose to notify from both clientids and new IPs
    client.Write("PRIVMSG *clientnotify :NotifyOnNewIP on");

    auto client8 = LoginClient();
    check_not_sent(client, ":Another client (.* / identifier123) authenticated as your user. Use the 'ListClients' command to see all 8 clients.");
    auto client9 = LoginClient("definitely_not_identifier123");
    client.ReadUntilRe(R"(:Another client \((localhost)? / definitely_not_identifier123\) authenticated as your user. Use the 'ListClients' command to see all 9 clients.)");
}

TEST_F(ZNCTest, ShellModule) {
    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();
    client.Write("znc loadmod shell");
    client.Write("PRIVMSG *shell :echo blahblah");
    client.ReadUntil("PRIVMSG nick :blahblah");
    client.ReadUntil("PRIVMSG nick :znc$");
}

TEST_F(ZNCTest, WatchModule) {
    // TODO test other messages
    // TODO test options
    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();
    client.Write("znc loadmod watch");
    client.Write("PRIVMSG *watch :add *");
    client.ReadUntil("Adding entry:");
    ircd.Write(":server 001 nick :Hello");
    ircd.Write(":nick JOIN :#znc");
    ircd.Write(":n!i@h PRIVMSG #znc :\001ACTION foo\001");
    client.ReadUntil(
        ":$*!watch@znc.in PRIVMSG nick :* CTCP: n [ACTION foo] to [#znc]");
    client.Write("PRIVMSG *watch :add * *spaces *word1 word2*");
    client.ReadUntil("Adding entry:");
    ircd.Write(":n!i@h PRIVMSG #znc :SOMETHING word1 word2 SOMETHING");
    client.ReadUntil(
        ":*spaces!watch@znc.in PRIVMSG nick :<n:#znc> SOMETHING word1 word2 SOMETHING");
}

TEST_F(ZNCTest, ModuleCrypt) {
    QFile conf(m_dir.path() + "/configs/znc.conf");
    ASSERT_TRUE(conf.open(QIODevice::Append | QIODevice::Text));
    QTextStream(&conf) << "ServerThrottle = 1\n";
    auto znc = Run();

    auto ircd1 = ConnectIRCd();
    auto client1 = LoginClient();
    client1.Write("znc loadmod controlpanel");
    client1.Write("PRIVMSG *controlpanel :CloneUser user user2");
    client1.ReadUntil("User user2 added!");
    client1.Write("PRIVMSG *controlpanel :Set Nick user2 nick2");
    client1.Write("znc loadmod crypt");
    client1.ReadUntil("Loaded module");

    auto ircd2 = ConnectIRCd();
    auto client2 = ConnectClient();
    client2.Write("PASS user2:hunter2");
    client2.Write("NICK nick2");
    client2.Write("USER user2/test x x :x");
    client2.Write("znc loadmod crypt");
    client2.ReadUntil("Loaded module");

    client1.Write("PRIVMSG *crypt :keyx nick2");
    client1.ReadUntil("Sent my DH1080 public key to nick2");

    QByteArray pub1("");
    ircd1.ReadUntilAndGet("NOTICE nick2 :DH1080_INIT ", pub1);
    ircd2.Write(":user!user@user/test " + pub1);

    client2.ReadUntil("Received DH1080 public key from user");
    client2.ReadUntil("Key for user successfully set.");

    QByteArray pub2("");
    ircd2.ReadUntilAndGet("NOTICE user :DH1080_FINISH ", pub2);
    ircd1.Write(":nick2!user2@user2/test " + pub2);

    client1.ReadUntil("Key for nick2 successfully set.");

    client1.Write("PRIVMSG *crypt :listkeys");
    QByteArray key1("");
    client1.ReadUntilAndGet("\002nick2\017: ", key1);
    client2.Write("PRIVMSG *crypt :listkeys");
    QByteArray key2("");
    client2.ReadUntilAndGet("\002user\017: ", key2);
    ASSERT_EQ(key1.mid(9), key2.mid(8));
    client1.Write("CAP REQ :echo-message");
    client1.Write("PRIVMSG .nick2 :Hello");
    QByteArray secretmsg;
    ircd1.ReadUntilAndGet("PRIVMSG nick2 :+OK ", secretmsg);
    ircd2.Write(":user!user@user/test " + secretmsg);
    client2.ReadUntil("Hello");
    client1.ReadUntil(secretmsg);  // by echo-message
}

TEST_F(ZNCTest, ModuleCSRFOverride) {
    // TODO: Qt 6.8 introduced QNetworkRequest::FullLocalServerNameAttribute to
    // let it connect to unix socket
    int port = PickPortNumber();
    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();
    client.Write(QStringLiteral("znc addport %1 all all").arg(port).toUtf8());
    client.Write("znc loadmod samplewebapi");
    client.ReadUntil("Loaded module");
    auto request = QNetworkRequest(
        QUrl(QStringLiteral("http://127.0.0.1:%1/mods/global/samplewebapi/")
                 .arg(port)));
    auto reply =
        HttpPost(request, {{"text", "ipsum"}})->readAll().toStdString();
    EXPECT_THAT(reply, HasSubstr("ipsum"));
}

TEST_F(ZNCTest, StripControlsModule) {
    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();

    client.Write("znc loadmod stripcontrols");
    client.ReadUntil("Loaded module");

    ircd.Write(":server 001 nick :Hello");
    client.Write(":nick JOIN #test");
    ircd.ReadUntil("JOIN #test");

    // OnChanCTCPMessage
    ircd.Write(":user!id@host PRIVMSG #test :\001\002bold\002 \003\034red\003 test\001");
    client.ReadUntil(":user!id@host PRIVMSG #test :\001bold red test\001");

    // OnChanNoticeMessage
    ircd.Write(":user!id@host NOTICE #test :\002bold\002 \003\034red\003 test");
    client.ReadUntil(":user!id@host NOTICE #test :bold red test");

    // OnChanTextMessage
    ircd.Write(":user!id@host PRIVMSG #test :\002bold\002 \003\034red\003 test");
    client.ReadUntil(":user!id@host PRIVMSG #test :bold red test");

    // OnPrivCTCPMessage
    ircd.Write(":user!id@host PRIVMSG nick :\001\002bold\002 \003\034red\003 test\001");
    client.ReadUntil(":user!id@host PRIVMSG nick :\001bold red test\001");

    // OnPrivNoticeMessage
    ircd.Write(":user!id@host NOTICE nick :\002bold\002 \003\034red\003 test");
    client.ReadUntil(":user!id@host NOTICE nick :bold red test");

    // OnPrivTextMessage
    ircd.Write(":user!id@host PRIVMSG nick :\002bold\002 \003\034red\003 test");
    client.ReadUntil(":user!id@host PRIVMSG nick :bold red test");

    // OnTopicMessage
    ircd.Write(":user!id@host TOPIC #test :\002bold\002 \003\034red\003 test");
    client.ReadUntil(":user!id@host TOPIC #test :bold red test");

    // OnNumericMessage
    // Topic from joining channel.
    ircd.Write("332 nick #test :\002bold\002 \003\034red\003 test");
    client.ReadUntil("332 nick #test :bold red test");

    // Topic from /list
    //ircd.Write("321 nick Channel :Users  Name]");
    ircd.Write("322 nick #test 42 :\002bold\002 \003\034red\003 test");
    client.ReadUntil("322 nick #test 42 :bold red test");

}

}  // namespace
}  // namespace znc_inttest
