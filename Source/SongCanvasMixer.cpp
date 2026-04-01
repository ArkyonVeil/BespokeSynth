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
   if (mCableOut!=nullptr)
      mOwner->RemovePatchCableSource(mCableOut);

   delete mVizBuffer;
   delete mAudioBuffer;
}
void SongCanvasMixer::CreateUIControls()
{
   mCableOut = new PatchCableSource(mOwner, kConnectionType_Audio);
   mCableOut->SetOverrideVizBuffer(mVizBuffer);
   mCableOut->SetOverrideCableDir(ofVec2f(0,1),PatchCableSource::Side::kBottom);
   mOwner->AddPatchCableSource(mCableOut);
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
   DrawTextNormal(ofToString(mMixerIndex),-8,-8,8);//Channel name

}
void SongCanvasMixer::Save(FileStreamOut& out)
{
   out<<mMixerVersion;

   out<<mMixerIndex;
   out<<mEnabled;
   out<<mVolume;
   out<<mPanLeft;
   out<<mPanRight;
}
void SongCanvasMixer::Load(FileStreamIn& in)
{
   int rev;
   in>>rev;

   in>>mMixerIndex;
   in>>mEnabled;
   in>>mVolume;
   in>>mPanLeft;
   in>>mPanRight;
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