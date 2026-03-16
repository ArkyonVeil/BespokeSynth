//
// Created by ArkyonVeil on 13/03/2026.
//
#pragma once
#include "FlowGrid.h"

class SongCanvas;

class PatchCableSource;
//Identifies a rack element. This class is unified and can potentially represent any rack variant, please use mVariantType to check and don't use stuff from the wrong variant <. >
class SongCanvasRackElement : public FlowGridElement, public ITimeListener, public IButtonListener
{
public:
   SongCanvasRackElement(std::string partName, SongCanvas* songCanvas);
   ~SongCanvasRackElement();
   std::string GetPreferredName() override;
   void OnMouseClick(bool rightClick) override;
   void SetPartName(std::string newName) const;
   void Excite(float excitePower)
   {
      if (mExcitePower < excitePower)
         mExcitePower = excitePower;
   } //Make it dance
   void SetExciteConstant(float excitePower) { mExciteConstant = excitePower; } //Make it do a base level of dancing, handy for long events.

   virtual void CreateUIControls(SongCanvas* owner);
   virtual void OnEnter() = 0;
   virtual void OnProcess(){};
   virtual void OnExit() = 0;
   virtual void SetEnabled(bool newState) { enabled = newState; }
   bool IsEnabled() { return enabled; }
   virtual int GetPreferredWidth() = 0;
   virtual void HandleRightClickDropdown(int optionValue) {};
   virtual std::vector<DropdownListElement> GetRightClickOptions() {return {};};
   virtual void DrawRackGraphics() = 0;
   void SaveState(FileStreamOut& out) override = 0;
   void LoadState(FileStreamIn& in, int rev) override = 0;

   std::string* GetName() { return mElementName; }
   void SetRenameState(bool newState) { mRenameActive = newState; }
   void OnTimeEvent(double time) override;
   void ButtonClicked(ClickButton* button, double time) override;

   int mInternalRackID;

protected:
   SongCanvas* mSongCanvas;
   std::string* mElementName;
private:

   void DrawModule() override;
   bool enabled{ false };
   bool mRenameActive = false;
   float mExcitePower{ 0 };
   float mExciteConstant{ 0 };
   float mExciteDrag{ 0 };
   double mLastClickTime{ 0 };

   int mDebugClick{0};
   int mInternalID{ 0 };
   TextEntry* mElementRenameTextBox;
};

/////////////
///Enabler///
/////////////

class SongCanvasRackEnabler: public SongCanvasRackElement
{
public:
   SongCanvasRackEnabler(const std::string& partName, SongCanvas* owner);
   ~SongCanvasRackEnabler();
   int GetPreferredWidth() override { return  90; };
   bool mEnablerInverted{ false };
   void OnEnter() override;
   void OnExit() override;
   void DrawRackGraphics() override;

   void HandleRightClickDropdown(int optionValue) override;
   std::vector<DropdownListElement> GetRightClickOptions() override;
   void SaveState(FileStreamOut& out) override {};
   void LoadState(FileStreamIn& in, int rev) override {};

private:
   PatchCableSource* mEnablerCable;
};

/////////////
///Pulser///
/////////////

class SongCanvasRackPulser:public SongCanvasRackElement , IDropdownListener
{
public:
   SongCanvasRackPulser(const std::string& partName, SongCanvas* songCanvas);
   ~SongCanvasRackPulser();
   void OnEnter();
   void OnExit();
   void OnTimeEvent(double time);
   void DrawRackGraphics() override;
   PatchCableSource* mPulserCable;
   NoteInterval GetInterval() { return mPulserInterval; }
   void SetInterval(NoteInterval interval) { mPulserInterval = interval; }
   int GetPreferredWidth() override { return  150; };

   bool mOnePulseMode{ false };
   void HandleRightClickDropdown(int optionValue) override;
   std::vector<DropdownListElement> GetRightClickOptions() override;
   void DropdownUpdated(DropdownList* list, int oldVal, double time) override;//TODO
   void UpdateMode();
   void SaveState(FileStreamOut& out) override {};
   void LoadState(FileStreamIn& in, int rev) override {};

private:
   DropdownList* mIntervalSelector{ nullptr };
   NoteInterval mPulserInterval = kInterval_8n;
   TransportListenerInfo* mTransportListenerInfo{ nullptr };
};

///////////
///Keyer///
///////////

class SongCanvasRackKeyer:public SongCanvasRackElement
{
public:
   SongCanvasRackKeyer(const std::string& partName, SongCanvas* songCanvas);
   int GetPreferredWidth() override { return  150; };
private:
   PatchCableSource* mKeyerCable;
   void OnEnter() override {};
   void OnExit() override {};
   void DrawRackGraphics() override;
   void SaveState(FileStreamOut& out) override {};
   void LoadState(FileStreamIn& in, int rev) override {};
};

/////////////
///Sampler///
/////////////

class SongCanvasRackSampler:public SongCanvasRackElement
{
public:
   SongCanvasRackSampler(const std::string& partName, SongCanvas* songCanvas);
   ~SongCanvasRackSampler();
   void OnEnter() override;
   void OnExit() override;
   void LoadFileSample();
   void ButtonClicked(ClickButton* button, double time);
   void SetSample(Sample* sample);
   void DrawRackGraphics() override;
   int GetPreferredWidth() override { return 150; }
   void SaveState(FileStreamOut& out) override {};
   void LoadState(FileStreamIn& in, int rev) override {};
private:
   Sample* mSample {};
   float mSamplerPitch { 0 };
   float mSamplerVolume { 0 };
   ClickButton* mSampleLoaderButton {};
   PatchCableSource* mSamplerCable;
};

/////////
///LFO///
/////////

class SongCanvasRackLFO:public SongCanvasRackElement
{
   int GetPreferredWidth() override {return 90;};
   SongCanvasRackLFO(const std::string& partName, SongCanvas* songCanvas);
   void OnEnter() override {};
   void OnExit() override {};

   void DrawRackGraphics() override {};
   void SaveState(FileStreamOut& out) override {};
   void LoadState(FileStreamIn& in, int rev) override {};
};