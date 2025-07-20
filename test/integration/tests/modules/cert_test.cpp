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


TEST_F(ZNCTest, CertModuleInfoCommand) {
    // openssl req -nodes -sha512 -newkey rsa:4096 -keyout user.pem \
       -x509 -days 36500 -out user.pem -subj "/CN=YourNickname"
    QDir dir(m_dir.path());
    ASSERT_TRUE(dir.mkpath("users/user/networks/test/moddata/cert"));
    ASSERT_TRUE(dir.cd("users/user/networks/test/moddata/cert"));

    QFile certfile(dir.filePath("user.pem"));
    ASSERT_TRUE(certfile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&certfile);
    // clang-format off
    // OpenSSL cannot read the cert key if it has leading spaces.
    out << R"(-----BEGIN PRIVATE KEY-----
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
FX/XhyED++zzDUQ0GFKnKLTfw7RlnQ==
-----END PRIVATE KEY-----
-----BEGIN CERTIFICATE-----
MIIFETCCAvmgAwIBAgIUb576kHiPn0kSmwqba1BKuHIjGsUwDQYJKoZIhvcNAQEN
BQAwFzEVMBMGA1UEAwwMWW91ck5pY2tuYW1lMCAXDTI1MDcwODA5NDIxMFoYDzIx
MjUwNjE0MDk0MjEwWjAXMRUwEwYDVQQDDAxZb3VyTmlja25hbWUwggIiMA0GCSqG
SIb3DQEBAQUAA4ICDwAwggIKAoICAQCTM8XuA6zaLTkhTJvPkskxFI/87a4v8GpR
DX9APvnJyfuR7gYN5OBweBgqI1cAxxV3HKY3auLNSmXsyuOnloftRB/FdP9NTrEr
+EM1Q7Rif6QnYKM+BIoaWXJwdd01f1HDPU3w5ihnT1iZ77KF0xRI/krlqf+xUrbv
f/8PT4TEq2SUYsljeck0F+RBgqZYRBWb+//AnV/0leIkIllkibP8Nw49Njcx/SjB
z8YiOJVWGLGDWk20tYgNn1E4jDD7iDbw5vPonKta+wATjePtDDVP/eMRj1zKza7U
8N9++mBXOLKMH4CD1jSbfWplSlOPbWGIGlXXCzlNbKHVfSP9djlJca/ZiEm3iI+u
77FbH1L5y7J3/bb+UmUf/kAjbeMO8jXZlHtZvPpZWD4qEOwOczq9vkiYxZ4GE7Te
4AIXxiMr4ym4kUG/QG0K3zviL+t93Br1MGbZhd6o+s+E3d0Nw/yiedvg0Gff+g2J
Bhgame0I+VOAPpHAtxGJbV/mHh0lobO7gBqb5ZgOcY+X5aTwdDczxAjfXKq2tIS2
c1grPiBEJwSGDgc9Va44n1OoM2/cQ4YpfX+Nl4cSuR+pWpwwGfnVmr81S1rnzSOh
TZ+RQgF0H/QDB1F29ATD4p75+B4rjplxqeHjbtP0pSOWZHj86Zvy2iABJTE7pehG
XRHw7EF0AwIDAQABo1MwUTAdBgNVHQ4EFgQUZ3EaRQrU33cjRjdwJ/grAM80XLYw
HwYDVR0jBBgwFoAUZ3EaRQrU33cjRjdwJ/grAM80XLYwDwYDVR0TAQH/BAUwAwEB
/zANBgkqhkiG9w0BAQ0FAAOCAgEASDtVStXrykcDWg6SrRsizF/FeUD+fR3jC7vv
BaRqr105SeYl6h2pkaWXDTDQ9d/tSb3WrqcNMmdG6tYzUBZMUGe+gKL8lnMgkwj3
83su7JiavcXXg+13tggTPOPr80oOglVuvZDWxNRf8sf8nOEdKfZbzqVl10WpNanN
0/onqNFdNlaqJ330udOcTbrSf5mnZgn0NtCq2acqE+EoWiW1Ukr4QMuC8mqsbvXs
h+NRs8PKTkqRGyUJz7GEnpEET2oYv8rPdRMZr/OAODl6ObiDVazwVOskQtGYvwgM
Bto+IdXPr2mD6X79Dex+ktrLA0v2vrtaz3XDFwes1ahomLGCu1d0dzSwiRjK55zq
rHENgoxsfioM6tydMvrc+T232CMskDWFdwqMJ5KkpZsxGXSmzF9HbatCEGXxeb0m
kAvMsCV2dEKPDHstqLsWVGmmQIeeu8qAKRG/TNlsmJVBh+CN+QS8MdorzkbJ1m96
RnyhKXTOCUXvqGjkEFq1oLYJqVxBpFLSho82oKfNe7GZIp/qeBNEGRKlIIpfd5Kk
mOPBT33d5l0N/bnUz0DjMqKEiYLl9p+ce2NoszkuBNVfFIQDSsQbiUYWWjldJhfb
J3Thl0nrMKu5gP18hUKyONV9JPbNEpNvRzIg0Y6z25ropmROmm/ToiFijb3aIqw0
eNrYEMg=
-----END CERTIFICATE-----)";
    // clang-format on
    certfile.close();

    auto znc = Run();
    auto ircd = ConnectIRCd();
    auto client = LoginClient();

    client.Write("znc loadmod --type=network cert");
    client.ReadUntil("Loaded module");

    client.Write("PRIVMSG *cert :info");
    client.ReadUntil("7b:72:19:30:1d:c5:3f:f8:d1:7f:46:8e:06:e6"); // Part of SHA1
    client.ReadUntil("6b:b6:00:53:2f:3a:9e:d0:2f:1e:a3:7d:99:32"); // Part of SHA256
    client.ReadUntil("47:d3:5d:23:fb:32:09:14:19:c1:cd:2f:8e:a1"); // Part of SHA512

    client.Write("PRIVMSG *cert :info strip");
    client.ReadUntil("7b7219301dc53ff8d17f468e06e6"); // Part of SHA1
    client.ReadUntil("6bb600532f3a9ed02f1ea37d9932"); // Part of SHA256
    client.ReadUntil("47d35d23fb32091419c1cd2f8ea1"); // Part of SHA512
}


}  // namespace
}  // namespace znc_inttest