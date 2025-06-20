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

#include <znc/Modules.h>

class CStripControlsMod : public CModule {
  public:
    MODCONSTRUCTOR(CStripControlsMod) {}

    EModRet OnPrivCTCPMessage(CCTCPMessage& Message) override {
        CString sText = Message.GetText();
        sText.StripControls();
        Message.SetText(sText);
        return CONTINUE;
    }

    EModRet OnChanCTCPMessage(CCTCPMessage& Message) override {
        CString sText = Message.GetText();
        sText.StripControls();
        Message.SetText(sText);
        return CONTINUE;
    }

    EModRet OnPrivNoticeMessage(CNoticeMessage& Message) override {
        CString sText = Message.GetText();
        sText.StripControls();
        Message.SetText(sText);
        return CONTINUE;
    }

    EModRet OnChanNoticeMessage(CNoticeMessage& Message) override {
        CString sText = Message.GetText();
        sText.StripControls();
        Message.SetText(sText);
        return CONTINUE;
    }

    EModRet OnPrivTextMessage(CTextMessage& Message) override {
        CString sText = Message.GetText();
        sText.StripControls();
        Message.SetText(sText);
        return CONTINUE;
    }

    EModRet OnChanTextMessage(CTextMessage& Message) override {
        CString sText = Message.GetText();
        sText.StripControls();
        Message.SetText(sText);
        return CONTINUE;
    }

    EModRet OnTopicMessage(CTopicMessage& Message) override {
        CString sTopic = Message.GetTopic();
        sTopic.StripControls();
        Message.SetTopic(sTopic);
        return CONTINUE;
    }

    EModRet OnNumericMessage(CNumericMessage& Message) override {
        // Strip topic from /list
        if (Message.GetCode() == 322) {  // RPL_LIST
            CString sTopic = Message.GetParam(3);
            sTopic.StripControls();
            Message.SetParam(3, sTopic);
        }
        // Strip topic when joining channel
        else if (Message.GetCode() == 332) {  // RPL_TOPIC
            CString sTopic = Message.GetParam(2);
            sTopic.StripControls();
            Message.SetParam(2, sTopic);
        }
        return CONTINUE;
    }
};

    template <>
    void TModInfo<CStripControlsMod>(CModInfo& Info) {
        Info.SetWikiPage("stripcontrols");
        Info.AddType(CModInfo::UserModule);
    }

    NETWORKMODULEDEFS(
        CStripControlsMod,
        t_s("Strips control codes (Colors, Bold, ..) from channel "
            "and private messages."))
