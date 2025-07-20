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

TEST_F(ZNCTest, PerformQuoteCleanup) {
    // Create a test registry file with quote prefix before starting ZNC
    QDir dir(m_dir.path());
    ASSERT_TRUE(dir.mkpath("users/user/networks/test/moddata/perform"));
    ASSERT_TRUE(dir.cd("users/user/networks/test/moddata/perform"));

    QFile registry(dir.filePath(".registry"));
    ASSERT_TRUE(registry.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&registry);
    out << "Perform quote%20;oper%20;test1%20;password%20;%0a;raw%20;oper%20;aaa2%20;password%20;%0a;\n";
    registry.close();

    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();

    client.Write("znc loadmod perform");
    client.ReadUntil("Loaded module");

    // Verify the command was cleaned up by checking what gets sent to IRC
    ircd.Write(":server 001 nick :Hello");
    ircd.ReadUntil("oper test1 password");


    ASSERT_TRUE(registry.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = QTextStream(&registry).readAll();
    EXPECT_THAT(content.toStdString(), Not(HasSubstr("quote")));
    EXPECT_THAT(content.toStdString(), HasSubstr("Perform oper%20;test1%20;password%20;%0a;"));
}


}  // namespace
}  // namespace znc_inttest
