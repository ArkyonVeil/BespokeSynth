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

#include "INoteReceiver.h"
#include "ModularSynth.h"
#include "SongCanvas.h"
#include "Sample.h"
#include "UserPrefs.h"

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
   mDisplayStringPxWidth = GetStringWidth(partName);
}


void SongCanvasRackElement::CreateUIControls()
{
   FlowGridElement::CreateUIControls();
   mElementRenameTextBox = mSongCanvas->GetRackRenameTextbox();
}
float SongCanvasRackElement::GetPreferredWidth() const
{
   int baseSize = 60;
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
         if (x > GetGeneralReservedWidth())
            return; //Only register rename attempts in the first half of the part. IE, where part names are.
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
   if (mExciteWiggle > 0) //Make the outline bounce for extra visual satisfaction.
   {
      float excConst = mExciteWiggle + sin(ofGetGlobalTime() * 12) * 0.2F;
      if (mExcitePower < excConst)
         mExcitePower = excConst;
   }
   mExcitePower = MAX(0, mExcitePower - ofGetLastFrameTime() * 2);
   mExciteDrag = ofLerp(mExciteDrag, mExcitePower, ofGetLastFrameTime() * 12);
   mOutlineThickness = 0.8F + mExciteDrag * 1.2 + mExciteConstant;
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
   float highlight = 0;
   IAudioSource* audioSource = dynamic_cast<IAudioSource*>(this);
   if (audioSource)
   {
      RollingBuffer* vizBuff = audioSource->GetVizBuffer();
      int numSamples = std::min(500, vizBuff->Size());
      float sample;
      float mag = 0;
      for (int ch = 0; ch < vizBuff->NumChannels(); ++ch)
      {
         for (int i = 0; i < numSamples; ++i)
         {
            sample = vizBuff->GetSample(i, ch);
            mag += sample * sample;
         }
      }
      mag /= numSamples * vizBuff->NumChannels();
      mag = sqrtf(mag);
      mag = sqrtf(mag);
      mag *= 3;
      mag = ofClamp(mag, 0, 1);

      if (UserPrefs.draw_module_highlights.Get())
         highlight = mag * .15f;
   }

   /* TODO Make it blend white for extra highlight power.
   ofColor color = ofColor::green;
   float backgroundAlpha = IsEnabled() ? 180 : 120;
   if (IsEnabled())
      color = ofColor(color.r * (.25f + highlight), color.g * (.25f + highlight), color.b * (.25f + highlight), backgroundAlpha);
   else
      color = ofColor(color.r * .2f, color.g * .2f, color.b * .2f, backgroundAlpha);
*/

   SetExciteConstant(highlight*10);
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
   return SongCanvasRackElement::GetPreferredWidth() + 17;
}

void SongCanvasAudioRackElement::OnPostResize()
{
   SongCanvasRackElement::OnPostResize();
   mChannelPicker->SetPosition(mWidth - 26, GetCenteredElementY(mChannelPicker));
}
void SongCanvasAudioRackElement::SaveState(FileStreamOut& out)
{
   SongCanvasRackElement::SaveState(out);
   out << mMixerIndex;
}
void SongCanvasAudioRackElement::LoadState(FileStreamIn& in, int rev)
{
   mLoading = true;
   SongCanvasRackElement::LoadState(in, rev);
   in >> mMixerIndex;
   SetMixer(mSongCanvas->GetMixerRef(mMixerIndex));
   mLoading = false;
}
void SongCanvasAudioRackElement::SwapMixers(int newIndex)
{
   if (mLoading)
      return;
   mMixerIndex = -1;
   mMixer = nullptr;
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


void SongCanvasRackEnabler::OnEnter(SongCanvas_CanvasElement* element)
{
   double time = NextBufferTime(mSongCanvas);

   mEnablerCable->AddHistoryEvent(time, true, 0);

   Excite(1);
   SetExciteWiggle(0.6);
   for (auto* cable : mEnablerCable->GetPatchCables())
   {
      IUIControl* uicontrol = dynamic_cast<IUIControl*>(cable->GetTarget());
      if (uicontrol)
      {
         uicontrol->SetValue(!mEnablerInverted, time);
      }
   }
}
void SongCanvasRackEnabler::OnExit(SongCanvas_CanvasElement* element)
{
   double time = NextBufferTime(mSongCanvas);
   mEnablerCable->AddHistoryEvent(time, false, 0);

   SetExciteWiggle(0);

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

void SongCanvasRackPulser::OnEnter(SongCanvas_CanvasElement* element)
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
void SongCanvasRackPulser::OnExit(SongCanvas_CanvasElement* element)
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
   if (!mOnePulseMode) //Interval mode
   {
      element->mCurrentColor = mCanvasPulserColor;
      element->mCurrentColorGrad = ofColor(
      MAX(0, element->mCurrentColor.r - 90),
      MAX(0, element->mCurrentColor.g - 90),
      MAX(0, element->mCurrentColor.b - 90));
   }
   else //One Pulse
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
   else
   {
      ofPushStyle();
      float interval = TheTransport->CountInStandardMeasure(mPulserInterval);
      float lengthSizePixels = element->GetCanvas()->GetLengthSizePixels();
      float intervalSpacing = lengthSizePixels / interval;

      float offset;
      offset = element->mOffset * (rect.width / element->mLength); //Normally we use this, but if dragging, this isn't correct anymore so we need to calculate it in real time.
      float startX = rect.x;
      if (element->GetHighlighted())
      {
         int pRow, pCol;
         float pOffset;
         element->GetDragDestinationDataUnquantized(ofVec2f(rect.x, rect.y), pRow, pCol, pOffset);
         offset = pOffset * (rect.width / element->mLength);
      }
      ofSetColor(200, 200, 0, 150);
      ofSetLineWidth(0.75f);
      int i = 0;
      while (i * intervalSpacing - offset < rect.width - 0.5f)
      {
         float lX = startX - offset + i * intervalSpacing;
         if (lX < rect.x - 0.5f)
         {
            i++;
            continue;
         }
         ofLine(lX, rect.y + rect.height / 2, lX, rect.y + rect.height - 1.5f);
         i++;
      }

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
, mSampleButton(this)
{
   mSampleDisplayNameWidth = 50;
   SetColor(ofColor::green);
   mVoiceParams.mVol = 0.5f;
   mVoiceParams.mAdsr.Set(10, 0, 1, 10);
   mVoiceParams.mSample = mSample;
   mVoiceParams.mSamplePitch = 48;

   mSampleDisplayNameWidth = mSampleDisplayNameWidthDefault;
   mDrawAudioBufferSettings.maxChannels = 1;

   mPolyMgr.Init(kVoiceType_Sampler, &mVoiceParams);
}
SongCanvasRackSampler::~SongCanvasRackSampler()
{
   delete mSample;
}

void SongCanvasRackSampler::KillAudio()
{
   if (mSample)
   {
      mPolyMgr.KillAll();
   }
}
void SongCanvasRackSampler::CreateUIControls()
{
   SongCanvasAudioRackElement::CreateUIControls();
}

void SongCanvasRackSampler::OnEnter(SongCanvas_CanvasElement* element)
{
   if (mSample)
   {
      if (!mSample->IsSampleLoading())
      {
         Excite(1);
         PlaySample();
      }
   }
}
void SongCanvasRackSampler::OnExit(SongCanvas_CanvasElement* element)
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
}
bool SongCanvasRackSampler::MouseMoved(float x, float y)
{
   bool r = SongCanvasAudioRackElement::MouseMoved(x, y);
   mSampleButton.OnMouseMove(x, y);
   return r;
}
void SongCanvasRackSampler::SetSample(Sample* sample)
{
   if (sample != mSample && mSample)
      delete mSample;
   mSample = sample;
   mVoiceParams.mSample = mSample;
   sample->SetPlayPosition(0);
   mSampleDisplayNameWidth = mSampleDisplayNameWidthDefault;
   if (mSample)
   {
      //Calculate name length for display/truncating purposes.
      TextTruncationSettings tts;
      tts.maxTextWidth = 160;
      tts.fontSize = 9;
      float sN = GetStringWidth(TruncateString(mSample->Name(), tts), 8);
      mSampleDisplayNameWidth = MAX(mSampleDisplayNameWidthDefault, sN + 10);

      //Calculate 'long' sample logic.
      if (mSample->LengthInSeconds() < 2)
      {
         mLongSample = false;
      }
      else
      {
         int bSize = mSample->Data()->BufferSize();
         float vols = 0;
         int startOffset = mSample->GetOriginalSampleRate() * 2;

         int numChannels = mSample->NumChannels();
         const int kSearchSize = 500;
         int endSearchIdx = MIN(bSize, startOffset + kSearchSize);
         /*
         float peak = 0;
         float* dch = mSample->Data()->GetChannel(0);
         for (int i = 0; i < bSize; ++i)
         {
            peak = MAX(peak,abs(dch[i]));
         }*/

         for (int chI = 0; chI < numChannels; ++chI)
         {
            auto* ch = mSample->Data()->GetChannel(chI);
            for (int i = startOffset; i < endSearchIdx; ++i)
            {
               vols += abs(ch[i]);
            }
         }
         vols = (vols / (float)numChannels) / (float)kSearchSize;

         if (vols > 0.05f)
            mLongSample = true;
         else
            mLongSample = false;
      }
   }
   mSampleButton.mSample = sample;

   //Go through all our canvas parts and set their sizes.
   if (mSample)
   {
      if (mSCLoadingDone)
      {
         auto r = mSongCanvas->GetAllCanvasElementsOfRack(this);

         //Position of a canvas element. 0 -> measure 0. 1 -> measure max.
         float sampleLength = mSample->LengthInSeconds() / (TheTransport->MsPerBar() / 1000) / mSongCanvas->GetMeasureCount();
         for (auto e : r)
         {
            float start = e->GetStart();
            e->SetEnd(start + sampleLength);
         }
      }
   }


   //Finally update our row, so we can resize to the proper size.
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
      GetVizBuffer()->WriteChunk(mWriteBuffer.GetChannel(ch), mWriteBuffer.BufferSize(), ch);//This allows the element to bounce.
      Add(GetMixerBuffer()->GetChannel(ch), mWriteBuffer.GetChannel(ch), gBufferSize);
   }
}
void SongCanvasRackSampler::DrawRackGraphics()
{
   mSampleButton.Draw();
}

float SongCanvasRackSampler::GetPreferredWidth() const
{
   float val = SongCanvasAudioRackElement::GetPreferredWidth();

   val += 48;

   return val;
}

void SongCanvasRackSampler::OnPostResize()
{
   SongCanvasAudioRackElement::OnPostResize();
   float workSpace = mWidth - GetGeneralReservedWidth() - 34;
   mSampleButton.SetRect(GetGeneralReservedWidth(), 1, CLAMP(workSpace, 5, 75), mHeight - 2);
}
void SongCanvasRackSampler::OnClicked(float x, float y, bool right)
{
   //Clicking the Sampler workspace typically attempts to play the sample  for quick feedback.
   //ofLog() << "Hotcake";

   if (mSample)
   {
      int previewMode = static_cast<int>(mSongCanvas->mSamplerAudioPreviewMode);

      if (!mSampleButton.CheckIntercept(x, y))
      {
         if (previewMode == 0 && !mLongSample)
         {
            PlaySample();
         }
         if (previewMode == 2) //Always
         {
            PlaySample();
         }
      }
   }

   mSampleButton.OnClick(x, y, right);

   SongCanvasAudioRackElement::OnClicked(x, y, right);
}
void SongCanvasRackSampler::SongCanvasOptionsUpdated()
{
   SongCanvasAudioRackElement::SongCanvasOptionsUpdated();

   mSampleButton.UpdateRect();
}
bool SongCanvasRackSampler::TestClick(float x, float y, bool right, bool testOnly)
{
   bool r = SongCanvasAudioRackElement::TestClick(x, y, right, testOnly);

   if (mSampleButton.CheckIntercept(x, y))
      r = true;

   return r;
}
void SongCanvasRackSampler::FilesDropped(std::vector<std::string> files, int x, int y)
{
   Sample* s = new Sample();
   s->LockDataMutex(true);
   s->Read(files[0].c_str());
   s->LockDataMutex(false);
   SetSample(s);
}
void SongCanvasRackSampler::SampleDropped(int x, int y, Sample* sample)
{
   Sample* s = new Sample();
   s->LockDataMutex(true);
   s->CopyFrom(sample);
   s->LockDataMutex(false);
   SetSample(s);
}


void SongCanvasRackSampler::SetupCanvasPart(SongCanvas_CanvasElement* element)
{
   element->mCurrentColor = mCanvasSamplerColor;
   element->mCurrentColorGrad = mCanvasSamplerColor2;
   if (mSample && mSCLoadingDone)
   {
      float sampleLength = mSample->LengthInSeconds() / (TheTransport->MsPerBar() / 1000) / mSongCanvas->GetMeasureCount();
      element->SetEnd(element->GetStart() + sampleLength);
   }
   element->SetAllowResize(false);
}
void SongCanvasRackSampler::DrawCanvasPartGraphics(SongCanvas_CanvasElement* element, ofRectangle rect)
{
   SongCanvasAudioRackElement::DrawCanvasPartGraphics(element, rect);

   if (mSample)
   {
      ofPushMatrix();
      ofTranslate(rect.x, rect.y + 9);
      DrawAudioBuffer(rect.width, rect.height - 9, mSample->Data(), mDrawAudioBufferSettings);
      ofPopMatrix();
   }
}

void SongCanvasRackSampler::SaveState(FileStreamOut& out)
{
   SongCanvasAudioRackElement::SaveState(out);

   bool hasSample = false;
   if (mSample)
      hasSample = true;
   out << hasSample;
   if (mSample)
      mSample->SaveState(out);
}
void SongCanvasRackSampler::LoadState(FileStreamIn& in, int rev)
{
   SongCanvasAudioRackElement::LoadState(in, rev);
   mSCLoadingDone = false;

   bool hasSample;
   in >> hasSample;
   if (hasSample)
   {
      auto* sample = new Sample();
      sample->LoadState(in);
      SetSample(sample);
   }
}
void SongCanvasRackSampler::OnLoadFinish()
{
   SongCanvasAudioRackElement::OnLoadFinish();

   mSCLoadingDone = true;
}

// Sample Button
SongCanvasRackSampler::RackSampleButton::RackSampleButton(SongCanvasRackSampler* owner)
{
   mDrawAudioBufferSettings.maxChannels = 1;
   ofRect(30, 1, 50, 20);
   mOwner = owner;
   mSampleRenderNameSettings.fontSize = 9;
   mSampleRenderNameSettings.cutOffStyle = "";
}
void SongCanvasRackSampler::RackSampleButton::OnMouseMove(float x, float y)
{
   if (mRect.width <= 10)
   {
      mHoveredTotal = false;
      mHoveredName = false;
      mHoveredPlay = false;
      mHoveredStop = false;
      return;
   }
   mHoveredTotal = mRect.contains(x, y);

   x -= mRect.x;
   y -= mRect.y;
   mHoveredName = mHoveredTotal && y < 12;

   if (mRect.width > kMinWidthDrawButtons)
   {
      if (mDrawControlOptions)
         mHoveredPlay = mPlayButtonRect.contains(x, y);
      else
         mHoveredPlay = false;
      mHoveredStop = mStopButtonRect.contains(x, y);
      mHoveredDrag = mDragButtonRect.contains(x, y);
   }
   else
   {
      mHoveredName = false;
      mHoveredPlay = false;
      mHoveredStop = false;
   }
}
bool SongCanvasRackSampler::RackSampleButton::CheckIntercept(float x, float y)
{
   if (!mSample)
   {
      return mHoveredTotal;
   }
   else
   {
      int r = mHoveredPlay + mHoveredStop + mHoveredName + mHoveredDrag;
      if (r > 0)
         return true;
      else
         return false;
   }
}

void SongCanvasRackSampler::RackSampleButton::OnClick(float x, float y, bool right)
{
   if (!mRect.contains(x, y))
      return;
   if (right)
      return;

   if (!mSample)
   {
      mOwner->LoadFileSample();
   }
   else
   {
      if (mHoveredName)
      {
         mOwner->LoadFileSample();
      }
      if (mHoveredPlay)
      {
         mOwner->PlaySample();
      }
      if (mHoveredStop)
      {
         mOwner->KillAudio();
      }
      if (mHoveredDrag)
      {
         TheSynth->GrabSample(mSample->Data(), mSample->Name());
      }
   }
}
void SongCanvasRackSampler::RackSampleButton::Draw()
{
   if (mRect.width <= 10) //Don't bother drawing if we're too small.
      return;
   ofPushStyle();
   ofPushMatrix();
   ofTranslate(mRect.x, mRect.y);

   ofSetColor(ofColor(84, 106, 79, 200));
   ofFill();
   ofRect(0, 0, mRect.width, mRect.height);
   ofNoFill();
   if (mSample)
   {
      ofPushMatrix();
      ofTranslate(2, 2);
      DrawAudioBuffer(mRect.width - 4, mRect.height - 2, mSample->Data(), mDrawAudioBufferSettings);
      ofPopMatrix();

      bool clipOptions = mRect.width <= kMinWidthDrawButtons;

      ofSetColor(ofColor(230, 230, 230));
      if (!mOwner->GetHovered())
      {
         mNameDisplayAnimOffset = 0;
         DrawTextNormal(mDisplayName, 2, 9, 9);
      }
      else
      {
         mNameDisplayAnimOffset = MIN(mFullNameWidth - mRect.width + 10, mNameDisplayAnimOffset + ofGetDeltaTime() * 30);
         mNameDisplayAnimOffset = MAX(2, mNameDisplayAnimOffset);
         ofClipWindow(1, 1, mRect.width, mRect.height, false);
         DrawTextNormal(mFullSampleName, 4 - mNameDisplayAnimOffset, 9, 9);
         ofResetClipWindow();
      }

      if (!clipOptions)
      {
         ofPushStyle();
         ofSetColor(ofColor(230, 230, 230));
         ofFill();
         if (mDrawControlOptions)
         {
            ofTriangle(
            mPlayButtonRect.x + 3, mStopButtonRect.y + 3,
            mPlayButtonRect.x + 3, mPlayButtonRect.y + mPlayButtonRect.height - 3,
            mPlayButtonRect.x + mPlayButtonRect.width - 3, mPlayButtonRect.y + mPlayButtonRect.height / 2);
         }
         ofRect(mStopButtonRect.x + 3, mStopButtonRect.y + 3, mStopButtonRect.width - 6, mStopButtonRect.height - 6, 0);

         //Draw the dragButton
         float iX = mDragButtonRect.x - 2;
         float iY = mDragButtonRect.y - 1;
         for (int i = 0; i < 5; ++i)
         {
            float height = (i % 2 == 0) ? 3 : 6;
            float x = iX + 4 + i * 3;
            ofLine(x, iY + 7 - height / 2, x, iY + 7 + height / 2);
         }

         ofPopStyle();
      }

      ofSetColor(ofColor::cyan);
      if (mHoveredName)
         ofRect(0, 0, mRect.width, 12);
      if (mHoveredPlay)
         ofRect(mPlayButtonRect);
      if (mHoveredStop)
         ofRect(mStopButtonRect);
      if (mHoveredDrag)
         ofRect(mDragButtonRect);
   }
   else
   {
      if (mHoveredTotal)
      {
         ofNoFill();
         ofSetColor(ofColor::cyan);
         ofRect(0, 0, mRect.width, mRect.height);
      }
      ofSetColor(ofColor(210, 210, 210));
      DrawTextNormal(mDisplayName, 2, mRect.height / 2 + 5);
   }

   //Drop in a sample notice.
   bool sampleDropNotice = TheSynth->GetHeldSample() && TheSynth->GetHeldSample() != mSample;

   if (sampleDropNotice)
   {
      ofSetColor(255, 255, 255, 45 + sinf(ofGetGlobalTime() * 7) * 40);
      ofFill();
      ofRect(0, 0, mRect.width, mRect.height);
   }
   ofPopMatrix();

   ofPopStyle();
}
void SongCanvasRackSampler::RackSampleButton::UpdateRect()
{
   SetRect(mRect.x, mRect.y, mRect.width, mRect.height);
}

void SongCanvasRackSampler::RackSampleButton::SetRect(float x, float y, float width, float height)
{
   mRect = ofRectangle(x, y, width, height);

   if (mSample)
   {
      mDisplayName = StripNameExtension(mSample->Name());
      mFullSampleName = mDisplayName;
      mFullNameWidth = GetStringWidth(mFullSampleName, 9);
      mDisplayName = TruncateString(mDisplayName, mSampleRenderNameSettings);
      mSampleRenderNameSettings.maxTextWidth = width - 5;
   }
   else
   {
      mDisplayName = TruncateString("Sample", width - 5);
   }
   float miniButtonY = mRect.height - 11;
   float miniXOffset = 2;

   mDrawControlOptions = false;
   auto previewMode = (int)mOwner->mSongCanvas->mSamplerAudioPreviewMode;
   if ((previewMode == 0 && mOwner->mLongSample) || previewMode == 1)
      mDrawControlOptions = true;

   if (mDrawControlOptions)
   {
      mPlayButtonRect = ofRectangle(miniXOffset, miniButtonY, 12, 12);
      miniXOffset += 12;
   }
   mStopButtonRect = ofRectangle(miniXOffset, miniButtonY, 12, 12);

   mDragButtonRect = ofRectangle(width - 18, miniButtonY, 16, 12);
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
