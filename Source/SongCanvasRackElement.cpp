//
// Created by kendo on 13/03/2026.
//
#pragma once
#include "ModularSynth.h"
#include "SongCanvas.h"

std::string TruncateString(std::string str, size_t width, bool show_ellipsis = true)
{
   if (str.length() > width)
   {
      if (show_ellipsis)
      {
         str.resize(width);
         return str.append("...");
      }
      else
      {
         str.resize(width);
         return str;
      }
   }
   return str;
}

std::string RemoveNonNumericalChars(const std::string& input)
{
   std::string result;
   for (char c : input)
   {
      if (c >= '0' && c <= '9')
      {
         result += c;
      }
   }
   return result;
}


SongCanvasRackElement::SongCanvasRackElement(float preferredWidth, SongCanvasElementVariant variantType, std::string name, SongCanvas* owner, const ofColor& overrideColor)
: UIFlowGridElement(preferredWidth, overrideColor)
{
   mElementName = new std::string(name);
   mSongCanvas = owner;
   mVariantType = variantType;
   mInternalRackID = mSongCanvas->GetInternalRackId();
   switch (variantType)
   {
      case SongCanvasElementVariant::Enabler:
         SetColor(ofColor::white);
         break;
      case SongCanvasElementVariant::Pulser:
         SetColor(ofColor::yellow);
         mVariantExtraWidth = 50;
         mTransportListenerInfo = TheTransport->AddListener(this, mPulserInterval, OffsetInfo(0, true), true);
         break;
      case SongCanvasElementVariant::LFO:

         break;
      case SongCanvasElementVariant::Sampler:

         break;
      case SongCanvasElementVariant::OnePulse:
         SetColor(ofColor(150, 150, 0));
         break;
   }
   CreateUIControls(owner);
   mHighlightOutlineColor = ofColor(0, 150, 255);
}

SongCanvasRackElement::~SongCanvasRackElement()
{
   switch (mVariantType)
   {
      case SongCanvasElementVariant::Enabler:
         mSongCanvas->RemovePatchCableSource(mEnablerCable);
         break;
      case SongCanvasElementVariant::Pulser:
         mSongCanvas->RemovePatchCableSource(mPulserCable);
         TheTransport->RemoveListener(this);
         mSongCanvas->DisposeElement(mIntervalSelector);
         mIntervalSelector->Delete();
         break;
      case SongCanvasElementVariant::LFO:
         break;
      case SongCanvasElementVariant::Sampler: break;
      case SongCanvasElementVariant::OnePulse:
         mSongCanvas->RemovePatchCableSource(mPulserCable);
         break;
      default:;
   }
}

void SongCanvasRackElement::CreateUIControls(SongCanvas* owner)
{
   switch (mVariantType)
   {
      case SongCanvasElementVariant::Enabler:
         mEnablerCable = new PatchCableSource(owner, kConnectionType_UIControl);
         owner->AddPatchCableSource(mEnablerCable);
         mEnablerCable->SetAllowMultipleTargets(true);
         break;
      case SongCanvasElementVariant::Pulser:
         mPulserCable = new PatchCableSource(owner, kConnectionType_Pulse);
         owner->AddPatchCableSource(mPulserCable);
         mPulserCable->SetAllowMultipleTargets(true);

         mIntervalSelector = new DropdownList(owner, "interval", 75, 2, (int*)(&mPulserInterval));

         mIntervalSelector->AddLabel("16", kInterval_16);
         mIntervalSelector->AddLabel("8", kInterval_8);
         mIntervalSelector->AddLabel("4", kInterval_4);
         mIntervalSelector->AddLabel("3", kInterval_3);
         mIntervalSelector->AddLabel("2", kInterval_2);
         mIntervalSelector->AddLabel("1n", kInterval_1n);
         mIntervalSelector->AddLabel("2n", kInterval_2n);
         mIntervalSelector->AddLabel("4n", kInterval_4n);
         mIntervalSelector->AddLabel("4nt", kInterval_4nt);
         mIntervalSelector->AddLabel("8n", kInterval_8n);
         mIntervalSelector->AddLabel("8nt", kInterval_8nt);
         mIntervalSelector->AddLabel("16n", kInterval_16n);
         mIntervalSelector->AddLabel("16nt", kInterval_16nt);
         mIntervalSelector->AddLabel("32n", kInterval_32n);
         mIntervalSelector->AddLabel("64n", kInterval_64n);
         mIntervalSelector->AddLabel("none", kInterval_None);
         mIntervalSelector->AddLabel("div", kInterval_CustomDivisor);
         break;
      case SongCanvasElementVariant::LFO: break;
      case SongCanvasElementVariant::Sampler: break;
      case SongCanvasElementVariant::OnePulse:
         mPulserCable = new PatchCableSource(owner, kConnectionType_Pulse);
         owner->AddPatchCableSource(mPulserCable);
         mPulserCable->SetAllowMultipleTargets(true);
         break;
      default:;
   }

   mElementRenameTextBox = owner->GetRackRenameTextbox();
   //mElementRenameTextBox->
   //mElementRenameTextBox = new TextEntry(owner, "", 32, 32, 8, &mElementName);
   //mTransportTextBox = new TextEntry{ this, "", startCanvasOffset - 68, 22, 6, &mTime, 0, 99999 };
}

void SongCanvasRackElement::Draw()
{
   UIFlowGridElement::Draw();

   ofPushStyle();
   ofSetColor(ofColor::white);
   if (mExciteConstant > 0) //Make the outline bounce for extra visual satisfaction.
   {
      float excConst = mExciteConstant + sin(ofGetGlobalTime() * 12) * 0.2F;
      if (mExcitePower < excConst)
         mExcitePower = excConst;
   }
   mExcitePower = MAX(0, mExcitePower - ofGetLastFrameTime() * 2);
   mExciteDrag = ofLerp(mExciteDrag, mExcitePower, ofGetLastFrameTime() * 12);
   mOutlineThickness = 0.8F + mExciteDrag * 1.2;

   auto rPos = GetRelativePosition();
   auto pos = GetFlowGrid()->GetPosition(true);
   float compressRefit = 12; //Offset so it's not at the element edge.
   if (mWidth < 90)
   { //Compensate for high compression
      compressRefit *= static_cast<float>(mWidth) / 90;
   }
   switch (mVariantType)
   {
      case SongCanvasElementVariant::Enabler:
         mEnablerCable->SetManualPosition(rPos.x + mWidth - compressRefit, rPos.y + mHeight / 2);
         break;
      case SongCanvasElementVariant::Pulser:
         mPulserCable->SetManualPosition(rPos.x + mWidth - compressRefit, rPos.y + mHeight / 2);
         ofPushMatrix();
         ofTranslate(-pos.x, -pos.y);
         mIntervalSelector->SetPosition(rPos.x + mWidth - compressRefit - 53, rPos.y + 7);
         mIntervalSelector->Draw();
         ofPopMatrix();

         break;
      case SongCanvasElementVariant::LFO:
         break;
      case SongCanvasElementVariant::Sampler:
         break;
      case SongCanvasElementVariant::OnePulse:
         mPulserCable->SetManualPosition(rPos.x + mWidth - compressRefit, rPos.y + mHeight / 2);
         break;
   }

   if (mRenameActive)
   {
      //mElementRenameTextBox->SetPosition(rPos.x, rPos.y + mHeight / 2);
      ofPushMatrix();
      ofTranslate(-pos.x, -pos.y);
      mElementRenameTextBox->SetPosition(rPos.x + 4, rPos.y + 7);
      mElementRenameTextBox->Draw();

      ofPopMatrix();

      int form = (static_cast<std::string>(mElementRenameTextBox->GetText()).size() - 10) * 6;
      if (form < 0)
         form = 0;
      SetPreferredSize(90 + form + mVariantExtraWidth);

      GetFlowGrid()->RecalculateElements();

      if (mElementRenameTextBox->GetActiveKeyboardFocus() != mElementRenameTextBox)
      {
         mSongCanvas->SetRackElementRenameState(this, false);
      }
   }
   else
   {
      int textSize = (mWidth - 15) / 7.0;
      std::string displayString;
      if (textSize <= 3)
      {
         displayString = RemoveNonNumericalChars(mElementName->c_str());
         if (displayString.empty())
            displayString = TruncateString(mElementName->c_str(), textSize, true);
      }
      else
      {
         displayString = TruncateString(mElementName->c_str(), textSize, true);
      }

      if (mWidth > 20)
         DrawTextNormal(displayString, mX + 8, mY + mHeight / 2 + 5.5);
   }

   ofPopStyle();
}

void SongCanvasRackElement::OnMouseClick(bool rightClick)
{
   if (rightClick)
   {
      auto p = GetRelativePosition();
      mSongCanvas->SetNewRackDropdownContext(this);
      mSongCanvas->GetRackRightClickDropdown()->SetPosition(p.x, p.y);
      mSongCanvas->GetRackRightClickDropdown()->OnClicked(1, 1, false);
      mSongCanvas->GetRackRightClickDropdown()->SetPosition(-500, -500);
   }
   else
   {
      mSongCanvas->SetSelectedRackElement(this);
      if (TheSynth->GetGlobalTime() < mLastClickTime + 0.5)
      {
         mSongCanvas->SetRackElementRenameState(this, true);
         mLastClickTime = 0;
      }
      else
      {
         mLastClickTime = TheSynth->GetGlobalTime();
      }
   }
}
void SongCanvasRackElement::SetName(std::string newName) const
{
   *mElementName = newName;
}

void SongCanvasRackElement::OnEnter()
{
   mActive = true;
   double time = NextBufferTime(mSongCanvas);
   switch (mVariantType)
   {
      case SongCanvasElementVariant::Enabler:

         mEnablerCable->AddHistoryEvent(time, true, 0);
         Excite(1);
         SetExciteConstant(0.6);
         for (auto* cable : mEnablerCable->GetPatchCables())
         {
            IUIControl* uicontrol = dynamic_cast<IUIControl*>(cable->GetTarget());
            if (uicontrol)
            {
               uicontrol->SetValue(1, time);
            }
         }

         break;
      case SongCanvasElementVariant::Pulser:

         break;


      case SongCanvasElementVariant::LFO: break;
      case SongCanvasElementVariant::Sampler: break;
      case SongCanvasElementVariant::OnePulse:
         const std::vector<IPulseReceiver*>& receivers = mPulserCable->GetPulseReceivers();
         mPulserCable->AddHistoryEvent(time, true, 0);
         mPulserCable->AddHistoryEvent(time + 15, false);
         Excite(1);
         for (auto* receiver : receivers)
            receiver->OnPulse(time, 1, 0);
         break;
   }
}
void SongCanvasRackElement::OnProcess()
{
   switch (mVariantType)
   {
      case SongCanvasElementVariant::Enabler:
         break;
      case SongCanvasElementVariant::Pulser:
         break;
      case SongCanvasElementVariant::LFO:
         break;
      case SongCanvasElementVariant::Sampler:
         break;
      case SongCanvasElementVariant::OnePulse:
         break;
      default:;
   }
}
void SongCanvasRackElement::OnExit()
{
   mActive = false;
   switch (mVariantType)
   {
      case SongCanvasElementVariant::Enabler:
         mEnablerCable->AddHistoryEvent(NextBufferTime(mSongCanvas), false, 0);
         SetExciteConstant(0);

         for (auto* cable : mEnablerCable->GetPatchCables())
         {
            IUIControl* uicontrol = dynamic_cast<IUIControl*>(cable->GetTarget());
            if (uicontrol)
            {
               uicontrol->SetValue(0, NextBufferTime(mSongCanvas));
            }
         }
         break;
      case SongCanvasElementVariant::Pulser: break;
      case SongCanvasElementVariant::LFO: break;
      case SongCanvasElementVariant::Sampler: break;
      case SongCanvasElementVariant::OnePulse: break;
      default:;
   }
}

void SongCanvasRackElement::OnTimeEvent(double time)
{
   if (IsActive() && mSongCanvas->IsEnabled())
   {
      const std::vector<IPulseReceiver*>& receivers = mPulserCable->GetPulseReceivers();
      mPulserCable->AddHistoryEvent(time, true, 0);
      mPulserCable->AddHistoryEvent(time + 15, false);
      Excite(1);
      for (auto* receiver : receivers)
         receiver->OnPulse(time, 1, 0);
   }
}
