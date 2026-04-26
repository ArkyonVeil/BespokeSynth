//
// Created by ArkyonVeil on 14/04/2026.
//


/////////////
///Sampler///
/////////////
#pragma once
#include "ADSRDisplay.h"
#include "SampleVoice.h"
#include "SongCanvasRackElement.h"

class SongCanvasRackSampler : public SongCanvasAudioRackElement, public IFloatSliderListener, public IIntSliderListener
{
public:
   SongCanvasRackSampler(const std::string& partName, SongCanvas* songCanvas);
   ~SongCanvasRackSampler();
   void KillAudio();
   std::string GetFlowGridElementType() const override { return "partsampler"; };
   void CreateUIControls() override;
   static IDrawableModule* Create() { return new SongCanvasRackSampler("Part", nullptr); };
   void OnEnter(SongCanvasNote* element) override;
   void OnExit(SongCanvasNote* element) override;
   int GetPitchFromCanvasElement(SongCanvasNote* element);
   int GetPitchFromCanvasElementSize(float size);
   float GetCanvasElementSizeFromPitch(int notePitch);
   void LoadFileSample();
   void UpdateSampleLength(SongCanvasNote* element);
   float GetPreferredWidth() const override;
   void ButtonClicked(ClickButton* button, double time) override;
   bool MouseMoved(float x, float y) override;
   void SetSample(Sample* sample);
   float GetSampleLengthForCanvasInPitchBase();
   void PlaySample(int notePitch = 48);
   void OnPostResize() override;
   void OnClicked(float x, float y, bool right) override;
   void SongCanvasOptionsUpdated() override;
    bool TestIntercepts(float x, float y, bool right) override;
   void FilesDropped(std::vector<std::string> files, int x, int y) override;
   void SampleDropped(int x, int y, Sample* sample) override;
   bool CanDropSample() const override { return true; }

   int GetModuleSaveStateRev() const override { return 1; };

   std::vector<DropdownListElement> GetRightClickOptions() override;
   void HandleRightClickDropdown(int optionValue) override;
   void ReloadAudioOptions();

   ChannelBuffer* GetBuffer() { return &mWriteBuffer; }
   int GetActiveOptions() const { return mVolumeEnabled + mADSREnabled;}

   //Canvas Stuff
   void SetupCanvasPart(SongCanvasNote* element) override;
   void DrawCanvasPartGraphics(SongCanvasNote* element, ofRectangle rect) override;
   float GetNotePosQuantizationOverride(SongCanvasNote* element, float input, int context) override;

   void OnTempoUpdated() override;

   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, int rev) override;
   void OnLoadFinish() override;
   void Process(double time) override;
   void DrawRackGraphics() override;
   void SetRackEnabled(bool enabled) override;

   void FloatSliderUpdated(FloatSlider* slider, float oldVal, double time) override {mMemVolume = slider->GetValue();};
   void IntSliderUpdated(IntSlider* slider, int oldVal, double time) override {};

   DrawAudioBufferSettings mDrawAudioBufferSettings;

   struct RackSampleButton
   {
      RackSampleButton(SongCanvasRackSampler* owner);
      void OnMouseMove(float x, float y);
      bool CheckIntercept(float x, float y);
      void OnClick(float x, float y, bool right);
      void Draw();
      void UpdateRect();
      void SetRect(float x, float y, float width, float height);

      bool mHoveredTotal{ false };
      bool mHoveredPlay{ false };
      bool mHoveredStop{ false };
      bool mHoveredName{ false };
      bool mHoveredDrag{ false };
      bool mDrawControlOptions{ false };
      Sample* mSample{ nullptr };
      ofRectangle mRect;
      ofRectangle mPlayButtonRect;
      ofRectangle mStopButtonRect;
      ofRectangle mDragButtonRect;
      SongCanvasRackSampler* mOwner;
      DrawAudioBufferSettings mDrawAudioBufferSettings;
      TextTruncationSettings mSampleRenderNameSettings;
      std::string mDisplayName;
      std::string mFullSampleName;

      const float kMinWidthDrawButtons = 45;
      float mNameDisplayAnimOffset;
      float mFullNameWidth;
   };
   RackSampleButton mSampleButton;

protected:
   float GetMinTextSpace() const override{ return 35;}

private:
   Sample* mSample{ nullptr };
   bool mLongSample{ false };
   PatchCableSource* mSamplerCable{ nullptr };
   bool mSCLoadingDone{ true };
   float mSampleBaseTimeDuration { -1.0f };
   float mSampleBaseOldTimeDuration { -1.0f };

   bool mExpandProperties { true };
   bool mExpandPropertiesButtonHovered { false };

   bool mVolumeEnabled { false };
   bool mADSREnabled { false };

   FloatSlider* mVolumeSlider { nullptr };
   ADSRDisplay* mADSRDisplay { nullptr };

   ofColor const kOptionNameColour{101,198,101,255};

   float mMemVolume { 0.5f };
   ADSR mMemADSR;

   float const kSamplerButtonWidthPref{ 76 };
   float const kSliderWidthPref { 40 };
   float const kADSRWidthPref { 45 };
   float const kOptionsPadding { 4 };
   float const kExpandPropertiesButtonWidthPref { 10 };

   float mExpandPropertiesWidth { 0 };
   ofVec2f mExpandPropertiesTrianglePos {0,0};
   ChannelBuffer mWriteBuffer;
   int mSamplesPlaying{ 0 };
   PolyphonyMgr mPolyMgr;
   SampleVoiceParams mVoiceParams{};
   float mSampleDisplayNameWidth{ 0 };
   const float mSampleDisplayNameWidthDefault{ 55 };
};