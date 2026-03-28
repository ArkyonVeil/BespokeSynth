//
// Created by ArkyonVeil on 13/03/2026.
//
#pragma once
#include "FlowGrid.h"


class SongCanvas;
class SongCanvas_CanvasElement;
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
   void SetExciteConstant(float excitePower) { mExciteConstant = excitePower; } //Make it do a base level of dancing, handy for long events.

   void CreateUIControls() override;
   virtual void OnEnter() = 0;
   virtual void OnProcess(){};
   virtual void OnExit() = 0;
   float GetPreferredWidth() const override;
   virtual void HandleRightClickDropdown(int optionValue){};
   virtual std::vector<DropdownListElement> GetRightClickOptions() { return {}; };
   virtual void DrawRackGraphics() = 0;


   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, int rev) override;

   //Save/load unique canvas part data.
   virtual void SaveCanvasPart(SongCanvas_CanvasElement* obj, FileStreamOut& out){};
   virtual void LoadCanvasPart(SongCanvas_CanvasElement* obj, FileStreamIn& in, int rev){};

   //Canvas stuff
   virtual void DrawCanvasPartGraphics(SongCanvas_CanvasElement* element, ofRectangle rect){};
   virtual void SetupCanvasPart(SongCanvas_CanvasElement* element){};

   std::string* GetName() { return mElementName; }
   void SetRenameState(bool newState) { mRenameActive = newState; }
   void OnTimeEvent(double time) override{};
   void OnClicked(float x, float y, bool right) override;
   void ButtonClicked(ClickButton* button, double time) override{};

   int mInternalRackID;
   bool mRackEnabled;

protected:
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
   void DrawModule() override;
   bool mRenameActive = false;
   float mExcitePower{ 0 };
   float mExciteConstant{ 0 };
   float mExciteDrag{ 0 };
   double mLastClickTime{ 0 };
   int mMaxNameSize = 32; //If the text is longer than this we truncate it

   float mLastRenameSize = 0;
   int mLastNameSize = 0;

   int mDebugClick{ 0 };
   TextEntry* mElementRenameTextBox;
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
   void OnEnter() override;
   void OnExit() override;
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
   void OnEnter();
   void OnExit();
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
   PatchCableSource* mKeyerCable;
   void OnEnter() override{};
   void OnExit() override{};
   void DrawRackGraphics() override;
   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, int rev) override;
};

/////////////
///Sampler///
/////////////

class SongCanvasRackSampler : public SongCanvasRackElement
{
public:
   SongCanvasRackSampler(const std::string& partName, SongCanvas* songCanvas);
   ~SongCanvasRackSampler();
   std::string GetFlowGridElementType() const override { return "partsampler"; };
   static IDrawableModule* Create() { return new SongCanvasRackSampler("Part", nullptr); };
   void OnEnter() override;
   void OnExit() override;
   void LoadFileSample();
   void ButtonClicked(ClickButton* button, double time);
   void SetSample(Sample* sample);
   void DrawRackGraphics() override;

   void SetupCanvasPart(SongCanvas_CanvasElement* element) override;

   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, int rev) override;

private:
   Sample* mSample{};
   float mSamplerPitch{ 0 };
   float mSamplerVolume{ 0 };
   ClickButton* mSampleLoaderButton{};
   PatchCableSource* mSamplerCable;
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
   void OnEnter() override{};
   void OnExit() override{};

   void SetupCanvasPart(SongCanvas_CanvasElement* element) override;

   void DrawRackGraphics() override{};
   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, int rev) override;

};