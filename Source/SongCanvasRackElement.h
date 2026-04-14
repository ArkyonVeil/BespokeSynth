//
// Created by ArkyonVeil on 13/03/2026.
//
#pragma once
#include "FlowGrid.h"
#include "PolyphonyMgr.h"
#include "Sample.h"
#include "SampleVoice.h"
#include "SongCanvasMixer.h"


class SongCanvas;
class SongCanvas_CanvasElement;
class SongCanvasMixer;
class PatchCableSource;

class SongCanvasRackFactory : public FlowGridElementFactory
{
public:
   ~SongCanvasRackFactory() = default;
   explicit SongCanvasRackFactory(SongCanvas* songCanvas)
   {
      mSongCanvas = songCanvas;
   }
   SongCanvas* mSongCanvas;
   FlowGridElement* Create(std::string typeName) override;
};

struct RackCanvasPartData
{
};
struct RackCanvasPartDataPulser : RackCanvasPartData
{
   //Unused for now.
};

//Identifies a rack element. This class is unified and can potentially represent any rack variant, please use mVariantType to check and don't use stuff from the wrong variant <. >
class SongCanvasRackElement : public FlowGridElement, public ITimeListener, public IButtonListener
{
public:
   SongCanvasRackElement(std::string partName, std::string internalName, SongCanvas* songCanvas);
   ~SongCanvasRackElement() override{};

   void SetPartName(std::string newName) const;
   void Excite(float excitePower)
   {
      if (mExcitePower < excitePower)
         mExcitePower = excitePower;
   } //Make it dance
   void SetExciteWiggle(float excitePower) { mExciteWiggle = excitePower; } //Make it do a base level of dancing, handy for long events.
   void SetExciteConstant(float excitePower) { mExciteConstant = excitePower; }

   void CreateUIControls() override;
   virtual void OnEnter(SongCanvas_CanvasElement* element) = 0;
   virtual void OnProcessTransport(){};
   virtual void OnExit(SongCanvas_CanvasElement* element) = 0;
   float GetPreferredWidth() const override;
   virtual void HandleRightClickDropdown(int optionValue){};
   virtual std::vector<DropdownListElement> GetRightClickOptions() { return {}; };
   virtual void DrawRackGraphics() = 0;


   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, int rev) override;
   virtual void OnLoadFinish(){}; //Runs after the Song Canvas is fully initialized. Normally this shouldn't be needed, but Ark -> Idiot

   //Save/load unique canvas part data.
   virtual void SaveCanvasPart(SongCanvas_CanvasElement* obj, FileStreamOut& out){};
   virtual void LoadCanvasPart(SongCanvas_CanvasElement* obj, FileStreamIn& in, int rev){};

   //Canvas stuff
   virtual void DrawCanvasPartGraphics(SongCanvas_CanvasElement* element, ofRectangle rect){};
   virtual void SetupCanvasPart(SongCanvas_CanvasElement* element){};
   virtual void DeletedCanvasPart(SongCanvas_CanvasElement* element){};

   std::string* GetName() { return mElementName; }
   void SetRenameState(bool newState) { mRenameActive = newState; }
   void OnTimeEvent(double time) override{};
   void OnClicked(float x, float y, bool right) override;
   bool MouseMoved(float x, float y) override;
   void MouseReleased() override;
   void ButtonClicked(ClickButton* button, double time) override{};
   float GetCenteredElementY(IUIControl* element) const { return (mHeight - element->GetRect(true).height) / 2; }

   virtual void SongCanvasOptionsUpdated(){};

   int mInternalRackID;
   bool mRackEnabled;

   std::unordered_map<int, RackCanvasPartData*> mCanvasPartData;

protected:
   void DrawModule() override;//Reserved for base graphics.
   virtual void DrawExtendedBaseGraphics(){};
   float GetLeftWidthPadding(); //Padding from left of rack to text
   float GetPartNameWidth() const; //Raw size of part text
   float GetGeneralReservedWidth(); //Includes size of part text + padding. Should place unique stuff after this.
   SongCanvas* mSongCanvas;
   std::string* mElementName;

   //Canvas part colours here for consistency and ease of comparison.
   const ofColor mCanvasEnablerColor = ofColor(180, 180, 180);
   const ofColor mCanvasEnablerColor2 = ofColor(100, 100, 100);
   const ofColor mCanvasEnablerInvertColor = ofColor(100, 100, 100);
   const ofColor mCanvasEnablerInvertColor2 = ofColor(50, 50, 50);
   const ofColor mCanvasPulserColor = ofColor(180, 180, 0);
   const ofColor mCanvasLFOColor = ofColor::purple;
   const ofColor mCanvasSamplerColor = ofColor(40, 180, 40);
   const ofColor mCanvasSamplerColor2 = ofColor(20, 70, 20);
   const ofColor mCanvasOnePulseColor = ofColor(80, 80, 0);

private:
   bool mRenameActive = false;
   float mExcitePower{ 0 };
   float mExciteWiggle{ 0 };
   float mExciteDrag{ 0 };
   float mExciteConstant{ 0 };
   double mLastClickTime{ 0 };
   int mMaxNameSize = 32; //If the text is longer than this we truncate it

   float mLastRenameSize = 0;
   float mDisplayStringPxWidth{ 0 };
   int mLastNameSize = 0;
   bool mBufferQuickRename = false;

   int mDebugClick{ 0 };
   TextEntry* mElementRenameTextBox;
};

//Rack elements that produce sound should inherit this for consistent support and convenience methods such as channel assigment.
class SongCanvasAudioRackElement : public SongCanvasRackElement, public IAudioSource, public ITextEntryListener
{
public:
   SongCanvasAudioRackElement(const std::string& partName, const std::string& internalName, SongCanvas* songCanvas)
   : SongCanvasRackElement(partName, internalName, songCanvas)
   {}
   ~SongCanvasAudioRackElement();

   SongCanvasMixer* GetMixer() { return mMixer; };
   ChannelBuffer* GetMixerBuffer() { return mMixerBuffer; };
   int GetMixerIndex() { return mMixerIndex; }
   void CreateUIControls() override;
   void SetMixer(SongCanvasMixer* mixer);
   void TextEntryComplete(TextEntry* entry) override;
   float GetPreferredWidth() const override;

   int GetNumTargets() override { return 0; };
   bool ShouldSuppressAutomaticOutputCable() override { return true; };
   void OnPostResize() override;

   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, int rev) override;

protected:
   void SwapMixers(int newIndex);
   int mMixerIndex{ -1 };

private:
   void DrawExtendedBaseGraphics() override;
   SongCanvasMixer* mMixer;
   ChannelBuffer* mMixerBuffer;
   TextEntry* mChannelPicker;
   bool mLoading{ false };
};

/////////////
///Enabler///
/////////////

class SongCanvasRackEnabler : public SongCanvasRackElement
{
public:
   SongCanvasRackEnabler(const std::string& partName, SongCanvas* owner);
   ~SongCanvasRackEnabler();
   std::string GetFlowGridElementType() const override { return "partenabler"; };
   void CreateUIControls() override;
   static IDrawableModule* Create() { return new SongCanvasRackEnabler("Part", nullptr); };
   bool mEnablerInverted{ false };
   void OnEnter(SongCanvas_CanvasElement* element) override;
   void OnExit(SongCanvas_CanvasElement* element) override;
   void DrawRackGraphics() override;

   void SetupCanvasPart(SongCanvas_CanvasElement* element) override;
   void DrawCanvasPartGraphics(SongCanvas_CanvasElement* element, ofRectangle rect) override;

   void HandleRightClickDropdown(int optionValue) override;
   std::vector<DropdownListElement> GetRightClickOptions() override;
   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, int rev) override;


private:
   PatchCableSource* mEnablerCable;
};

/////////////
///Pulser///
/////////////

class SongCanvasRackPulser : public SongCanvasRackElement, public IDropdownListener
{
public:
   SongCanvasRackPulser(const std::string& partName, SongCanvas* songCanvas);
   ~SongCanvasRackPulser();
   std::string GetFlowGridElementType() const override { return "partpulser"; };
   void OnEnter(SongCanvas_CanvasElement* element);
   void OnExit(SongCanvas_CanvasElement* element);
   void OnTimeEvent(double time);
   void DrawRackGraphics() override;
   void CreateUIControls() override;
   void Init() override;
   void OnPostResize() override;
   static IDrawableModule* Create() { return new SongCanvasRackPulser("Part", nullptr); };
   PatchCableSource* mPulserCable;
   NoteInterval GetInterval() { return mPulserInterval; }
   float GetPreferredWidth() const override;
   void SetInterval(NoteInterval interval) { mPulserInterval = interval; }

   void SetupCanvasPart(SongCanvas_CanvasElement* element) override;
   void DrawCanvasPartGraphics(SongCanvas_CanvasElement* element, ofRectangle rect) override;

   bool mOnePulseMode{ false };
   void HandleRightClickDropdown(int optionValue) override;
   std::vector<DropdownListElement> GetRightClickOptions() override;
   void DropdownUpdated(DropdownList* list, int oldVal, double time) override;
   void UpdateMode();
   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, int rev) override;

private:
   DropdownList* mIntervalSelector{ nullptr };
   NoteInterval mPulserInterval = kInterval_8n;
   TransportListenerInfo* mTransportListenerInfo{ nullptr };
};

///////////
///Keyer///
///////////

class SongCanvasRackKeyer : public SongCanvasRackElement
{
public:
   SongCanvasRackKeyer(const std::string& partName, SongCanvas* songCanvas);
   std::string GetFlowGridElementType() const override { return "partkeyer"; }
   float GetPreferredWidth() const override { return 90; }
   static IDrawableModule* Create() { return new SongCanvasRackKeyer("Part", nullptr); };


private:
   PatchCableSource* mKeyerCable{ nullptr };
   void OnEnter(SongCanvas_CanvasElement* element) override{};
   void OnExit(SongCanvas_CanvasElement* element) override{};
   void DrawRackGraphics() override;
   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, int rev) override;
};

/////////////
///Sampler///
/////////////

class SongCanvasRackSampler : public SongCanvasAudioRackElement
{
public:
   SongCanvasRackSampler(const std::string& partName, SongCanvas* songCanvas);
   ~SongCanvasRackSampler();
   void KillAudio();
   std::string GetFlowGridElementType() const override { return "partsampler"; };
   void CreateUIControls() override;
   static IDrawableModule* Create() { return new SongCanvasRackSampler("Part", nullptr); };
   void OnEnter(SongCanvas_CanvasElement* element) override;
   void OnExit(SongCanvas_CanvasElement* element) override;
   void LoadFileSample();
   float GetPreferredWidth() const override;
   void ButtonClicked(ClickButton* button, double time) override;
   bool MouseMoved(float x, float y) override;
   void SetSample(Sample* sample);
   void PlaySample();
   void OnPostResize() override;
   void OnClicked(float x, float y, bool right) override;
   void SongCanvasOptionsUpdated() override;
   bool TestClick(float x, float y, bool right, bool testOnly) override;
   void FilesDropped(std::vector<std::string> files, int x, int y) override;
   void SampleDropped(int x, int y, Sample* sample) override;
   bool CanDropSample() const override { return true; }


   ChannelBuffer* GetBuffer() { return &mWriteBuffer; }

   void SetupCanvasPart(SongCanvas_CanvasElement* element) override;
   void DrawCanvasPartGraphics(SongCanvas_CanvasElement* element, ofRectangle rect) override;

   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, int rev) override;
   void OnLoadFinish() override;
   void Process(double time) override;
   void DrawRackGraphics() override;

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

private:
   Sample* mSample{ nullptr };
   bool mLongSample{ false };
   PatchCableSource* mSamplerCable{ nullptr };
   bool mSCLoadingDone{ true };

   ChannelBuffer mWriteBuffer;
   int mSamplesPlaying{ 0 };
   PolyphonyMgr mPolyMgr;
   SampleVoiceParams mVoiceParams{};
   float mSampleDisplayNameWidth{ 0 };
   const float mSampleDisplayNameWidthDefault{ 55 };
   double mLastProcessTime;
};


/////////
///LFO///
/////////

class SongCanvasRackLFO : public SongCanvasRackElement
{
public:
   std::string GetFlowGridElementType() const override { return "partlfo"; };
   static IDrawableModule* Create() { return new SongCanvasRackLFO("Part", nullptr); }
   SongCanvasRackLFO(const std::string& partName, SongCanvas* songCanvas);
   void OnEnter(SongCanvas_CanvasElement* element) override{};
   void OnExit(SongCanvas_CanvasElement* element) override{};

   void SetupCanvasPart(SongCanvas_CanvasElement* element) override;

   void DrawRackGraphics() override{};
   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, int rev) override;
};