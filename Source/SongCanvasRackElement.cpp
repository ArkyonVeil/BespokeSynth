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
#include "INoteReceiver.h"
#include "ModularSynth.h"
#include "SongCanvas.h"
#include "Sample.h"

#include "juce_audio_formats/juce_audio_formats.h"
using namespace juce;

std::string TruncateStringLegacy(std::string str, size_t width, bool show_ellipsis = true)
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


FlowGridElement* SongCanvasRackFactory::Create(const std::string typeName)
{
   //Ark: This isn't particularly sophisticated, but I also don't know any better. Anyone who cares is welcome to redo. <>V
   if ("partenabler" == typeName)
      return new SongCanvasRackEnabler("", mSongCanvas);
   if ("partpulser" == typeName)
      return new SongCanvasRackPulser("", mSongCanvas);
   if ("partkeyer" == typeName)
      return new SongCanvasRackKeyer("", mSongCanvas);
   if ("partsampler" == typeName)
      return new SongCanvasRackSampler("", mSongCanvas);
   if ("partlfo" == typeName)
      return new SongCanvasRackLFO("", mSongCanvas);

   throw std::invalid_argument("SongCanvas loaded an unknown rack type --> " + typeName);
}
SongCanvasRackElement::SongCanvasRackElement(std::string partName, std::string internalName, SongCanvas* songCanvas)
: FlowGridElement(mFlowGridParent, internalName)
{
   SetOwningContainer(songCanvas->GetOwningContainer());
   mElementName = new std::string(partName);
   mSongCanvas = songCanvas;
   mInternalRackID = mSongCanvas->GetInternalRackId();
   mHighlightOutlineColor = ofColor(0, 150, 255);
}


void SongCanvasRackElement::CreateUIControls()
{
   FlowGridElement::CreateUIControls();
   mElementRenameTextBox = mSongCanvas->GetRackRenameTextbox();
}
float SongCanvasRackElement::GetPreferredWidth() const
{
   int baseSize = 30;
   std::string rackName = TruncateString(*mElementName, 200);

   int textSize = MIN(200, GetStringWidth(rackName));

   return baseSize + textSize;
}


void SongCanvasRackElement::SetPartName(std::string newName) const
{
   *mElementName = newName;
}


void SongCanvasRackElement::SaveState(FileStreamOut& out)
{
   FlowGridElement::SaveState(out);

   out << *mElementName;
   out << mRackEnabled;
   out << mInternalRackID;

   auto cables = GetPatchCableSources();
   out << (int)cables.size();
   for (auto* cable : cables)
      cable->SaveState(out);
}
void SongCanvasRackElement::LoadState(FileStreamIn& in, int rev)
{
   FlowGridElement::LoadState(in, rev);

   std::string partName;
   in >> partName;
   SetPartName(partName);
   in >> mRackEnabled;
   in >> mInternalRackID;

   int numCables;
   in >> numCables;
   PatchCableSource* dummy = nullptr;
   for (int i = 0; i < numCables; ++i)
   {
      PatchCableSource* readIn;
      if (i < GetPatchCableSources().size())
      {
         readIn = GetPatchCableSources()[i];
      }
      else
      {
         if (dummy == nullptr)
            dummy = new PatchCableSource(this, kConnectionType_Special);
         readIn = dummy;
      }
      readIn->LoadState(in);
   }
}
void SongCanvasRackElement::OnClicked(float x, float y, bool right)
{
   FlowGridElement::OnClicked(x, y, right);
   if (right)
   {
      mSongCanvas->OpenRightClickRackMenu(this);
   }
   else
   {
      if (mFlowGridParent->GetDraggedGridElement() == this) //Suppresses renaming if being dragged.
      {
         mLastClickTime = 0;
      }

      if (TheSynth->GetGlobalTime() < mLastClickTime + 0.5)
      {
         mBufferQuickRename = true;
         mLastClickTime = 0;
      }
      else
      {
         mLastClickTime = TheSynth->GetGlobalTime();
      }
   }
}
bool SongCanvasRackElement::MouseMoved(float x, float y)
{
   bool val = FlowGridElement::MouseMoved(x, y);

   if (mFlowGridParent->GetDraggedGridElement() == this)
   {
      mBufferQuickRename = false;
   }

   return val;
}
void SongCanvasRackElement::MouseReleased()
{
   FlowGridElement::MouseReleased();

   if (mBufferQuickRename)
   {
      mSongCanvas->SetRackElementRenameState(this, true);
      mBufferQuickRename = false;
   }
}
float SongCanvasRackElement::GetLeftWidthPadding()
{
   return 8;
}
float SongCanvasRackElement::GetPartNameWidth() const
{
   return mDisplayStringPxWidth;
}
float SongCanvasRackElement::GetGeneralReservedWidth()
{
   return GetLeftWidthPadding() + mDisplayStringPxWidth + 8;
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
   DrawExtendedBaseGraphics();

   //Unique rack graphics are drawn here...
   DrawRackGraphics();

   if (mRenameActive)
   {
      //mElementRenameTextBox->SetPosition(rPos.x, rPos.y + mHeight / 2);
      mElementRenameTextBox->SetPosition(4, 7);
      mElementRenameTextBox->Draw();

      int form = (static_cast<std::string>(mElementRenameTextBox->GetText()).size() - 10) * 6;
      if (form < 0)
         form = 0;

      if (mLastRenameSize != mElementRenameTextBox->GetRect(true).width)
      {
         mLastRenameSize = mElementRenameTextBox->GetRect(true).width;
         UpdateRow();
      }

      if (mElementRenameTextBox->GetActiveKeyboardFocus() != mElementRenameTextBox)
      {
         mSongCanvas->SetRackElementRenameState(this, false);
      }
   }
   else
   {
      //TODO: This truncation code is stinky. Replace it soon
      int textSize = (mWidth - 15) / 7.0;
      std::string displayString;
      if (textSize <= 3)
      {
         displayString = RemoveNonNumericalChars(mElementName->c_str());
         if (displayString.empty())
            displayString = TruncateStringLegacy(mElementName->c_str(), textSize, true);
      }
      else
      {
         displayString = TruncateStringLegacy(mElementName->c_str(), textSize, true);
      }

      if (displayString.size() != mLastNameSize)
      {
         mLastNameSize = displayString.size();
         mDisplayStringPxWidth = GetStringWidth(displayString);
         UpdateRow();
      }

      if (mWidth >= 30)
      {
         /*
         if (mFlowGridParent->GetSelectedGridElement() != this)*/
         DrawTextNormal(displayString, GetLeftWidthPadding(), mHeight / 2 + 5.5);
         /*else
            DrawTextBold(displayString, 8, mHeight / 2 + 5.5);*/
      }
   }
   ofPopStyle();
}

///////////////////
///AUDIO GENERIC///
///////////////////

SongCanvasAudioRackElement::~SongCanvasAudioRackElement()
{
   RemoveUIControl(mChannelPicker);
}
void SongCanvasAudioRackElement::DrawExtendedBaseGraphics()
{
   mChannelPicker->Draw();
}
void SongCanvasAudioRackElement::CreateUIControls()
{
   SongCanvasRackElement::CreateUIControls();

   mChannelPicker = new TextEntry(this, "mixer", mWidth - 36, mHeight / 2, 2, &mMixerIndex, 0, 99);
   mChannelPicker->SetCableTargetable(false);
   mChannelPicker->SetRequireEnter(false);
}

void SongCanvasAudioRackElement::SetMixer(SongCanvasMixer* mixer)
{
   mMixer = mixer;
   mMixerIndex = mMixer->mMixerIndex;
   mMixerBuffer = mMixer->GetBuffer();
}
void SongCanvasAudioRackElement::TextEntryComplete(TextEntry* entry)
{
   SwapMixers(mMixerIndex);
}
float SongCanvasAudioRackElement::GetPreferredWidth() const
{
   return SongCanvasRackElement::GetPreferredWidth() + 34;
}
float SongCanvasAudioRackElement::GetAudioReservedWidth() const
{
   return 34;
}
void SongCanvasAudioRackElement::OnPostResize()
{
   SongCanvasRackElement::OnPostResize();
   mChannelPicker->SetPosition(mWidth - 26, GetCenteredElementY(mChannelPicker));
}
void SongCanvasAudioRackElement::SwapMixers(int newIndex)
{
   mMixerIndex = -1;
   SetMixer(mSongCanvas->GetMixer(newIndex));
}


/////////////
///Enabler///
/////////////

SongCanvasRackEnabler::SongCanvasRackEnabler(const std::string& partName, SongCanvas* songCanvas)
: SongCanvasRackElement(partName, GetFlowGridElementType(), songCanvas)
{
   SetColor(ofColor::white);
}
SongCanvasRackEnabler::~SongCanvasRackEnabler()
{
   RemovePatchCableSource(mEnablerCable);
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
void SongCanvasRackEnabler::SaveState(FileStreamOut& out)
{
   SongCanvasRackElement::SaveState(out);
   out << mEnablerInverted;
}
void SongCanvasRackEnabler::LoadState(FileStreamIn& in, int rev)
{
   SongCanvasRackElement::LoadState(in, rev);
   in >> mEnablerInverted;
}
void SongCanvasRackEnabler::DrawRackGraphics()
{
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
: SongCanvasRackElement(partName, GetFlowGridElementType(), songCanvas)
{
}
SongCanvasRackPulser::~SongCanvasRackPulser()
{
   this->RemovePatchCableSource(mPulserCable);
   TheTransport->RemoveListener(this);
   RemoveUIControl(mIntervalSelector);
   mIntervalSelector->Delete();
}

void SongCanvasRackPulser::CreateUIControls()
{
   SongCanvasRackElement::CreateUIControls();

   mPulserCable = new PatchCableSource(this, kConnectionType_Pulse);
   this->AddPatchCableSource(mPulserCable);
   mPulserCable->SetAllowMultipleTargets(true);

   mIntervalSelector = new DropdownList(this, "interval", mWidth - 50, 3, (int*)(&mPulserInterval));
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
}
void SongCanvasRackPulser::Init()
{
   IDrawableModule::Init();
   mTransportListenerInfo = TheTransport->AddListener(this, mPulserInterval, OffsetInfo(0, true), true);
   UpdateMode();
}
void SongCanvasRackPulser::OnPostResize()
{
   if (!mOnePulseMode)
   {
      if (mWidth < GetPreferredWidth() - 20)
      {
         mIntervalSelector->SetShowing(false);
      }
      else
      {
         mIntervalSelector->SetShowing(true);
      }
   }
}

void SongCanvasRackPulser::OnEnter()
{
   if (mOnePulseMode)
   {
      double time = NextBufferTime(mSongCanvas);
      const std::vector<IPulseReceiver*>& receivers = mPulserCable->GetPulseReceivers();
      mPulserCable->AddHistoryEvent(time, true, 0);
      mPulserCable->AddHistoryEvent(time + 15, false);
      Excite(1);
      for (auto* receiver : receivers)
         receiver->OnPulse(time, 1, 0);
   }
}
void SongCanvasRackPulser::OnExit()
{
}

void SongCanvasRackPulser::OnTimeEvent(double time)
{
   if (mOnePulseMode)
      return;
   if (IsEnabled() && mSongCanvas->IsEnabled() && mSongCanvas->IsRackActive(this))
   {
      const std::vector<IPulseReceiver*>& receivers = mPulserCable->GetPulseReceivers();
      mPulserCable->AddHistoryEvent(time, true, 0);
      mPulserCable->AddHistoryEvent(time + 15, false);
      Excite(1);
      for (auto* receiver : receivers)
         receiver->OnPulse(time, 1, 0);
   }
}


float SongCanvasRackPulser::GetPreferredWidth() const
{
   float val = SongCanvasRackElement::GetPreferredWidth();
   if (!mOnePulseMode)
      val += mIntervalSelector->GetRect(true).width;
   return val;
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
void SongCanvasRackPulser::DrawCanvasPartGraphics(SongCanvas_CanvasElement* element, ofRectangle rect)
{
   if (mOnePulseMode)
   {
      ofPushStyle();
      ofSetColor(200, 200, 0);
      ofRect(rect.x, rect.y, 2, rect.height, 0);
      ofPopStyle();
   }
}

void SongCanvasRackPulser::HandleRightClickDropdown(int optionValue)
{
   if (optionValue == 10)
   {
      mOnePulseMode = true;
      UpdateRow();
   }
   else if (optionValue == 11)
   {
      mOnePulseMode = false;
      UpdateRow();
   }
   UpdateMode();
}

std::vector<DropdownListElement> SongCanvasRackPulser::GetRightClickOptions()
{
   if (mOnePulseMode)
   {
      return std::vector{ DropdownListElement{ "interval mode", 11 } };
   }
   return std::vector{ DropdownListElement{ "one pulse mode", 10 } };
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
      SetColor(ofColor(40, 40, 0));
      SetColorOutline(ofColor(220, 220, 0));
      mIntervalSelector->SetShowing(false);
   }
   else
   {
      SetColor(ofColor::yellow);
      mIntervalSelector->SetShowing(true);
   }
}
void SongCanvasRackPulser::SaveState(FileStreamOut& out)
{
   SongCanvasRackElement::SaveState(out);
   out << mOnePulseMode;
   out << mPulserInterval;
}
void SongCanvasRackPulser::LoadState(FileStreamIn& in, int rev)
{
   SongCanvasRackElement::LoadState(in, rev);
   in >> mOnePulseMode;
   int v;
   in >> v;
   mPulserInterval = static_cast<NoteInterval>(v);
   UpdateMode();
}

void SongCanvasRackPulser::DrawRackGraphics()
{
   if (!mOnePulseMode)
   {
      mPulserCable->SetManualPosition(mWidth - 12, mHeight / 2);
      mIntervalSelector->SetPosition(mWidth - 64, mHeight / 4 + 1);
      mIntervalSelector->Draw();
   }
   else
   {
      ofPushStyle();
      ofFill();
      ofSetColor(ofColor{ 200, 200, 0 });
      ofRect(0, 0, 4, mHeight, 0);
      ofPopStyle();
      mPulserCable->SetManualPosition(mWidth - 12, mHeight / 2);
   }
}


///////////
///Keyer///
///////////

SongCanvasRackKeyer::SongCanvasRackKeyer(const std::string& partName, SongCanvas* songCanvas)
: SongCanvasRackElement(partName, GetFlowGridElementType(), songCanvas)
{
}
void SongCanvasRackKeyer::DrawRackGraphics()
{
}
void SongCanvasRackKeyer::SaveState(FileStreamOut& out)
{
   SongCanvasRackElement::SaveState(out);
}
void SongCanvasRackKeyer::LoadState(FileStreamIn& in, int rev)
{
   SongCanvasRackElement::LoadState(in, rev);
}


/////////////
///Sampler///
/////////////

SongCanvasRackSampler::SongCanvasRackSampler(const std::string& partName, SongCanvas* songCanvas)
: SongCanvasAudioRackElement(partName, GetFlowGridElementType(), songCanvas)
, mWriteBuffer(gBufferSize)
, mPolyMgr(this)
{
   SetColor(ofColor::green);

   mSampleLoaderButton = new ClickButton(this, "sample", 0, 3, ButtonDisplayStyle::kText);
   mVoiceParams.mVol = 0.5f;
   mVoiceParams.mAdsr.Set(10, 0, 1, 10);
   mVoiceParams.mSample = mSample;
   mVoiceParams.mSamplePitch = 48;

   mPolyMgr.Init(kVoiceType_Sampler, &mVoiceParams);
}
SongCanvasRackSampler::~SongCanvasRackSampler()
{
   delete mSample;
   RemoveUIControl(mSampleLoaderButton);
}


void SongCanvasRackSampler::OnEnter()
{
   if (!mSample->IsSampleLoading())
   {
      Excite(1);
      PlaySample();
   }
}
void SongCanvasRackSampler::OnExit()
{
   //TODO
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
   delete mSample;
   mSample = sample;
   mVoiceParams.mSample = mSample;
   sample->SetPlayPosition(0);
   mSampleLoaderButton->SetOverrideDisplayName(mSample->Name());

   UpdateRow();
}

void SongCanvasRackSampler::PlaySample()
{
   NoteMessage note(mLastProcessTime, 48, 127);
   mPolyMgr.Start(note.time, note.pitch, note.velocity / 127.0f, note.voiceIdx, note.modulation);
}
void SongCanvasRackSampler::Process(double time)
{
   //PROFILE stuff is in the Song Canvas

   //TODO: Look for optimization opportunities, this might be doing pointless work.

   if (!mEnabled || mSample == nullptr)
      return;

   mLastProcessTime = time;

   int numChannels = 2;
   int bufferSize = GetMixerBuffer()->BufferSize();

   //We have to manually sync our buffer with the mixer's. Otherwise we trip over a crash in the Load() process over a missing cable.
   GetMixerBuffer()->SetNumActiveChannels(MAX(numChannels, GetMixerBuffer()->NumActiveChannels()));

   //Cleanup
   mWriteBuffer.SetNumActiveChannels(numChannels);
   mWriteBuffer.Clear();

   //Sample data is processed by the mPolyMgr, saving us a ton of work here.
   mSample->LockDataMutex(true);
   mPolyMgr.Process(time, &mWriteBuffer, bufferSize);
   mSample->LockDataMutex(false);

   //Send it over to the mixer.
   for (int ch = 0; ch < mWriteBuffer.NumActiveChannels(); ++ch)
   {
      Add(GetMixerBuffer()->GetChannel(ch), mWriteBuffer.GetChannel(ch), gBufferSize);
   }
}
void SongCanvasRackSampler::DrawRackGraphics()
{
   mSampleLoaderButton->Draw();
}

float SongCanvasRackSampler::GetPreferredWidth() const
{
   float val = SongCanvasAudioRackElement::GetPreferredWidth();
   val += mSampleLoaderButton->GetPreferredWidth() - 12;
   return val;
}

void SongCanvasRackSampler::OnPostResize()
{
   SongCanvasAudioRackElement::OnPostResize();
   mSampleLoaderButton->SetPosition(GetGeneralReservedWidth(), 9);
   float bMax = mWidth - GetGeneralReservedWidth() - GetAudioReservedWidth();
   mSampleLoaderButton->SetMaxWidth(MAX(0, bMax));
}
void SongCanvasRackSampler::SetupCanvasPart(SongCanvas_CanvasElement* element)
{
   element->mCurrentColor = mCanvasSamplerColor;
   element->mCurrentColorGrad = mCanvasSamplerColor2;
}

void SongCanvasRackSampler::SaveState(FileStreamOut& out)
{
   SongCanvasRackElement::SaveState(out);
}
void SongCanvasRackSampler::LoadState(FileStreamIn& in, int rev)
{
   SongCanvasRackElement::LoadState(in, rev);
}


/////////
///LFO///
/////////

SongCanvasRackLFO::SongCanvasRackLFO(const std::string& partName, SongCanvas* songCanvas)
: SongCanvasRackElement(partName, GetFlowGridElementType(), songCanvas)
{
   //TODO
}
void SongCanvasRackLFO::SetupCanvasPart(SongCanvas_CanvasElement* element)
{
   element->mCurrentColor = mCanvasLFOColor;
}
void SongCanvasRackLFO::SaveState(FileStreamOut& out)
{
   SongCanvasRackElement::SaveState(out);
}
void SongCanvasRackLFO::LoadState(FileStreamIn& in, int rev)
{
   SongCanvasRackElement::LoadState(in, rev);
}
