//
// Created by ArkyonVeil on 13/03/2026.
//

class SongCanvas;

class PatchCableSource;
//Identifies a rack element. This class is unified and can potentially represent any rack variant, please use mVariantType to check and don't use stuff from the wrong variant <. >
class SongCanvasRackElement : public FlowGridElement, public ITimeListener, public IButtonListener
{
public:
   SongCanvasRackElement(SongCanvasElementVariant variantType, std::string name, SongCanvas* owner, const ofColor& overrideColor = ofColor::white);
   void Draw() override;
   void CreateUIControls(SongCanvas* owner);
   void OnMouseClick(bool rightClick) override;
   void SetName(std::string newName) const;
   void Excite(float excitePower)
   {
      if (mExcitePower < excitePower)
         mExcitePower = excitePower;
   } //Make it dance
   void SetExciteConstant(float excitePower) { mExciteConstant = excitePower; } //Make it do a base level of dancing, handy for long events.
   void OnEnter();
   void OnProcess();
   void OnExit();
   void SetActive(bool newState) { mActive = newState; }
   bool IsActive() { return mActive; }
   void HandleRightClickDropdown(int optionValue);
   NoteInterval GetInterval() { return mPulserInterval; }
   void SetInterval(NoteInterval interval) { mPulserInterval = interval; }

   std::string* GetName() { return mElementName; }
   void SetRenameState(bool newState) { mRenameActive = newState; }
   ~SongCanvasRackElement();
   void OnTimeEvent(double time) override;
   void LoadFile();
   void UpdateSample(Sample* sample);
   void ButtonClicked(ClickButton* button, double time) override;

   PatchCableSource* GetEnablerCable() { return mEnablerCable; }
   PatchCableSource* GetPulserCable() { return mPulserCable; }
   int mInternalRackID;

   std::vector<DropdownListElement> mRackPartRightClickOptions {};
   DropdownList* GetPulserIntervalDropdown() const { return mIntervalSelector; }

   //Enabler
   bool mEnablerInverted{ false };
   //Sampler
   Sample* mSample {};
   float mSamplerPitch { 0 };
   float mSamplerVolume { 0 };
   ClickButton* mSampleLoaderButton {};

   static constexpr int PreferredWidthEnabler { 90 };
   static constexpr int PreferredWidthPulser { 150 };
   static constexpr int PreferredWidthOnePulse { 90 };
   static constexpr int PreferredWidthSampler { 275 };
   static constexpr int PreferredWidthLFO { 90 };

   SongCanvasElementVariant mVariantType;

private:
   PatchCableSource* mEnablerCable;
   PatchCableSource* mPulserCable;
   PatchCableSource* mSamplerCable;
   std::string* mElementName;
   SongCanvas* mSongCanvas;
   DropdownList* mIntervalSelector{ nullptr };
   NoteInterval mPulserInterval = kInterval_8n;
   TransportListenerInfo* mTransportListenerInfo{ nullptr };

   bool mActive{ false };
   bool mRenameActive = false;
   float mExcitePower{ 0 };
   float mExciteConstant{ 0 };
   float mExciteDrag{ 0 };
   float mVariantExtraWidth{ 0 };
   double mLastClickTime{ 0 };



   int mDebugClick{0};
   int mInternalID{ 0 };
   TextEntry* mElementRenameTextBox;
};