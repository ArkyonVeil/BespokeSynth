//
// Created by ArkyonVeil on 29/03/2026.
//
#pragma once
#include "LevelMeterDisplay.h"
#include "SongCanvas.h"

class SongCanvasMixer : IFloatSliderListener
{
public:
   SongCanvasMixer(SongCanvas* owner, int index);
   ~SongCanvasMixer();
   void PreDispose();
   void AddOrphanTarget(IAudioReceiver* oldTarget, ofVec2f sourceCoord);

   void CreateUIControls();
   ChannelBuffer* GetBuffer() const { return mAudioBuffer; }; //Gets this mixer's audio buffer. Useful for patching in internal sampler data.
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

   ofRectangle GetRect() const { return { kRectBounds.x * comp, kRectBounds.y, kRectBounds.width * comp, kRectBounds.height }; }
   ofRectangle GetNameRect() const { return { mNameBounds.x * comp, mNameBounds.y, mNameBounds.width * comp, mNameBounds.height }; }
   float comp{ 1 };
   int mMixerIndex; //Please do not write to this.
   int const kMixerVersion{ 0 };
   bool mIsDeleted{ false };

private:
   SongCanvas* mOwner;
   RollingBuffer* mVizBuffer; //Animation buffer
   ChannelBuffer* mAudioBuffer;
   PatchCableSource* mCableOut;
   IAudioReceiver* mTarget{ nullptr };

   FloatSlider* mPanSlider{ nullptr };
   FloatSlider* mVolumeSlider{ nullptr };
   LevelMeterDisplay mLeftChannelLevel;
   LevelMeterDisplay mRightChannelLevel;

   int mNumChannels{ 1 };

   float mPan{ 0 };
   float mVolume{ 1 };
   bool mEnabled{ true };
   bool mHovered{ false };
   bool mHoverName{ false };
   ofRectangle const kRectBounds{ -25, -16, 50, 16 };
   ofRectangle mNameBounds{ -2, -2, 4, 4 };

   ofColor mMixerBarBackgroundCol{ 15, 15, 15 };
   ofColor mMixerBarGuideCol{ 70, 80, 80 };
   ofColor mMixerBarForegroundCol{ 225, 225, 225 };
};
