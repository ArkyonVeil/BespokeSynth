//
// Created by ArkyonVeil on 29/03/2026.
//

#pragma once
#include "SongCanvas.h"

class SongCanvasMixer {
public:
   SongCanvasMixer(SongCanvas* owner, int index);
   ~SongCanvasMixer();

   void CreateUIControls();
   ChannelBuffer* GetBuffer() { return mAudioBuffer;};//Gets this mixer's audio buffer. Useful for patching in internal sampler data.
   void Process();//Process the audio data.
   void Draw(float x, float y);//Draws the object. X,Y refers to the cable origin. Based on SongCanvas coords.
   void Save(FileStreamOut& out);
   void Load(FileStreamIn& in);
   void PostRepatch(PatchCableSource* cable, bool fromUserClick);

   int mMixerIndex;//Please do not write to this.
   int mMixerVersion{0};

private:
   SongCanvas* mOwner;
   RollingBuffer* mVizBuffer;//Animation buffer
   ChannelBuffer* mAudioBuffer;
   PatchCableSource* mCableOut;
   IAudioReceiver* mTarget {nullptr};
   int mNumChannels {1};

   float mPanLeft {1};
   float mPanRight {1};
   float mVolume {1};
   bool mEnabled {true};
};


