//
// Created by ArkyonVeil on 14/04/2026.
//

#include "SongCanvasRackSampler.h"

#include "INoteReceiver.h"
#include "ModularSynth.h"
#include "juce_audio_formats/juce_audio_formats.h"
using namespace juce;

SongCanvasRackSampler::SongCanvasRackSampler(const std::string& partName, SongCanvas* songCanvas)
: SongCanvasAudioRackElement(partName, GetFlowGridElementType(), songCanvas)
, mWriteBuffer(gBufferSize)
, mPolyMgr(this)
, mSampleButton(this)
{
   SetColor(ofColor::green);
   mVoiceParams.mVol = mMemVolume;
   mVoiceParams.mAdsr.Set(10, 0, 1, 10);
   mMemADSR = mVoiceParams.mAdsr;
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
   mVolumeSlider = new FloatSlider(this, "volume", 0, mHeight + 2, 30, 15, &mVoiceParams.mVol, 0, 2, 2);
   mVolumeSlider->SetShowName(false);
   mVolumeSlider->SetShowing(false);
   mVolumeSlider->SetHoldCableWhileHidden(true);

   mADSRDisplay = new ADSRDisplay(this, "ADSR", 3, mHeight + 2, 80, 50, &mVoiceParams.mAdsr);
   mADSRDisplay->SetShowing(false);
   mADSRDisplay->SetHoldCableWhileHidden(true);
}

void SongCanvasRackSampler::OnEnter(SongCanvas_CanvasElement* element)
{
   if (mSample)
   {
      if (!mSample->IsSampleLoading())
      {
         Excite(1);
         PlaySample(GetPitchFromCanvasElement(element));
      }
   }
}
void SongCanvasRackSampler::OnExit(SongCanvas_CanvasElement* element)
{
   //TODO
}

int SongCanvasRackSampler::GetPitchFromCanvasElement(SongCanvas_CanvasElement* element)
{
   if (!element)
      return 48;
   return GetPitchFromCanvasElementSize(element->GetEnd() - element->GetStart());
}

int SongCanvasRackSampler::GetPitchFromCanvasElementSize(float size)
{
   if (size <= 0.0f)
   {
      return 0;
   }
   float sampleSize = GetSampleLengthForCanvasInPitchBase();
   double ratio = static_cast<double>(sampleSize) / static_cast<double>(size);

   // Calculate the pitch shift in semitones (Delta P).
   // Delta P = 12 * log2(Ratio).
   double deltaPitch = 12.0 * std::log2(ratio);

   int calculatedPitch = static_cast<int>(std::round(48.0 + deltaPitch));

   return calculatedPitch;
}

float SongCanvasRackSampler::GetCanvasElementSizeFromPitch(int notePitch)
{
   float sampleSize = GetSampleLengthForCanvasInPitchBase();
   //Sometimes its good practice to cram a little bit of math. It goes a long way...
   //Theory still hazy though.
   //- Ark
   return sampleSize / exp2f((static_cast<float>(notePitch) - 48.0f) / 12);
}

//Updates a sample length visually based on the pitch reflected in its size.
void SongCanvasRackSampler::UpdateSampleLength(SongCanvas_CanvasElement* element)
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
}
bool SongCanvasRackSampler::MouseMoved(float x, float y)
{
   bool r = SongCanvasAudioRackElement::MouseMoved(x, y);
   mSampleButton.OnMouseMove(x, y);

   mExpandPropertiesButtonHovered = false;
   if (GetActiveOptions() > 0)
   {
      if (GetRectLocal().contains(x, y))
      {
         float s = mExpandPropertiesWidth / 2;
         mExpandPropertiesButtonHovered = x > mExpandPropertiesTrianglePos.x - s && x < mExpandPropertiesTrianglePos.x + s;
      }
   }
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
      tts.fontSize = 9;
      float sN = GetStringWidth(TruncateString(mSample->Name(), 160, tts), 8);
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

         if (vols > 0.05f || mSample->LengthInSeconds() >= 10)
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
         float sampleLength = GetSampleLengthForCanvasInPitchBase();
         for (auto e : r)
         {
            float start = e->GetStart();
            e->SetEnd(start + sampleLength);
            //e->
         }
      }
   }
}

float SongCanvasRackSampler::GetSampleLengthForCanvasInPitchBase()
{
   return mSample->LengthInSeconds() / (TheTransport->MsPerBar() / 1000) / mSongCanvas->GetMeasureCount();
}

void SongCanvasRackSampler::PlaySample(int notePitch)
{
   NoteMessage note(NextBufferTime(mSongCanvas), notePitch, 127);
   mPolyMgr.Start(note.time, note.pitch, note.velocity / 127.0f, note.voiceIdx, note.modulation);
}
void SongCanvasRackSampler::Process(double time)
{
   //PROFILE stuff is in the Song Canvas

   if (!mEnabled || mSample == nullptr)
      return;

   //Skip processing if no voices are currently playing.
   bool anyActive = false;
   for (int i = 0; i < kNumVoices; ++i)
   {
      const VoiceInfo& info = mPolyMgr.GetVoiceInfo(i);
      if (info.mPitch != -1)
      {
         anyActive = true;
         break;
      }
   }
   if (!anyActive)
      return;

   ComputeSliders(0);

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
      GetVizBuffer()->WriteChunk(mWriteBuffer.GetChannel(ch), mWriteBuffer.BufferSize(), ch); //This allows the element to bounce.
      Add(GetMixerBuffer()->GetChannel(ch), mWriteBuffer.GetChannel(ch), gBufferSize);
   }
}

void SongCanvasRackSampler::DrawRackGraphics()
{
   //if (gHoveredUIControl)
   //   DrawTextNormal(gHoveredUIControl->Name(),3,6,8);
   mSampleButton.Draw();
   if (GetActiveOptions())
   {
      float rot = 0;
      if (mExpandProperties)
      {
         rot = 90;
         float textYOffset = 25;
         if (mVolumeEnabled)
         {
            mVolumeSlider->Draw();
            float x, y;
            mVolumeSlider->GetPosition(x, y, true);
            float w = mVolumeSlider->GetRect(true).width;
            ofPushStyle();
            ofSetColor(kOptionNameColour);
            float sw = GetStringWidth("volume");
            float centeredX = x + (w - sw) / 2.0f;
            DrawTextNormal("volume", centeredX, y + textYOffset, 13);
            ofPopStyle();
         }
         if (mADSREnabled)
         {
            mADSRDisplay->Draw();
         }
      }
      if (mExpandPropertiesButtonHovered)
      {
         ofPushStyle();
         ofFill();
         ofSetColor(255, 255, 255, 25);
         float triX = mExpandPropertiesTrianglePos.x;
         float s = mExpandPropertiesWidth;
         ofRect(triX - s / 2, 0, s, mHeight, 1);
         ofPopStyle();
      }
      ofPushStyle();
      ofSetColor(0, 0, 0, 150);
      ofFill();
      ofTriangleShaped(mExpandPropertiesTrianglePos.x, mExpandPropertiesTrianglePos.y, MAX(2, mExpandPropertiesWidth - 1), rot);
      ofPopStyle();
   }
}
void SongCanvasRackSampler::SetRackEnabled(bool enabled)
{
   SongCanvasAudioRackElement::SetRackEnabled(enabled);
   if (!enabled)
   {
      KillAudio();
   }
}

float SongCanvasRackSampler::GetPreferredWidth() const
{
   float val = SongCanvasAudioRackElement::GetPreferredWidth();

   val += kSamplerButtonWidthPref;

   int optionsEnabled = mVolumeEnabled + mADSREnabled;
   if (mExpandProperties)
   {
      if (mVolumeEnabled)
         val += kSliderWidthPref;
      if (mADSREnabled)
         val += kADSRWidthPref;


      val += MAX(0, kOptionsPadding * optionsEnabled);
   }
   if (optionsEnabled > 0)
      val += kExpandPropertiesButtonWidthPref;

   return val;
}

void SongCanvasRackSampler::OnPostResize()
{
   SongCanvasAudioRackElement::OnPostResize();
   float workSpace = mWidth - (GetReservedLeftWidth() + GetReservedRightWidth()); //Available workspace.


   //We need to ensure everything fits.
   int optionsActive = GetActiveOptions();

   //3 Algs:

   float paddingTotalWidth = (MAX(0, optionsActive) * kOptionsPadding) * mExpandProperties;
   float sampleButtonWidth;
   float sliderWidth;
   float sliderA1Width = kSamplerButtonWidthPref * 0.6f;
   float adsrWidth;
   float paddingWidth;
   float expandPropertiesWidth;

   float optionsSpaceRequired = 0;
   if (optionsActive > 0)
   {
      optionsSpaceRequired += kExpandPropertiesButtonWidthPref;
      if (mExpandProperties)
         optionsSpaceRequired += (mVolumeEnabled * kSliderWidthPref) + (mADSREnabled * kADSRWidthPref);
   }

   workSpace = MAX(0.5f, workSpace);
   if (kSamplerButtonWidthPref + paddingTotalWidth + optionsSpaceRequired <= workSpace) //0-No Comp
   {
      expandPropertiesWidth = kExpandPropertiesButtonWidthPref;
      sampleButtonWidth = kSamplerButtonWidthPref;
      sliderWidth = kSliderWidthPref;
      adsrWidth = kADSRWidthPref;
      paddingWidth = kOptionsPadding;
   }
   else if (sliderA1Width + optionsSpaceRequired < workSpace) //1-Compress SampleB + Padding
   {
      float range = (kSamplerButtonWidthPref + paddingTotalWidth) - (sliderA1Width); //Working range, also difference.
      float ratio = (workSpace - sliderA1Width - optionsSpaceRequired) / range;

      expandPropertiesWidth = kExpandPropertiesButtonWidthPref;
      sampleButtonWidth = sliderA1Width + (kSamplerButtonWidthPref - sliderA1Width) * ratio;
      paddingWidth = kOptionsPadding * ratio;
      sliderWidth = kSliderWidthPref;
      adsrWidth = kADSRWidthPref;
   }
   else //2-All, padding removed.
   {
      float ratio = workSpace / (sliderA1Width + optionsSpaceRequired);
      expandPropertiesWidth = kExpandPropertiesButtonWidthPref * ratio;
      sampleButtonWidth = sliderA1Width * ratio;
      paddingWidth = 0;
      sliderWidth = kSliderWidthPref * ratio;
      adsrWidth = kADSRWidthPref * ratio;
   }
   mExpandPropertiesWidth = expandPropertiesWidth;

   mSampleButton.SetRect(GetReservedLeftWidth(), 1, MAX(5, sampleButtonWidth), mHeight - 2);

   if (optionsActive > 0)
   {
      float offsetX = GetReservedLeftWidth() + sampleButtonWidth + expandPropertiesWidth / 2;
      mExpandPropertiesTrianglePos = ofVec2f(offsetX, mHeight / 2);
      offsetX += expandPropertiesWidth / 2 + paddingWidth;
      if (mExpandProperties)
      {
         if (mVolumeEnabled)
         {
            mVolumeSlider->ClearOverridePatchCableInputDirection();
            mVolumeSlider->SetPosition(offsetX, 3);
            mVolumeSlider->SetDimensions(sliderWidth, 15);
            offsetX += sliderWidth + paddingWidth;
         }
         if (mADSREnabled)
         {
            mADSRDisplay->SetPosition(offsetX, 3);
            mADSRDisplay->SetDimensions(adsrWidth, mHeight - 6);
            //offsetX += adsrWidth + paddingSize; No other options planned.
         }
      }
      else //When they're not expanded, its good practice to move them to a specific location, to guide the cable hook points.
      {
         if (mVolumeEnabled)
         {
            mVolumeSlider->SetOverridePatchCableInputDirection(ofVec2f(0, 1));
            mVolumeSlider->SetPosition(offsetX, 2);
            mVolumeSlider->SetDimensions(1, 1);
            offsetX += 2;
         }
         if (mADSREnabled)
         {
            mADSRDisplay->SetPosition(offsetX, 2);
            mADSRDisplay->SetDimensions(1, 1);
            //offsetX += adsrWidth + paddingSize; No other options planned.
         }
      }
   }
}
void SongCanvasRackSampler::OnClicked(float x, float y, bool right)
{
   //Clicking the Sampler workspace typically attempts to play the sample  for quick feedback.
   //ofLog() << "Hotcake";
   bool flagUpdateRow = false;
   if (mExpandPropertiesButtonHovered)
   {
      mExpandProperties = !mExpandProperties;
      flagUpdateRow = true;
   }
   else if (mSample)
   {
      int previewMode = static_cast<int>(mSongCanvas->mSamplerAudioPreviewMode);

      if (!mSampleButton.CheckIntercept(x, y))
      {
         if (previewMode == 0 && !mLongSample)
         {
            PlaySample(48);
         }
         if (previewMode == 2) //Always
         {
            PlaySample(48);
         }
      }
   }

   mSampleButton.OnClick(x, y, right);

   SongCanvasAudioRackElement::OnClicked(x, y, right);
   if (flagUpdateRow)
      UpdateRow();
}
void SongCanvasRackSampler::SongCanvasOptionsUpdated()
{
   SongCanvasAudioRackElement::SongCanvasOptionsUpdated();

   mSampleButton.UpdateRect();
}
bool SongCanvasRackSampler::TestIntercepts(float x, float y, bool right)
{
   if (mSampleButton.CheckIntercept(x, y))
      return true;
   return SongCanvasAudioRackElement::TestIntercepts(x, y, right);
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
std::vector<DropdownListElement> SongCanvasRackSampler::GetRightClickOptions()
{
   std::vector<DropdownListElement> options;
   if (!mVolumeEnabled)
      options.push_back({ "Volume", 10 });
   else
      options.push_back({ "remove Volume", 11 });

   if (!mADSREnabled)
      options.push_back({ "ADSR", 14 });
   else
      options.push_back({ "remove ADSR", 15 });

   //options.push_back({ "---", -1 });
   //options.push_back({ "options", 18 });
   return options;
}
void SongCanvasRackSampler::HandleRightClickDropdown(int optionValue)
{
   if (optionValue == 10)
      mVolumeEnabled = true;
   if (optionValue == 11)
      mVolumeEnabled = false;
   if (optionValue == 14)
      mADSREnabled = true;
   if (optionValue == 15)
      mADSREnabled = false;

   /*if (optionValue == 18)
   {
   }*/

   ReloadAudioOptions();
}

void SongCanvasRackSampler::ReloadAudioOptions()
{
   if (mVolumeEnabled)
   {
      mVolumeSlider->SetValue(mMemVolume, NextBufferTime(false));
      mVolumeSlider->SetShowing(true);
   }
   else
   {
      mVolumeSlider->SetShowing(false);
      mMemVolume = mVoiceParams.mVol;
      mVoiceParams.mVol = 0.5f;
      IUIControl::DestroyCablesTargetingControls(std::vector<IUIControl*>{ mVolumeSlider });
   }

   if (mADSREnabled)
   {
      mADSRDisplay->SetShowing(true);
      mVoiceParams.mAdsr = mMemADSR;
   }
   else
   {
      mADSRDisplay->SetShowing(false);
      mMemADSR = mVoiceParams.mAdsr;
      mVoiceParams.mAdsr.Clear();
      mVoiceParams.mAdsr.Set(10, 0, 1, 10);
   }

   UpdateRow();
}

void SongCanvasRackSampler::SetupCanvasPart(SongCanvas_CanvasElement* element)
{
   element->mCurrentColor = mCanvasSamplerColor;
   element->mCurrentColorGrad = mCanvasSamplerColor2;
   if (mSample && mSCLoadingDone)
   {
      float sampleLength = GetSampleLengthForCanvasInPitchBase();
      element->SetEnd(element->GetStart() + sampleLength);
   }
}
void SongCanvasRackSampler::DrawCanvasPartGraphics(SongCanvas_CanvasElement* element, ofRectangle rect)
{
   SongCanvasAudioRackElement::DrawCanvasPartGraphics(element, rect);

   if (mSample)
   {
      ofPushMatrix();
      ofTranslate(rect.x, rect.y + 9);

      //Draw the buffer
      DrawAudioBuffer(rect.width, rect.height - 9, mSample->Data(), mDrawAudioBufferSettings);
      auto ds = element->GetDragState();

      if (ds != CanvasElementDragOperation::kNotDragged)
      {
         if (ds == CanvasElementDragOperation::kLeftDrag || ds == CanvasElementDragOperation::kRightDrag)
         {
            float fontSize = 8;
            int pitch = GetPitchFromCanvasElement(element);
            std::string sizeText;
            bool renderAll = false;
            float allMaxWidth;
            float allExtraYOffset = 0;
            int offsetY = rect.height - 11;


            switch (mSongCanvas->mSamplerAudioResizeText)
            {
               case SongCanvas::EnumSamplerAudioResizeText::Scale:
                  sizeText = NoteName(pitch, false, true);
                  break;
               case SongCanvas::EnumSamplerAudioResizeText::Pitch:
                  sizeText = ofToString(pitch);
                  break;
               case SongCanvas::EnumSamplerAudioResizeText::Percent:
               {
                  float pitchDifference = 48 - pitch;
                  float percent = roundf(powf( 2,pitchDifference / 12.0f) * 100.0f);
                  sizeText = ofToString(percent) + "%";
                  break;
               }
               case SongCanvas::EnumSamplerAudioResizeText::Time:
               {
                  float sampleLength = mSample->LengthInSeconds();
                  float pitchDifference = 48 - pitch;
                  float percent = powf( 2,pitchDifference / 12.0f);
                  sizeText = ofGetSecondsToTimeMMSS(sampleLength * percent, false, false, true);
                  break;
               }
               case SongCanvas::EnumSamplerAudioResizeText::All:
               {
                  renderAll = true;

                  float pitchDifference = 48 - pitch;
                  float percent = powf( 2,pitchDifference / 12.0f);
                  float sampleLength = mSample->LengthInSeconds();
                  sizeText = NoteName(pitch, false, true);

                  std::string txS1, txS2;
                  txS1 = ofToString(roundf(percent*100)) + "%";
                  txS2 = ofGetSecondsToTimeMMSS(sampleLength * percent, false, false, true);

                  allMaxWidth = MAX(gFont.GetStringWidth(txS1, fontSize),gFont.GetStringWidth(txS2, fontSize));

                  sizeText += "\n"+ofToString(pitch);
                  sizeText += "\n"+txS1;
                  sizeText += "\n"+txS2;
               }
               break;
            }

            float textWidth;
            float backHeight = fontSize - 1.0f;
            if (renderAll)
            {
               offsetY += 13;
               backHeight = gFont.GetStringHeight(sizeText,fontSize)*4+3;
               textWidth = allMaxWidth+2.0f;
               allExtraYOffset -= 2.0f;
            }
            else
            {
               textWidth = gFont.GetStringWidth(sizeText, fontSize);
            }

            float pad = 3.0f;
            if (textWidth + pad < rect.width || renderAll)
            {
               ofRectangle backgroundRect;
               if (ds == CanvasElementDragOperation::kLeftDrag)
                  backgroundRect = { 0.f, offsetY - fontSize + 2.f + allExtraYOffset, textWidth + pad * 2.f, backHeight };
               else if (ds == CanvasElementDragOperation::kRightDrag)
                  backgroundRect = { rect.width - textWidth - pad * 2.f, offsetY - fontSize + 2.f + allExtraYOffset, textWidth + pad * 2.f, backHeight };

               ofPushStyle();
               ofFill();
               ofSetColor(ofColor::black, 125);
               ofRect(backgroundRect);
               ofPopStyle();

               ofPushStyle();
               if (pitch == 48)
                  ofSetColor(ofColor::cyan);
               else
                  ofSetColor(ofColor::white);

               if (ds == CanvasElementDragOperation::kLeftDrag)
                  DrawTextNormal(sizeText, 2, offsetY, fontSize);
               else if (ds == CanvasElementDragOperation::kRightDrag)
               {
                  if (!renderAll)
                     DrawTextRightJustify(sizeText, rect.width - 2, offsetY, fontSize);
                  else
                     DrawTextNormal(sizeText, rect.width - textWidth -1, offsetY, fontSize);
               }
               ofPopStyle();
            }
         }
      }

      ofPopMatrix();
   }
}


float SongCanvasRackSampler::GetCustomCanvasElementQuantization(SongCanvas_CanvasElement* element, float input, int context)
{
   //Goal is to evenly snap these to pitch.
   int side = context; //0 = Left side dragging, 1 = Right side dragging.

   //The input is 0-1, 0 at the start hand of the canvas, and 1 at the end of the canvas.
   //We will set our snap alignment to the opposite side to the side being dragged.
   float size;

   //We also need to know the ends, thankfully they have already been normalized.
   float start = element->GetStart();
   float end = element->GetEnd();

   //Let's also get the min max sizes and use those as caps.
   float minPitchSize = GetCanvasElementSizeFromPitch(127);
   float maxPitchSize = GetCanvasElementSizeFromPitch(0);


   if (side == 0) //If we're dragging the left side, we'll snap backwards from the end
      size = MAX(minPitchSize, end - input);
   else //If we're dragging the right side, we'll snap from the start.
      size = MAX(minPitchSize, input - start);


   //Now that we have a side, let's get a pitch estimate
   int elPitch = GetPitchFromCanvasElementSize(size);

   //Now we estimate the true size of the sample at that pitch
   float pitchSize = GetCanvasElementSizeFromPitch(elPitch);
   pitchSize = CLAMP(pitchSize, minPitchSize, maxPitchSize);

   //Now we have everything we need. Let's calculate the snap position.
   if (side == 0)
      return end - pitchSize;
   else
      return start + pitchSize;
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

   out << mVolumeEnabled;
   out << mADSREnabled;
   out << mExpandProperties;

   out << mMemVolume;
   mMemADSR = mVoiceParams.mAdsr;
   mMemADSR.SaveState(out);
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

   if (rev >= 1)
   {
      in >> mVolumeEnabled;
      in >> mADSREnabled;
      in >> mExpandProperties;

      in >> mMemVolume;
      mMemADSR.LoadState(in);
   }

   ReloadAudioOptions();
}
void SongCanvasRackSampler::OnLoadFinish()
{
   SongCanvasAudioRackElement::OnLoadFinish();

   mSCLoadingDone = true;
}

///////////////////
///Sample Button///
///////////////////

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
         mOwner->PlaySample(48);
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
      if (!mOwner->IsHovered())
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
      mDisplayName = TruncateString(mDisplayName, width - 5, mSampleRenderNameSettings);
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