//
// Created by ArkyonVeil on 13/03/2026.
//
#pragma once
#include "FlowGrid.h"
#include "PolyphonyMgr.h"
#include "Sample.h"
#include "SongCanvasMixer.h"

class SongCanvas;
class SongCanvasNote;
class SongCanvasMixer;
class SongCanvasRackSampler;
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

class SongCanvasRackElement : public FlowGridElement, public ITimeListener, public IButtonListener
{
public:
   SongCanvasRackElement(std::string partName, std::string internalName, SongCanvas* songCanvas);
   ~SongCanvasRackElement() override = default;

   void SetPartName(std::string newName);
   void Excite(float excitePower)
   {
      if (mExcitePower < excitePower)
         mExcitePower = excitePower;
   } //Make it dance
   void SetExciteWiggle(float excitePower) { mExciteWiggle = excitePower; } //Make it do a base level of dancing, handy for long events.
   void SetExciteConstant(float excitePower) { mExciteConstant = excitePower; }

   void CreateUIControls() override;
   void UpdatePartNameData();
   virtual void OnEnter(SongCanvasNote* element) = 0;
   virtual void OnProcessTransport(){};
   virtual void OnExit(SongCanvasNote* element) = 0;
   float GetPreferredWidth() const override;
   virtual void HandleRightClickDropdown(int optionValue){};
   virtual std::vector<DropdownListElement> GetRightClickOptions() { return {}; };
   virtual void DrawRackGraphics() = 0;


   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, int rev) override;
   virtual void OnLoadFinish(){}; //Runs after the Song Canvas is fully initialized. Normally this shouldn't be needed, but Ark -> Idiot

   //Canvas stuff
   virtual void DrawCanvasPartGraphics(SongCanvasNote* element, ofRectangle rect){};
   virtual void SetupCanvasPart(SongCanvasNote* element){};
   virtual void DeletedCanvasPart(SongCanvasNote* element){};
   virtual float GetNotePosQuantizationOverride(SongCanvasNote* element, float input, int context) { return -1; }



   std::string* GetName() { return mElementName; }
   void SetRenameState(bool newState) { mRenameActive = newState; }
   void OnTimeEvent(double time) override{};
   void OnClicked(float x, float y, bool right) override;
   bool MouseMoved(float x, float y) override;
   void MouseReleased() override;
   void ButtonClicked(ClickButton* button, double time) override{};
   float GetCenteredElementY(IUIControl* element) const { return (mHeight - element->GetRect(true).height) / 2; }
   float GetCompression() const { return MIN(1,mWidth/GetPreferredWidth());}//Returns the current scale, in comparison to the preferred size. 1->no comp. 0.5->half comp. 0->infinite comp
   virtual ofVec2f GetOutputPos() const { return {mWidth-GetReservedRightWidth()/2,mHeight / 2};}
   virtual void OnTempoUpdated() {};

   bool IsRackEnabled() const { return mRackEnabled;}
   virtual void SetRackEnabled(bool enabled) { mRackEnabled = enabled;}

   virtual int GetRackNoteFactoryId() { return 0; }
   virtual void SongCanvasOptionsUpdated(){};

   int mInternalRackID;

protected:
   void DrawModule() override;//Reserved for base graphics.

   virtual void DrawExtendedBaseGraphics(){};

   virtual float GetMinTextSpace() const { return 55;}//Minimum space to reserve for text.
   float const kMaxTextSize { 200 };//How much room part name text can occupy before we truncate the excess.
   float const kLeftWidthPadding {8}; //Padding from left of rack to text
   float const kPartNameToContentPadding { 8 };//Padding from text to content.
   float const kGenericCableOutSpace { 32 };//Pixels reserved for outputs (usually cables). At right of panel.
   float const kPartNameFontSize { 12 };

   //Constants, for PreferredWidth Calculations
   //You're strongly discouraged from using these anywhere but GetPreferredWidth() checks.
   virtual float GetReservedPrefLeftWidth() const; //Reserved space for part name, plus padding.
   virtual float GetReservedPrefRightWidth() const { return kGenericCableOutSpace; };//Generally the same but some base rack types may alter this.

   //Dynamic, affected by compression and draw accurate.
   virtual float GetReservedLeftWidth() const {return GetReservedPrefLeftWidth()*GetCompression();}//Affected by compression. Should place unique stuff after this.
   virtual float GetReservedRightWidth() const {return GetReservedPrefRightWidth()*GetCompression();}//Affected by compression. Typically used for rack outputs.
   float GetLeftReservedToTextEnd() const;

   SongCanvas* mSongCanvas;
   std::string* mElementName;//The true name of the rack.
   std::string mDisplayPartName;//The displayed name of the rack.

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

   TextTruncationSettings mPartNameTruncationSettings;

private:

   float GetRenameSpaceUsed() const;
   bool mRenameActive = false;
   bool mRackEnabled;
   float mExcitePower{ 0 };
   float mExciteWiggle{ 0 };
   float mExciteDrag{ 0 };
   float mExciteConstant{ 0 };
   double mLastClickTime{ 0 };
   float mLastRenameSize { -1 };

   float mRackNameStringPreferredWidth{ 0 };
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

   SongCanvasMixer* GetMixer() const { return mMixer; };
   ChannelBuffer* GetMixerBuffer() const { return mMixerBuffer; };
   int GetMixerIndex() { return mMixerIndex; }
   void CreateUIControls() override;
   void SetMixer(SongCanvasMixer* mixer);
   void TextEntryComplete(TextEntry* entry) override;

   int GetNumTargets() override { return 0; };
   bool ShouldSuppressAutomaticOutputCable() override { return true; };
   void OnPostResize() override;
   bool MouseMoved(float x, float y) override;

   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, int rev) override;

protected:
   void SwapMixers(int newIndex);
   float GetReservedPrefRightWidth() const override { return 34;};
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
   void OnEnter(SongCanvasNote* element) override;
   void OnExit(SongCanvasNote* element) override;
   void DrawRackGraphics() override;
   void OnPostResize() override;

   void SetupCanvasPart(SongCanvasNote* element) override;
   void DrawCanvasPartGraphics(SongCanvasNote* element, ofRectangle rect) override;

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
   void OnEnter(SongCanvasNote* element);
   void OnExit(SongCanvasNote* element);
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

   void SetupCanvasPart(SongCanvasNote* element) override;
   void DrawCanvasPartGraphics(SongCanvasNote* element, ofRectangle rect) override;

   bool mOnePulseMode{ false };
   void HandleRightClickDropdown(int optionValue) override;
   std::vector<DropdownListElement> GetRightClickOptions() override;
   void DropdownUpdated(DropdownList* list, int oldVal, double time) override;
   void UpdateMode();
   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, int rev) override;
protected:
   float GetMinTextSpace() const override{ if (mOnePulseMode) return 55; return 45;}

private:
   DropdownList* mIntervalSelector{ nullptr };
   NoteInterval mPulserInterval = kInterval_8n;
   TransportListenerInfo* mTransportListenerInfo{ nullptr };
};

////////////////////////
///MODULATOR ENVELOPE///
////////////////////////

class SongCanvasRackModEnvelope : public SongCanvasRackElement
{
public:
   std::string GetFlowGridElementType() const override { return "partmodenvelope"; };
   static IDrawableModule* Create() { return new SongCanvasRackModEnvelope("Part", nullptr); }
   SongCanvasRackModEnvelope(const std::string& partName, SongCanvas* songCanvas);
   void OnEnter(SongCanvasNote* element) override{};
   void OnExit(SongCanvasNote* element) override{};

   void SetupCanvasPart(SongCanvasNote* element) override;

   void DrawRackGraphics() override{};
   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, int rev) override;
};

/////////////////////
///MODULATOR CURVE///
/////////////////////


///////////
///Keyer///
///////////

//Unimplemented, see note in SongCanvasRackElement.cpp

class SongCanvasRackKeyer : public SongCanvasRackElement
{
public:
   SongCanvasRackKeyer(const std::string& partName, SongCanvas* songCanvas);
   std::string GetFlowGridElementType() const override { return "partkeyer"; }
   float GetPreferredWidth() const override { return 90; }
   static IDrawableModule* Create() { return new SongCanvasRackKeyer("Part", nullptr); };

private:
   PatchCableSource* mKeyerCable{ nullptr };
   void OnEnter(SongCanvasNote* element) override{};
   void OnExit(SongCanvasNote* element) override{};
   void DrawRackGraphics() override;
   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, int rev) override;
};
