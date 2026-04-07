//
// Created by ArkyonVeil on 29/03/2026.
//

#pragma once
#include "SongCanvas.h"

class SongCanvasMixer : IFloatSliderListener
{
public:
   SongCanvasMixer(SongCanvas* owner, int index);
   ~SongCanvasMixer();

   void CreateUIControls();
   ChannelBuffer* GetBuffer() { return mAudioBuffer; }; //Gets this mixer's audio buffer. Useful for patching in internal sampler data.
   void Process(); //Process the audio data.
   void Draw(float x, float y); //Draws the object. X,Y refers to the cable origin. Based on SongCanvas coords.
   void DrawMixerControls(float x, float y);
   void Save(FileStreamOut& out);
   void Load(FileStreamIn& in);
   void PostRepatch(PatchCableSource* cable, bool fromUserClick);

   void MouseMove(float x, float y);
   void MouseClick(float x, float y, bool right);
   void SetMixerControlsVisibility(bool state);
   bool IsSelected();
   void FloatSliderUpdated(FloatSlider* slider, float oldVal, double time) override;

   int mMixerIndex; //Please do not write to this.
   int mMixerVersion{ 0 };

private:
   SongCanvas* mOwner;
   RollingBuffer* mVizBuffer; //Animation buffer
   ChannelBuffer* mAudioBuffer;
   PatchCableSource* mCableOut;
   IAudioReceiver* mTarget{ nullptr };

   FloatSlider* mPanSlider { nullptr };
   FloatSlider* mVolumeSlider { nullptr };

   int mNumChannels{ 1 };

   float mPan{ 0 };
   float mVolume{ 1 };
   bool mEnabled{ true };
   bool mHovered{ false };
   ofRectangle mRectBounds {-25,-16,50,16};

   ofColor mMixerBarBackgroundCol {15,15,15};
   ofColor mMixerBarGuideCol {70,80,80};
   ofColor mMixerBarForegroundCol {225,225,225};
};
