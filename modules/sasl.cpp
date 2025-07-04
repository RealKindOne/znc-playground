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

#include <znc/IRCNetwork.h>
#include <znc/IRCSock.h>
#include <znc/User.h>
#include <algorithm>

#define NV_REQUIRE_AUTH "require_auth"
#define NV_MECHANISMS "mechanisms"
#define NV_AUTO_REAUTH "auto_reauth"

class Mechanisms : public VCString {
  public:
    void SetIndex(unsigned int uiIndex) { m_uiIndex = uiIndex; }

    unsigned int GetIndex() const { return m_uiIndex; }

    bool HasNext() const { return size() > (m_uiIndex + 1); }

    void IncrementIndex() { m_uiIndex++; }

    CString GetCurrent() const { return at(m_uiIndex); }

    CString GetNext() const {
        if (HasNext()) {
            return at(m_uiIndex + 1);
        }

        return "";
    }

  private:
    unsigned int m_uiIndex = 0;
};

// Forward declaration
class CSASLMod;

class CSASLAutoReauthTimer : public CTimer {
  public:
    CSASLAutoReauthTimer(CSASLMod* pModule)
        : CTimer((CModule*)pModule, 1, 1, "AutoReauth",
                 "Automatic SASL re-authentication after CAP NEW"),
          m_pSASLMod(pModule) {}

  protected:
    void RunJob() override;

  private:
    CSASLMod* m_pSASLMod;
};

class CSASLMod : public CModule {
    const struct {
        const char* szName;
        CDelayedTranslation sDescription;
        bool bDefault;
    } SupportedMechanisms[2] = {
        {"EXTERNAL", t_d("TLS certificate, for use with the *cert module"),
         true},
        {"PLAIN",
         t_d("Plain text negotiation, this should work always if the "
             "network supports SASL"),
         true}};

  public:
    MODCONSTRUCTOR(CSASLMod) {
        AddCommand("Help", t_d("search"), t_d("Generate this output"),
                   [=](const CString& sLine) { PrintHelp(sLine); });
        AddCommand("Set", t_d("[<username> [<password>]]"),
                   t_d("Set username and password for the mechanisms that need "
                       "them. Password is optional. Without parameters, "
                       "returns information about current settings."),
                   [=](const CString& sLine) { Set(sLine); });
        AddCommand("Mechanism", t_d("[mechanism[ ...]]"),
                   t_d("Set the mechanisms to be attempted (in order)"),
                   [=](const CString& sLine) { SetMechanismCommand(sLine); });
        AddCommand("Reauth", "", t_d("Re-authenticate using SASL"),
                   [=](const CString& sLine) { ReauthCommand(sLine); });
        AddCommand("RequireAuth", t_d("[yes|no]"),
                   t_d("Don't connect unless SASL authentication succeeds"),
                   [=](const CString& sLine) { RequireAuthCommand(sLine); });
        AddCommand("Verbose", "yes|no", "Set verbosity level, useful to debug",
                   [&](const CString& sLine) {
                       m_bVerbose = sLine.Token(1, true).ToBool();
                       PutModule("Verbose: " + CString(m_bVerbose));
                   });
        AddCommand(
            "AutoReauth", t_d("[yes|no]"),
            t_d("Automatically re-authenticate when SASL becomes available"),
            [=](const CString& sLine) { AutoReauthCommand(sLine); });

        m_bAuthenticated = false;
        m_bReauthInProgress = false;
        m_bReauthInProgress = false;
    }

    void PrintHelp(const CString& sLine) {
        HandleHelpCommand(sLine);

        CTable Mechanisms;
        Mechanisms.AddColumn(t_s("Mechanism"));
        Mechanisms.AddColumn(t_s("Description"));
        Mechanisms.SetStyle(CTable::ListStyle);

        for (const auto& it : SupportedMechanisms) {
            Mechanisms.AddRow();
            Mechanisms.SetCell(t_s("Mechanism"), it.szName);
            Mechanisms.SetCell(t_s("Description"), it.sDescription.Resolve());
        }

        PutModule("");
        PutModule(t_s("The following mechanisms are available:"));
        PutModule(Mechanisms);
    }

    void Set(const CString& sLine) {
        if (sLine.Token(1).empty()) {
            CString sUsername = GetNV("username");
            CString sPassword = GetNV("password");

            if (sUsername.empty()) {
                PutModule(t_s("Username is currently not set"));
            } else {
                PutModule(t_f("Username is currently set to '{1}'")(sUsername));
            }
            if (sPassword.empty()) {
                PutModule(t_s("Password was not supplied"));
            } else {
                PutModule(t_s("Password was supplied"));
            }
            return;
        }

        SetNV("username", sLine.Token(1));
        SetNV("password", sLine.Token(2));

        PutModule(t_f("Username has been set to [{1}]")(GetNV("username")));
        PutModule(t_f("Password has been set to [{1}]")(GetNV("password")));
    }

    void ReauthCommand(const CString& sLine) {
        if (!GetNetwork()->IsIRCConnected()) {
            PutModule(t_s("Not connected to IRC server"));
            return;
        }

        if (m_bReauthInProgress) {
            PutModule(t_s("Re-authentication already in progress"));
            return;
        }

        StartReauth();
    }

    void AutoReauthCommand(const CString& sLine) {
        if (!sLine.Token(1).empty()) {
            SetNV(NV_AUTO_REAUTH, sLine.Token(1));
        }

        if (GetNV(NV_AUTO_REAUTH).ToBool()) {
            PutModule(t_s("Automatic re-authentication is enabled"));
        } else {
            PutModule(t_s("Automatic re-authentication is disabled"));
        }
    }

    void StartReauth() {
        if (m_Mechanisms.empty()) {
            GetMechanismsString().Split(" ", m_Mechanisms);
        }

        if (m_Mechanisms.empty()) {
            PutModule(t_s("No SASL mechanisms configured"));
            return;
        }

        m_bReauthInProgress = true;
        m_Mechanisms.SetIndex(0);

        if (m_bVerbose) {
            PutModule(t_f("Starting SASL re-authentication with {1}")(
                m_Mechanisms.GetCurrent()));
        }

        PutIRC("AUTHENTICATE " + m_Mechanisms.GetCurrent());
    }

    void SetMechanismCommand(const CString& sLine) {
        CString sMechanisms = sLine.Token(1, true).AsUpper();

        if (!sMechanisms.empty()) {
            VCString vsMechanisms;
            sMechanisms.Split(" ", vsMechanisms);

            for (const CString& sMechanism : vsMechanisms) {
                if (!SupportsMechanism(sMechanism)) {
                    PutModule("Unsupported mechanism: " + sMechanism);
                    return;
                }

                // Check if EXTERNAL mechanism is being set
                if (sMechanism.Equals("EXTERNAL")) {
                    CModule* pCertMod = nullptr;

                    // Check for certificate at network level.
                    pCertMod = GetNetwork()->GetModules().FindModule("cert");

                    // Check for certificate at user level if not loaded at
                    // network level.
                    if (!pCertMod) {
                        pCertMod =
                            GetNetwork()->GetUser()->GetModules().FindModule(
                                "cert");
                    }

                    if (!pCertMod) {
                        PutModule(
                            "Warning: EXTERNAL mechanism requires the 'cert' "
                            "module to be loaded as either a user or network "
                            "module for client certificate authentication.");
                    } else {
                        // Check if cert module has a user.pem file and print
                        // its location
                        CString sPemFile =
                            pCertMod->GetSavePath() + "/user.pem";
                        if (CFile::Exists(sPemFile)) {
                            PutModule("Certificate file location: " + sPemFile);
                        } else {
                            PutModule(
                                "Warning: EXTERNAL mechanism requires a "
                                "certificate file. The cert module has no "
                                "user.pem file. Use the cert module's web "
                                "interface or place a certificate at: " +
                                sPemFile);
                        }
                    }
                }
            }

            SetNV(NV_MECHANISMS, sMechanisms);
        }

        PutModule(t_f("Current mechanisms set: {1}")(GetMechanismsString()));
    }

    void RequireAuthCommand(const CString& sLine) {
        if (!sLine.Token(1).empty()) {
            SetNV(NV_REQUIRE_AUTH, sLine.Token(1));
        }

        if (GetNV(NV_REQUIRE_AUTH).ToBool()) {
            PutModule(t_s("We require SASL negotiation to connect"));
        } else {
            PutModule(t_s("We will connect even if SASL fails"));
        }
    }

    bool SupportsMechanism(const CString& sMechanism) const {
        for (const auto& it : SupportedMechanisms) {
            if (sMechanism.Equals(it.szName)) {
                return true;
            }
        }

        return false;
    }

    CString GetMechanismsString() const {
        if (GetNV(NV_MECHANISMS).empty()) {
            CString sDefaults = "";

            for (const auto& it : SupportedMechanisms) {
                if (it.bDefault) {
                    if (!sDefaults.empty()) {
                        sDefaults += " ";
                    }

                    sDefaults += it.szName;
                }
            }

            return sDefaults;
        }

        return GetNV(NV_MECHANISMS);
    }

    void CheckRequireAuth() {
        if (!m_bAuthenticated && GetNV(NV_REQUIRE_AUTH).ToBool()) {
            GetNetwork()->GetIRCSock()->Quit("SASL not available");
        }
    }

    void Authenticate(const CString& sLine) {
        if (m_Mechanisms.empty()) return;
        /* Send blank authenticate for other mechanisms (like EXTERNAL). */
        CString sAuthLine;
        if (m_Mechanisms.GetCurrent().Equals("PLAIN") && sLine.Equals("+")) {
            sAuthLine = GetNV("username") + '\0' + GetNV("username") + '\0' +
                        GetNV("password");
            sAuthLine.Base64Encode();
        }

        /* The spec requires authentication data to be sent in chunks */
        const size_t chunkSize = 400;
        for (size_t offset = 0; offset < sAuthLine.length();
             offset += chunkSize) {
            size_t size = std::min(chunkSize, sAuthLine.length() - offset);
            PutIRC("AUTHENTICATE " + sAuthLine.substr(offset, size));
        }
        if (sAuthLine.length() % chunkSize == 0) {
            /* Signal end if we have a multiple of the chunk size */
            PutIRC("AUTHENTICATE +");
        }
    }

    bool OnServerCapAvailable(const CString& sCap) override {
        if (sCap.Equals("sasl")) {
            // If we're already connected and authenticated, and SASL becomes
            // newly available, automatically attempt re-authentication if
            // enabled
            if (GetNV(NV_AUTO_REAUTH).ToBool() &&
                GetNetwork()->IsIRCConnected() && m_bAuthenticated &&
                !m_bReauthInProgress) {
                if (m_bVerbose) {
                    PutModule(
                        t_s("SASL capability became available, starting "
                            "re-authentication"));
                }
                AddTimer(new CSASLAutoReauthTimer(this));
                return true;
            }
        }
        return CONTINUE;
    }

    void OnServerCapResult(const CString& sCap, bool bSuccess) override {
        if (sCap.Equals("sasl")) {
            if (bSuccess) {
                GetMechanismsString().Split(" ", m_Mechanisms);

                if (m_Mechanisms.empty()) {
                    CheckRequireAuth();
                    return;
                }

                GetNetwork()->GetIRCSock()->PauseCap();

                m_Mechanisms.SetIndex(0);
                PutIRC("AUTHENTICATE " + m_Mechanisms.GetCurrent());
            } else {
                CheckRequireAuth();
            }
        }
    }

    EModRet OnRawMessage(CMessage& msg) override {
        if (msg.GetCommand().Equals("AUTHENTICATE")) {
            if (m_bReauthInProgress || !m_bAuthenticated)
                Authenticate(msg.GetParam(0));
            return HALT;
        }
        return CONTINUE;
    }

    EModRet OnNumericMessage(CNumericMessage& msg) override {
        if (m_Mechanisms.empty() && !m_bReauthInProgress) return CONTINUE;
        if (msg.GetCode() == 903) {
            /* SASL success! */
            if (m_bVerbose) {
                PutModule(
                    t_f("{1} mechanism succeeded.")(m_Mechanisms.GetCurrent()));
            }
            if (!m_bReauthInProgress) {
                GetNetwork()->GetIRCSock()->ResumeCap();
            } else {
                PutModule(t_s("SASL re-authentication successful"));
            }
            m_bAuthenticated = true;
            m_bReauthInProgress = false;
            DEBUG("sasl: Authenticated with mechanism ["
                  << m_Mechanisms.GetCurrent() << "]");
        } else if (msg.GetCode() == 904 || msg.GetCode() == 905) {
            DEBUG("sasl: Mechanism [" << m_Mechanisms.GetCurrent()
                                      << "] failed.");
            if (m_bVerbose) {
                PutModule(
                    t_f("{1} mechanism failed.")(m_Mechanisms.GetCurrent()));
            }

            if (m_Mechanisms.HasNext()) {
                m_Mechanisms.IncrementIndex();
                PutIRC("AUTHENTICATE " + m_Mechanisms.GetCurrent());
            } else {
                if (m_bReauthInProgress) {
                    PutModule(t_s("SASL re-authentication failed"));
                    m_bReauthInProgress = false;
                } else {
                    CheckRequireAuth();
                    GetNetwork()->GetIRCSock()->ResumeCap();
                }
            }
        } else if (msg.GetCode() == 906) {
            if (m_bReauthInProgress) m_bReauthInProgress = false;
            /* CAP wasn't paused? */
            DEBUG("sasl: Reached 906.");
            CheckRequireAuth();
        } else if (msg.GetCode() == 907) {
            m_bAuthenticated = true;
            GetNetwork()->GetIRCSock()->ResumeCap();
            DEBUG("sasl: Received 907 -- We are already registered");
        } else if (msg.GetCode() == 908) {
            return HALT;
        } else {
            return CONTINUE;
        }
        return HALT;
    }

    void OnIRCConnected() override {
        /* Just incase something slipped through, perhaps the server doesn't
         * respond to our CAP negotiation. */

        CheckRequireAuth();
    }

    void OnIRCDisconnected() override {
        m_bAuthenticated = false;
        m_bReauthInProgress = false;
    }

    CString GetWebMenuTitle() override { return t_s("SASL"); }

    bool OnWebRequest(CWebSock& WebSock, const CString& sPageName,
                      CTemplate& Tmpl) override {
        if (sPageName != "index") {
            // only accept requests to index
            return false;
        }

        if (WebSock.IsPost()) {
            SetNV("username", WebSock.GetParam("username"));
            CString sPassword = WebSock.GetParam("password");
            if (!sPassword.empty()) {
                SetNV("password", sPassword);
            }
            SetNV(NV_REQUIRE_AUTH, WebSock.GetParam("require_auth"));
            SetNV(NV_AUTO_REAUTH, WebSock.GetParam("auto_reauth"));
            SetNV(NV_MECHANISMS, WebSock.GetParam("mechanisms"));
        }

        Tmpl["Username"] = GetNV("username");
        Tmpl["Password"] = GetNV("password");
        Tmpl["RequireAuth"] = GetNV(NV_REQUIRE_AUTH);
        Tmpl["Mechanisms"] = GetMechanismsString();
        Tmpl["AutoReauth"] = GetNV(NV_AUTO_REAUTH);

        for (const auto& it : SupportedMechanisms) {
            CTemplate& Row = Tmpl.AddRow("MechanismLoop");
            CString sName(it.szName);
            Row["Name"] = sName;
            Row["Description"] = it.sDescription.Resolve();
        }

        return true;
    }

  private:
    Mechanisms m_Mechanisms;
    bool m_bAuthenticated;
    bool m_bReauthInProgress;
    bool m_bVerbose = false;
};

// Implementation of timer's RunJob method
void CSASLAutoReauthTimer::RunJob() { m_pSASLMod->StartReauth(); }

template <>
void TModInfo<CSASLMod>(CModInfo& Info) {
    Info.SetWikiPage("sasl");
}

NETWORKMODULEDEFS(CSASLMod, t_s("Adds support for sasl authentication "
                                "capability to authenticate to an IRC server"))
