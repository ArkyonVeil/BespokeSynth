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

#include "ModularSynth.h"
#include "SongCanvas.h"
#include "UserPrefs.h"
#include "SongCanvasRackSampler.h"

#include "juce_audio_formats/juce_audio_formats.h"
using namespace juce;

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
      return new SongCanvasRackModEnvelope("", mSongCanvas);

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
   mPartNameTruncationSettings.fontSize = kPartNameFontSize;
   UpdatePartNameData();
}


void SongCanvasRackElement::CreateUIControls()
{
   FlowGridElement::CreateUIControls();
   mElementRenameTextBox = mSongCanvas->GetRackRenameTextbox();
}

void SongCanvasRackElement::UpdatePartNameData()
{
   //This is calculated on a need be basis.
   //First get the preferred size of the string, which is capped at around 200~. This is used to determine the room to allocate.
   mRackNameStringPreferredWidth = GetStringWidth(*mElementName, kPartNameFontSize);

   //Now the DisplayPartName is what's seen, as such it may be affected by compression.
   mDisplayPartName = TruncateString(*mElementName, kMaxTextSize * GetCompression(), mPartNameTruncationSettings);
}

float SongCanvasRackElement::GetPreferredWidth() const
{
   return GetReservedPrefLeftWidth() + GetReservedPrefRightWidth();
}

float SongCanvasRackElement::GetReservedPrefLeftWidth() const
{
   float textSize = MAX(GetRenameSpaceUsed(), MAX(GetMinTextSpace(), mRackNameStringPreferredWidth));
   return kLeftWidthPadding + textSize + kPartNameToContentPadding;
}
float SongCanvasRackElement::GetLeftReservedToTextEnd() const
{
   return kLeftWidthPadding + MAX(GetRenameSpaceUsed(), GetStringWidth(mDisplayPartName));
}
float SongCanvasRackElement::GetRenameSpaceUsed() const
{
   return mRenameActive ? mElementRenameTextBox->GetRect().width : 0;
}

void SongCanvasRackElement::SetPartName(std::string newName)
{
   *mElementName = newName;
   UpdatePartNameData();
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
   UpdatePartNameData();
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
         if (x > GetReservedLeftWidth())
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
      if (mWidth >= 30)
      {
         DrawTextNormal(mDisplayPartName, kLeftWidthPadding, mHeight / 2 + 5.5);
      }
   }
   if (!mRackEnabled)
   {
      ofFill();
      ofSetColor({ 0, 0, 0, 125 });
      ofRect(GetRectLocal());
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

   SetExciteConstant(highlight * 10);
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
   mSongCanvas->SetMixerIdHighlight(mMixerIndex);
}

void SongCanvasAudioRackElement::OnPostResize()
{
   SongCanvasRackElement::OnPostResize();
   mChannelPicker->SetPosition(mWidth - 26, GetCenteredElementY(mChannelPicker));
}
bool SongCanvasAudioRackElement::MouseMoved(float x, float y)
{
   if (mChannelPicker->GetRect(true).grow(2).contains(x, y))
      mSongCanvas->SetMixerIdHighlight(mMixerIndex);
   return SongCanvasRackElement::MouseMoved(x, y);
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


void SongCanvasRackEnabler::OnEnter(SongCanvasNote* element)
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
void SongCanvasRackEnabler::OnExit(SongCanvasNote* element)
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
}
void SongCanvasRackEnabler::OnPostResize()
{
   SongCanvasRackElement::OnPostResize();
   mEnablerCable->SetManualPosition(GetOutputPos());
}

void SongCanvasRackEnabler::SetupCanvasPart(SongCanvasNote* element)
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
void SongCanvasRackEnabler::DrawCanvasPartGraphics(SongCanvasNote* element, ofRectangle rect)
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
   SongCanvasRackElement::Init();
   mTransportListenerInfo = TheTransport->AddListener(this, mPulserInterval, OffsetInfo(0, true), true);
   UpdateMode();
}
void SongCanvasRackPulser::OnPostResize()
{
   mPulserCable->SetManualPosition(GetOutputPos());
   if (!mOnePulseMode)
   {
      mIntervalSelector->SetPosition(GetReservedLeftWidth(), mHeight / 4 + 1);
      if (mWidth < GetLeftReservedToTextEnd() + GetReservedRightWidth() + mIntervalSelector->GetRect().width)
      {
         mIntervalSelector->SetShowing(false);
      }
      else
      {
         mIntervalSelector->SetShowing(true);
      }
   }
}

void SongCanvasRackPulser::OnEnter(SongCanvasNote* element)
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
void SongCanvasRackPulser::OnExit(SongCanvasNote* element)
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

void SongCanvasRackPulser::SetupCanvasPart(SongCanvasNote* element)
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
void SongCanvasRackPulser::DrawCanvasPartGraphics(SongCanvasNote* element, ofRectangle rect)
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
      float lengthSizePixels = element->GetCanvas()->GetLengthUnitWidth();
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
      mIntervalSelector->Draw();
      if (!mIntervalSelector->IsShowing())
      {
         ofPushStyle();

         ofSetColor(ofColor::white);
         std::string label = mIntervalSelector->GetLabel(mPulserInterval);

         float availStart = GetLeftReservedToTextEnd();
         float availEnd = mWidth - GetReservedRightWidth() - mIntervalSelector->GetRect(true).width;
         if (availEnd < availStart)
            availEnd = mWidth - 40.f;
         float centerX = (availStart + availEnd) / 2.0f;
         int x = (int)(centerX + GetStringWidth(label, 12) / 2.f);

         DrawTextNormal(label, x, mHeight / 2.0f + 4, 12);

         ofPopStyle();
      }
   }
   else
   {
      ofPushStyle();
      ofFill();
      ofSetColor(ofColor{ 200, 200, 0 });
      ofRect(0, 0, 4, mHeight, 0);
      ofPopStyle();
   }
}


/////////////
///Sampler///
/////////////

//Now in its own file --> SongCanvasRackSampler.cpp/h

////////////////////////
///MODULATOR ENVELOPE///
////////////////////////

SongCanvasRackModEnvelope::SongCanvasRackModEnvelope(const std::string& partName, SongCanvas* songCanvas)
: SongCanvasRackElement(partName, GetFlowGridElementType(), songCanvas)
{
   //TODO
}
void SongCanvasRackModEnvelope::SetupCanvasPart(SongCanvasNote* element)
{
   element->mCurrentColor = mCanvasLFOColor;
}
void SongCanvasRackModEnvelope::SaveState(FileStreamOut& out)
{
   SongCanvasRackElement::SaveState(out);
}
void SongCanvasRackModEnvelope::LoadState(FileStreamIn& in, int rev)
{
   SongCanvasRackElement::LoadState(in, rev);
}

/////////////////////
///MODULATOR CURVE///
/////////////////////


///////////
///Keyer///
///////////

//ArkyonVeil's note: Designed for being the note equivalent of the Pulser.
//However, due to considerable overlap with other note making modules,
//it's been relegated to unimplemented status.
//Ideally 'Keyer' could exist as a hook from other modules into SC's canvas. Design pending.

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