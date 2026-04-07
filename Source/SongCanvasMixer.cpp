//
// Created by ArkyonVeil on 29/03/2026.
//
#define VIZ_BUFFER_SECONDS .1f

#include "SongCanvasMixer.h"
SongCanvasMixer::SongCanvasMixer(SongCanvas* owner, int index)
{
   mOwner = owner;
   mMixerIndex = index;
   mVizBuffer = new RollingBuffer(VIZ_BUFFER_SECONDS * gSampleRate);
   mAudioBuffer = new ChannelBuffer(gBufferSize);
}
SongCanvasMixer::~SongCanvasMixer()
{
   if (mCableOut != nullptr)
      mOwner->RemovePatchCableSource(mCableOut);

   delete mVizBuffer;
   delete mAudioBuffer;
}
void SongCanvasMixer::CreateUIControls()
{
   mCableOut = new PatchCableSource(mOwner, kConnectionType_Audio);
   mCableOut->SetOverrideVizBuffer(mVizBuffer);
   mCableOut->SetOverrideCableDir(ofVec2f(0, 1), PatchCableSource::Side::kBottom);
   mOwner->AddPatchCableSource(mCableOut);
   mVolumeSlider = new FloatSlider(mOwner,("volume"+ofToString(mMixerIndex)).c_str(),0,0,108,13,&mVolume,0,1);
   mPanSlider = new FloatSlider(mOwner,("pan"+ofToString(mMixerIndex)).c_str(),0,0,108,13,&mPan,-1,1);

   mVolumeSlider->SetOverrideDisplayName("volume");
   mPanSlider->SetOverrideDisplayName("pan");

   SetMixerControlsVisibility(false);
}

void SongCanvasMixer::Process()
{
   mNumChannels = GetBuffer()->NumActiveChannels();

   //Sync with output buffer
   if (mTarget)
   {
      ChannelBuffer* out = mTarget->GetBuffer();
      out->SetNumActiveChannels(MAX(mNumChannels, out->NumActiveChannels()));
   }
   mVizBuffer->SetNumChannels(mNumChannels);

   //Send the Data
   if (mTarget)
   {
      ChannelBuffer* out = mTarget->GetBuffer();
      for (int ch = 0; ch < mNumChannels; ++ch)
      {
         Add(out->GetChannel(ch), GetBuffer()->GetChannel(ch), GetBuffer()->BufferSize());
      }
   }

   //A n i m a t e
   for (int ch = 0; ch < GetBuffer()->NumActiveChannels(); ++ch)
   {
      mVizBuffer->WriteChunk(GetBuffer()->GetChannel(ch), GetBuffer()->BufferSize(), ch);
   }

   //Flush our stuff
   GetBuffer()->Reset();
}

void SongCanvasMixer::Draw(float x, float y)
{
   mCableOut->SetManualPosition(x, y);
   ofPushStyle();
   ofSetColor(ofColor::cyan);
   DrawTextNormal(ofToString(mMixerIndex), x-24, y-mRectBounds.height-2, 10); //Channel name

   //Indent Selection
   ofSetColor(ofColor(0,0,0,50));
   ofFill();
   ofRect(x+mRectBounds.x,y+mRectBounds.y,mRectBounds.width,mRectBounds.height+5);

   //Background
   float offsetX;
   float offsetY = y-12;
   float barSize = 42;

   offsetX = x - barSize/2;

   float mMixerBarVPanHeight = 5;

   ofFill();
   ofSetColor(mMixerBarBackgroundCol);
   ofRect(offsetX,offsetY, barSize, mMixerBarVPanHeight,0);


   //Foreground

   //How it works, ex:
   //Volume 1, Pan 0, offset 0%, fill 100%
   //Volume 0, Pan 0, offset 50%, fill 0%;
   //Volume 1, Pan -1, offset 0%, fill 50%;
   //Volume 1, Pan 1, offset 50%, fill 50%;
   float chL = (1-MAX(0,mPan))*mVolume;
   float chR = (1-MAX(0,-mPan))*mVolume;

   float fillWidth = (chL*barSize*0.5f)+(chR*barSize*0.5f);
   float fillOffset = (barSize/2)-(barSize*chL*0.5);

   if (fillWidth>1)//Only bother rendering if there's enough pixels.
   {
      ofSetColor(ofColor(235,235,235));
      ofRect(offsetX + fillOffset, offsetY, fillWidth, mMixerBarVPanHeight, 0);
   }


   //Lefthand Marker
   ofSetColor(mMixerBarGuideCol);
   ofRect(offsetX-2,offsetY,2,mMixerBarVPanHeight,0);

   //Middle Marker
   ofRect(offsetX+barSize/2-1,offsetY,2,mMixerBarVPanHeight,0);

   //Righthand Marker
   ofRect(offsetX+barSize,offsetY,2,mMixerBarVPanHeight,0);

   if (mHovered)
   {
      ofNoFill();
      ofSetColor(ofColor::cyan);
      ofRect(x+mRectBounds.x,y+mRectBounds.y,mRectBounds.width,mRectBounds.height+5);
   }
   if (!IsSelected())
   {
      float miniPosX, miniPosY;
      miniPosX = x-24;
      miniPosY = y-8;
      mPanSlider->SetPosition(miniPosX,miniPosY);
      mVolumeSlider->SetPosition(miniPosX,miniPosY);
   }

   ofPopStyle();
}

void SongCanvasMixer::DrawMixerControls(float x, float y)
{
   mPanSlider->SetPosition(x+6,y+6);
   mVolumeSlider->SetPosition(x+6,y+6+18);

   mPanSlider->Draw();
   mVolumeSlider->Draw();
}

void SongCanvasMixer::Save(FileStreamOut& out)
{
   out << mMixerVersion;

   out << mMixerIndex;
   out << mEnabled;
   out << mVolume;
   out << mPan;
}
void SongCanvasMixer::Load(FileStreamIn& in)
{
   int rev;
   in >> rev;

   in >> mMixerIndex;
   in >> mEnabled;
   in >> mVolume;
   in >> mPan;
}
void SongCanvasMixer::PostRepatch(PatchCableSource* cable, bool fromUserClick)
{
   if (mCableOut != cable)
      return; //Check if it's our cable first.

   if (mCableOut->GetTarget())
   {
      mTarget = dynamic_cast<IAudioReceiver*>(mCableOut->GetTarget());
   }
   else
   {
      mTarget = nullptr;
   }
}
//Its Relative-> 0,0 on cable location.
void SongCanvasMixer::MouseMove(float x, float y)
{
   mHovered = mRectBounds.contains(x,y);
}

//Its Relative-> 0,0 on cable location.
void SongCanvasMixer::MouseClick(float x, float y, bool right)
{
   if (right)
      return;
   bool inBoundClick = mRectBounds.contains(x, y);

   if (inBoundClick)
   {
      if (IsSelected())
      {
         SetMixerControlsVisibility(false);
         mOwner->SetMixerControlsState(false, mMixerIndex);
      }
      else
      {
         //If another one is active
         if (mOwner->GetSelectedMixer()!=nullptr)
         {
            auto oM = mOwner->GetSelectedMixer();
            oM->SetMixerControlsVisibility(false);
         }

         SetMixerControlsVisibility(true);
         mOwner->SetMixerControlsState(true, mMixerIndex);
      }
   }
}

void SongCanvasMixer::SetMixerControlsVisibility(bool state)
{
   mVolumeSlider->SetNoHover(!state);
   mPanSlider->SetNoHover(!state);
   if (state)
   {
      mVolumeSlider->SetDimensions(108,13);
      mPanSlider->SetDimensions(108,13);
   }
   else
   {
      mVolumeSlider->SetDimensions(1,1);
      mPanSlider->SetDimensions(1,1);
   }
}

bool SongCanvasMixer::IsSelected()
{
   return mOwner->GetSelectedMixer() == this;
}
void SongCanvasMixer::FloatSliderUpdated(FloatSlider* slider, float oldVal, double time)
{

}