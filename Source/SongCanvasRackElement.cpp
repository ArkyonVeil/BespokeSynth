//
// Created by ArkyonVeil on 13/03/2026.
//
///
/// How this works in a nutshell:
///
/// Rack Parts exist as overrides of FlowGrid elements
/// Each Rack part consists of its own subclass, akin to submodules of the Song Canvas
///
/// SongCanvas_CanvasElements are the parts as present in the canvas.
/// They all derive from the same class, but have their behavior customized by calling custom setups/drawing of their rack counterparts.
/// This means that you only really need to mostly work on the Rack Elements for new behavior! <>]

#pragma once
#include "ModularSynth.h"
#include "SongCanvas.h"
#include "Sample.h"

#include "juce_audio_formats/juce_audio_formats.h"
using namespace juce;

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


SongCanvasRackElement::SongCanvasRackElement(std::string partName, SongCanvas* songCanvas)
: FlowGridElement(mFlowGridParent)
{
   mElementName = new std::string(partName);
   mSongCanvas = songCanvas;
   mInternalRackID = mSongCanvas->GetInternalRackId();
   mHighlightOutlineColor = ofColor(0, 150, 255);
}


void SongCanvasRackElement::CreateUIControls()
{
   IDrawableModule::CreateUIControls();
   mElementRenameTextBox = mSongCanvas->GetRackRenameTextbox();
}


void SongCanvasRackElement::OnMouseClick(bool rightClick)
{
   FlowGridElement::OnMouseClick(rightClick);

   if (rightClick)
   {
      mSongCanvas->OpenRightClickRackMenu(this);
   }
   else
   {
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
void SongCanvasRackElement::SetPartName(std::string newName) const
{
   *mElementName = newName;
}


void SongCanvasRackElement::DrawModule()
{
   FlowGridElement::DrawModule();

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

   //Unique rack graphics are drawn here...
   DrawRackGraphics();

   if (mRenameActive)
   {
      //mElementRenameTextBox->SetPosition(rPos.x, rPos.y + mHeight / 2);
      mElementRenameTextBox->SetPosition( 4, 7);
      mElementRenameTextBox->Draw();

      int form = (static_cast<std::string>(mElementRenameTextBox->GetText()).size() - 10) * 6;
      if (form < 0)
         form = 0;
      SetPreferredSize(90 + form);
      GetFlowGrid()->RecalculateFlowGrid();

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
         DrawTextNormal(displayString,8, mHeight / 2 + 5.5);
   }
   ofPopStyle();
}

/////////////
///Enabler///
/////////////

SongCanvasRackEnabler::SongCanvasRackEnabler(const std::string& partName, SongCanvas* songCanvas)
: SongCanvasRackElement(partName, songCanvas)
{
   SetColor(ofColor::white);
}
SongCanvasRackEnabler::~SongCanvasRackEnabler()
{
   mSongCanvas->RemovePatchCableSource(mEnablerCable);
}
void SongCanvasRackEnabler::CreateUIControls()
{
   SongCanvasRackElement::CreateUIControls();
   mEnablerCable = new PatchCableSource(this, kConnectionType_UIControl);
   this->AddPatchCableSource(mEnablerCable);
   mEnablerCable->SetAllowMultipleTargets(true);
}


void SongCanvasRackEnabler::OnEnter()
{
   double time = NextBufferTime(mSongCanvas);

   mEnablerCable->AddHistoryEvent(time, true, 0);

   Excite(1);
   SetExciteConstant(0.6);
   for (auto* cable : mEnablerCable->GetPatchCables())
   {
      IUIControl* uicontrol = dynamic_cast<IUIControl*>(cable->GetTarget());
      if (uicontrol)
      {
         uicontrol->SetValue(!mEnablerInverted, time);
      }
   }
}
void SongCanvasRackEnabler::OnExit()
{
   double time = NextBufferTime(mSongCanvas);
   mEnablerCable->AddHistoryEvent(time, false, 0);

   SetExciteConstant(0);

   for (auto* cable : mEnablerCable->GetPatchCables())
   {
      IUIControl* uicontrol = dynamic_cast<IUIControl*>(cable->GetTarget());
      if (uicontrol)
      {
         uicontrol->SetValue(mEnablerInverted, time);
      }
   }
}
void SongCanvasRackEnabler::HandleRightClickDropdown(int optionValue)
{
   if (optionValue == 10)
   {
      mEnablerInverted = !mEnablerInverted;
      if (mEnablerInverted)
      {
         SetColor(ofColor(40, 40, 40));
         SetColorOutline(ofColor(255, 255, 255));
      }
      else
      {
         SetColor(ofColor(255, 255, 255));
      }
   }
}
std::vector<DropdownListElement> SongCanvasRackEnabler::GetRightClickOptions()
{
   return std::vector{ DropdownListElement{ "invert", 10 } };
}
void SongCanvasRackEnabler::DrawRackGraphics()
{
   //int oX = mX+mFlowGridParent->GetRect(true).x;
   //int oY = mY+mFlowGridParent->GetRect(true).y;

   mEnablerCable->SetManualPosition(mWidth - 12, mHeight / 2);
}

void SongCanvasRackEnabler::SetupCanvasPart(SongCanvas_CanvasElement* element)
{
   if (mEnablerInverted)
   {
      element->mCurrentColor = mCanvasEnablerInvertColor;
      element->mCurrentColorGrad = mCanvasEnablerInvertColor2;
      element->mTextDrawYOffset = 2;
   }

   else
   {
      element->mCurrentColor = mCanvasEnablerColor;
      element->mCurrentColorGrad = mCanvasEnablerColor2;
      element->mTextDrawYOffset = 0;
   }
}
void SongCanvasRackEnabler::DrawCanvasPartGraphics(SongCanvas_CanvasElement* element, ofRectangle rect)
{
   if (mEnablerInverted)
   {
      ofRectangle seamRect = rect;
      seamRect.height = MIN(rect.y, 2);
      ofSetColor(ofColor(200, 200, 200)); //Draw a seam.
      ofRect(seamRect, 0);
   }
}

/////////////
///Pulser///
/////////////


SongCanvasRackPulser::SongCanvasRackPulser(const std::string& partName, SongCanvas* songCanvas)
: SongCanvasRackElement(partName, songCanvas)
{
   SetColor(ofColor::yellow);
   if (mOnePulseMode)
   {
      SetColor(ofColor(150, 150, 0));
   }
}
SongCanvasRackPulser::~SongCanvasRackPulser()
{
   mSongCanvas->RemovePatchCableSource(mPulserCable);
   TheTransport->RemoveListener(this);
   mSongCanvas->DisposeElement(mIntervalSelector);
   mIntervalSelector->Delete();
}

void SongCanvasRackPulser::CreateUIControls()
{
   SongCanvasRackElement::CreateUIControls();

   mPulserCable = new PatchCableSource(this, kConnectionType_Pulse);
   this->AddPatchCableSource(mPulserCable);
   mPulserCable->SetAllowMultipleTargets(true);

   mIntervalSelector = new DropdownList(this, "interval", 75, 2, (int*)(&mPulserInterval));
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

}
void SongCanvasRackPulser::Init()
{
   SongCanvasRackElement::Init();
   mTransportListenerInfo = TheTransport->AddListener(this, mPulserInterval, OffsetInfo(0, true), true);
}

void SongCanvasRackPulser::OnEnter()
{
   double time = NextBufferTime(mSongCanvas);
   const std::vector<IPulseReceiver*>& receivers = mPulserCable->GetPulseReceivers();
   mPulserCable->AddHistoryEvent(time, true, 0);
   mPulserCable->AddHistoryEvent(time + 15, false);
   Excite(1);
   for (auto* receiver : receivers)
      receiver->OnPulse(time, 1, 0);
}
void SongCanvasRackPulser::OnExit()
{
}

void SongCanvasRackPulser::OnTimeEvent(double time)
{
   if (IsEnabled() && mSongCanvas->IsEnabled())
   {
      const std::vector<IPulseReceiver*>& receivers = mPulserCable->GetPulseReceivers();
      mPulserCable->AddHistoryEvent(time, true, 0);
      mPulserCable->AddHistoryEvent(time + 15, false);
      Excite(1);
      for (auto* receiver : receivers)
         receiver->OnPulse(time, 1, 0);
   }
}


void SongCanvasRackPulser::SetupCanvasPart(SongCanvas_CanvasElement* element)
{
   if (!mOnePulseMode)
   {
      element->mCurrentColor = mCanvasPulserColor;
      element->mCurrentColorGrad = ofColor(
      MAX(0, element->mCurrentColor.r - 90),
      MAX(0, element->mCurrentColor.g - 90),
      MAX(0, element->mCurrentColor.b - 90));
   }
   else
   {
      element->mCurrentColor = mCanvasOnePulseColor;
      element->mTextDrawXOffset = 4;
      element->mCurrentColorGrad = ofColor(
      MAX(0, element->mCurrentColor.r - 30),
      MAX(0, element->mCurrentColor.g - 30),
      MAX(0, element->mCurrentColor.b - 30));
   }
}

void SongCanvasRackPulser::HandleRightClickDropdown(int optionValue)
{
   if (optionValue == 10)
   {
      mOnePulseMode = true;
   }
   else if (optionValue == 11)
   {
      mOnePulseMode = false;
   }
   UpdateMode();
}

std::vector<DropdownListElement> SongCanvasRackPulser::GetRightClickOptions()
{
   if (mOnePulseMode)
   {
      return std::vector{ DropdownListElement{ "one pulse mode", 10 } };
   }
   return std::vector{ DropdownListElement{ "interval mode", 11 } };
}
void SongCanvasRackPulser::DropdownUpdated(DropdownList* list, int oldVal, double time)
{
   if (list == mIntervalSelector)
   {
      TransportListenerInfo* transportListenerInfo = TheTransport->GetListenerInfo(this);
      if (transportListenerInfo != nullptr)
      {
         transportListenerInfo->mInterval = this->GetInterval();
         transportListenerInfo->mOffsetInfo = OffsetInfo(0, false);
      }
   }
}
void SongCanvasRackPulser::UpdateMode()
{
   if (mOnePulseMode)
   {
      SetColor(ofColor(40, 40, 40));
      SetColorOutline(ofColor(255, 255, 255));
      mIntervalSelector->SetShowing(false);
   }
   else
   {
      SetColor(ofColor(255, 255, 255));
      mIntervalSelector->SetShowing(true);
   }
}

void SongCanvasRackPulser::DrawRackGraphics()
{
   if (!mOnePulseMode)
   {
      mPulserCable->SetManualPosition(mWidth - 12, mHeight / 2);
      mIntervalSelector->SetPosition(mWidth - 53, 7);
      mIntervalSelector->Draw();
   }
   else
   {
      mPulserCable->SetManualPosition(mWidth - 12, mHeight / 2);
   }
}


///////////
///Keyer///
///////////

void SongCanvasRackKeyer::DrawRackGraphics()
{
}
SongCanvasRackKeyer::SongCanvasRackKeyer(const std::string& partName, SongCanvas* songCanvas)
: SongCanvasRackElement(partName, songCanvas)
{
}


/////////////
///Sampler///
/////////////

SongCanvasRackSampler::SongCanvasRackSampler(const std::string& partName, SongCanvas* songCanvas)
: SongCanvasRackElement(partName, songCanvas)
{
   SetColor(ofColor::green);

   mSampleLoaderButton = new ClickButton(songCanvas, "sample", 60, 2, ButtonDisplayStyle::kText);
}
SongCanvasRackSampler::~SongCanvasRackSampler()
{
   mSongCanvas->DisposeElement(mSampleLoaderButton);

   //TODO I have strong suspicions that more will be needed here.
}


void SongCanvasRackSampler::OnEnter()
{
   if (!mSample->IsSampleLoading())
   {
      Excite(1);
      mSongCanvas->PlaySample();
   }
}
void SongCanvasRackSampler::OnExit()
{
}

void SongCanvasRackSampler::LoadFileSample()
{
   auto file_pattern = TheSynth->GetAudioFormatManager().getWildcardForAllFormats();
   if (File::areFileNamesCaseSensitive())
      file_pattern += ";" + file_pattern.toUpperCase();
   FileChooser chooser("Load sample", File(ofToSamplePath("")),
                       file_pattern, true, false, TheSynth->GetFileChooserParent());
   if (chooser.browseForFileToOpen())
   {
      auto file = chooser.getResult();

      Sample* sample = new Sample();
      if (file.existsAsFile())
         sample->Read(file.getFullPathName().toStdString().c_str());
      SetSample(sample);
   }
}

void SongCanvasRackSampler::ButtonClicked(ClickButton* button, double time)
{
   if (button == mSampleLoaderButton)
   {
      LoadFileSample();
   }
}
void SongCanvasRackSampler::SetSample(Sample* sample)
{
   mSample = sample;
   mSongCanvas->DebugSetSample(mSample);
   sample->SetPlayPosition(0);
   mSampleLoaderButton->SetOverrideDisplayName(mSample->Name());
}
void SongCanvasRackSampler::DrawRackGraphics()
{
   mSampleLoaderButton->SetPosition(12 + GetStringWidth(*mElementName), 7);
   mSampleLoaderButton->Draw();
}
void SongCanvasRackSampler::SetupCanvasPart(SongCanvas_CanvasElement* element)
{
   element->mCurrentColor = mCanvasSamplerColor;
   element->mCurrentColorGrad = mCanvasSamplerColor2;
}

/////////
///LFO///
/////////

SongCanvasRackLFO::SongCanvasRackLFO(const std::string& partName, SongCanvas* songCanvas)
: SongCanvasRackElement(partName, songCanvas)
{
   //TODO
}
void SongCanvasRackLFO::SetupCanvasPart(SongCanvas_CanvasElement* element)
{
   element->mCurrentColor = mCanvasLFOColor;
}
