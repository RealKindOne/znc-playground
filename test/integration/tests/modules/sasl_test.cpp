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

namespace znc_inttest {
namespace {

class SaslModuleTest : public ZNCTest,
                       public testing::WithParamInterface<
                           std::pair<int, std::vector<std::string>>> {
  public:
    static std::string Prefix() {
        std::string s;
        for (int i = 0; i < 33; ++i) s += "YWFh";
        s += "YQBh";
        for (int i = 0; i < 33; ++i) s += "YWFh";
        s += "AGJi";
        for (int i = 0; i < 31; ++i) s += "YmJi";
        EXPECT_EQ(s.length(), 396);
        return s;
    }

  protected:
    int PassLen() { return std::get<0>(GetParam()); }
    void ExpectPlainAuth(Socket& ircd) {
        for (const auto& str : std::get<1>(GetParam())) {
            QByteArray line;
            ircd.ReadUntilAndGet("AUTHENTICATE ", line);
            ASSERT_EQ(line.toStdString(), "AUTHENTICATE " + str);
        }
        ASSERT_EQ(ircd.ReadRemainder().indexOf("AUTHENTICATE"), -1);
    }
};

TEST_P(SaslModuleTest, Test) {
    QFile conf(m_dir.path() + "/configs/znc.conf");
    ASSERT_TRUE(conf.open(QIODevice::Append | QIODevice::Text));
    QTextStream(&conf) << "ServerThrottle = 1\n";
    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();
    client.Write("znc loadmod sasl");
    QByteArray sUser(100, 'a');
    QByteArray sPass(PassLen(), 'b');
    client.Write("PRIVMSG *sasl :set " + sUser + " " + sPass);
    client.Write("znc jump");
    ircd = ConnectIRCd();
    ircd.ReadUntil("CAP LS");
    ircd.Write("CAP * LS :away-notify sasl");
    ircd.ReadUntil("CAP REQ :away-notify sasl");
    ircd.Write("CAP * ACK :away-notify sasl");
    ircd.ReadUntil("AUTHENTICATE EXTERNAL");
    ircd.Write(":server 904 *");
    ircd.ReadUntil("AUTHENTICATE PLAIN");
    ircd.Write("AUTHENTICATE +");
    ExpectPlainAuth(ircd);
    ircd.Write(":server 903 user :Logged in");
    ircd.ReadUntil("CAP END");
}

INSTANTIATE_TEST_CASE_P(SaslInst, SaslModuleTest,
                        testing::Values(
                            std::pair<int, std::vector<std::string>>{
                                95, {SaslModuleTest::Prefix()}},
                            std::pair<int, std::vector<std::string>>{
                                96, {SaslModuleTest::Prefix() + "Yg==", "+"}},
                            std::pair<int, std::vector<std::string>>{
                                97, {SaslModuleTest::Prefix() + "YmI=", "+"}},
                            std::pair<int, std::vector<std::string>>{
                                98, {SaslModuleTest::Prefix() + "YmJi", "+"}},
                            std::pair<int, std::vector<std::string>>{
                                99,
                                {SaslModuleTest::Prefix() + "YmJi", "Yg=="}}));

TEST_F(ZNCTest, SaslMechsNotInit) {
    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();
    client.Write("znc loadmod sasl");
    client.Write("PRIVMSG *sasl :set * *");
    client.ReadUntil("Password has been set");
    ircd.Write("AUTHENTICATE +");
    ircd.Write("PING foo");
    ircd.ReadUntil("PONG foo");
}

TEST_F(ZNCTest, SaslRequire) {
    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();
    client.Write("znc loadmod sasl");
    client.Write("PRIVMSG *sasl :set * *");
    client.Write("PRIVMSG *sasl :requireauth yes");
    client.ReadUntil("Password has been set");
    client.Write("znc jump");
    ircd = ConnectIRCd();
    ircd.ReadUntil("CAP LS");
    ircd.Write(":server 001 nick :Hello");
    ircd.ReadUntil("QUIT :SASL not available");
    auto ircd2 = ConnectIRCd();
}

TEST_F(ZNCTest, SaslAuthPlainImapAuth) {
    auto znc = Run();
    auto ircd = ConnectIRCd();
    QTcpServer imap;
    ASSERT_TRUE(imap.listen(QHostAddress::LocalHost)) << imap.errorString().toStdString();
    auto client = LoginClient();
    client.Write(
        QStringLiteral("znc loadmod imapauth 127.0.0.1 %1 %@mail.test.com")
            .arg(imap.serverPort())
            .toUtf8());
    client.ReadUntil("Loaded");

    auto client2 = ConnectClient();
    client2.Write("NICK foo");
    client2.Write("CAP REQ :sasl");
    client2.Write("USER bar");
    client2.Write("AUTHENTICATE PLAIN");
    client2.Write("AUTHENTICATE " + QByteArrayLiteral("\0user@phone/net\0hunter3").toBase64());
    client2.ReadUntil("ACK :sasl");

    ASSERT_TRUE(imap.waitForNewConnection(30000 /* msec */));
    auto imapsock = WrapIO(imap.nextPendingConnection());
    imapsock.Write("* OK IMAP4rev1 Service Ready");
    imapsock.ReadUntil("AUTH LOGIN user@mail.test.com hunter3");
    imapsock.Write("AUTH OK");

    client2.ReadUntil(":irc.znc.in 903 foo :SASL authentication successful");
}

TEST_F(ZNCTest, SaslAuthExternal) {
    int port = PickPortNumber();

    auto znc = Run();
    auto ircd = ConnectIRCd();
    ircd.Write(":server 001 nick :Hello");
    auto client = LoginClient();
    client.Write(QStringLiteral("znc addport +%1 all all").arg(port).toUtf8());
    client.ReadUntil(":Port added");
    client.Write("znc loadmod certauth");
    client.ReadUntil("Loaded");
    client.Close();

    QSslSocket sock;
    // Could generate a new one for the test, but this one is good enough
    sock.setLocalCertificate(m_dir.path() + "/znc.pem");
    sock.setPrivateKey(m_dir.path() + "/znc.pem");
    sock.setPeerVerifyMode(QSslSocket::VerifyNone);
    sock.connectToHostEncrypted("127.0.0.1", port);
    ASSERT_TRUE(sock.waitForConnected()) << sock.errorString().toStdString();
    ASSERT_TRUE(sock.waitForEncrypted()) << sock.errorString().toStdString();
    auto client2 = WrapIO(&sock);
    client2.Write("PASS :hunter2");
    client2.Write("NICK nick");
    client2.Write("USER user/test x x :x");
    client2.Write("privmsg *certauth add");
    client2.ReadUntil("added");

    auto Reconnect = [&] {
        client2.Close();
        ASSERT_TRUE(sock.state() == QAbstractSocket::UnconnectedState || sock.waitForDisconnected())
            << sock.errorString().toStdString();
        sock.connectToHostEncrypted("127.0.0.1", port);
        ASSERT_TRUE(sock.waitForConnected())
            << sock.errorString().toStdString();
        ASSERT_TRUE(sock.waitForEncrypted())
            << sock.errorString().toStdString();
        client2.Write("CAP REQ sasl");
        client2.Write("NICK nick");
        client2.Write("USER u x x :x");
        client2.ReadUntil("ACK :sasl");
        client2.Write("AUTHENTICATE EXTERNAL");
        client2.ReadUntil("AUTHENTICATE +");
    };

    Reconnect();
    ircd.Write(":friend PRIVMSG nick :hello");
    client2.Write("AUTHENTICATE +");
    client2.ReadUntil(
        ":irc.znc.in 900 nick nick!user@127.0.0.1 user :You are now logged in "
        "as user");
    client2.ReadUntil(":irc.znc.in 903 nick :SASL authentication successful");
    client2.Write("CAP END");
    // '[' comes from lack of server-time
    client2.ReadUntil(":friend PRIVMSG nick :[");

    Reconnect();
    client2.Write("AUTHENTICATE " + QByteArrayLiteral("user/te").toBase64());
    client2.ReadUntil(
        ":irc.znc.in 900 nick nick!user@127.0.0.1 user :You are now logged in "
        "as user");
    client2.ReadUntil(":irc.znc.in 903 nick :SASL authentication successful");
    client2.Write("CAP END");
    client2.ReadUntil(
        ":*status!status@znc.in PRIVMSG nick :Network te doesn't exist.");

    Reconnect();
    client2.Write("AUTHENTICATE " + QByteArrayLiteral("moo").toBase64());
    client2.ReadUntil(
        ":irc.znc.in 904 nick :The specified user doesn't have this key");

    client = LoginClient();
    client.Write("privmsg *certauth :del 1");
    client.ReadUntil("Removed");
    Reconnect();
    client2.Write("AUTHENTICATE +");
    client2.ReadUntil(
        ":irc.znc.in 904 nick :Client cert not recognized");

    // Wrong mechanism
    auto client3 = ConnectClient();
    client3.Write("CAP LS 302");
    client3.Write("NICK nick");
    client3.ReadUntil(" sasl=EXTERNAL,PLAIN ");
    client3.Write("CAP REQ :sasl");
    client3.ReadUntil("ACK :sasl");
    client3.Write("AUTHENTICATE FOO");
    client3.ReadUntil(":irc.znc.in 908 nick EXTERNAL,PLAIN :are available SASL mechanisms");
    client3.ReadUntil(
        ":irc.znc.in 904 nick :SASL authentication failed");
}

}  // namespace
}  // namespace znc_inttest
