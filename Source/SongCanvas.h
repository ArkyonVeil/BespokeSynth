/**
    bespoke synth, a software modular synthesizer
    Copyright (C) 2021 Ryan Challinor (contact: awwbees@gmail.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/
//
//  SongCanvas.h
//  Bespoke
//
//  Module assembled by ArkyonVeil on April/24.
//
//

#pragma once

#include "Transport.h"
#include "Checkbox.h"
#include "Canvas.h"
#include "CanvasScrollbar.h"
#include "CanvasTimeline.h"
#include "Slider.h"
#include "ClickButton.h"
#include "DropdownList.h"
#include "ISignalListener.h"
#include "SongCanvas_CanvasElement.h"
#include "TextEntry.h"
#include "UIFlowGrid.h"


class SongCanvas_CanvasElement;
class SongCanvasRackElement;
class SongCanvas : public IDrawableModule,
                   public ICanvasListener,
                   public ITextEntryListener,
                   public IFloatSliderListener,
                   public IButtonListener,
                   public IDropdownListener,
                   public IAudioPoller,
                   public ISignalListener,
                   public ITimeListener,
                   public ICanvasTimelineListener
{
public:
   SongCanvas();
   ~SongCanvas();
   static IDrawableModule* Create() { return new SongCanvas(); }
   static bool AcceptsAudio() { return false; }
   static bool AcceptsNotes() { return false; }
   static bool AcceptsPulses() { return false; }

   void CreateUIControls() override;

   void SetEnabled(bool enabled) override { mEnabled = enabled; }
   bool IsResizable() const override { return true; }
   void Resize(float w, float h) override;
   void Init() override;

   virtual void LoadLayout(const ofxJSONElement& moduleInfo) override;
   virtual void SetUpFromSaveData() override;
   virtual void SaveLayout(ofxJSONElement& moduleInfo) override;
   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, int rev) override;

   bool IsEnabled() const override { return mEnabled; }
   void CanvasUpdated(Canvas* canvas) override;
   void ResizeWorkspace(float diff);
   ofColor GetRowColor(int row) const { return mRowColors[row % mRowColors.size()]; };
   void TextEntryComplete(TextEntry* entry) override;
   void ReloadMeasures(bool overrideAutoFit);
   void FloatSliderUpdated(FloatSlider* slider, float oldVal, double time) override;
   void ButtonClicked(ClickButton* button, double time) override;
   void CheckboxUpdated(Checkbox* checkbox, double time) override;
   bool MouseMoved(float x, float y) override;
   void OnClicked(float x, float y, bool right) override;
   void MouseReleased() override;
   void DropdownUpdated(DropdownList* list, int oldVal, double time) override;
   void SetNewRackDropdownContext(SongCanvasRackElement* element);
   void SetSelectedRackElement(SongCanvasRackElement* element);
   void SetupCanvasElement(SongCanvas_CanvasElement* element);
   void ElementAdditionSuppressed(float posX, float posY) override;
   TextEntry* GetRackRenameTextbox() const { return mRackRenameTextBox; }
   void DeleteRackElement(SongCanvasRackElement* element) const;
   std::vector<SongCanvasRackElement*> GetAllRackElements() const; //These arrays are not cached, do not abuse.
   std::vector<SongCanvas_CanvasElement*> GetAllCanvasElementsOfRack(const SongCanvasRackElement* element) const;
   std::vector<SongCanvas_CanvasElement*> GetAllCanvasElementsOfLayer(int layerIndex) const;
   SongCanvasRackElement* GetRackElementWithID(int id);
   void UserUpdatedCanvasTimeline(float newLoopMin, float newLoopMax) override;

   void IncrementInternalRackId() { mInternalRackIDCounter++; }
   int GetInternalRackId() const { return mInternalRackIDCounter; }
   void OnTransportAdvanced(float amount) override;
   void ReceiveSignal(SignalId signalID) override;
   void ReceiveSignal(SignalGeneric signalComplex) override {}
   void DisposeElement(IClickable* element);

   DropdownList* GetRackRightClickDropdown() const { return mRackElementRightClickDropdown; }

   void OnTimeEvent(double time) override;
   void FeatureResize(int extraW, int extraH);
   int GetModuleSaveStateRev() const override { return 4; }

   enum EnumSongCanvasStyle
   {
      ESRed = 0,
      ESPink = 1,
      ESYellow = 2,
      ESCyan = 3,
      ESGreen = 4,
      ESOrange = 5,
      ESPurple = 6,
      ESBlue = 7,
      ESWhite = 8,
      ESBlack = 9,
      ESGlass = 10,
      ESCarbon = 11,
      ESCheckerboard = 12,
      ESTransport = 13,
      ESRGB = 14,
      ESPride = 15,
      ESTrans = 16
   };
   ofColor GetFancyStyleColour(EnumSongCanvasStyle style, float time);


   static std::array<ofColor,2> ESCarbonColours;
   static std::array<ofColor,3> ESRGBColours;
   static std::array<ofColor,6> ESPrideColours;
   static std::array<ofColor,4> ESTransColours;

private:
   struct SongCanvasLayer
   {
      ofColor baseColor;
      float offset;
      bool enabled;
      std::string layerName;
   };
   void UpdateEndMode();


   //IDrawableModule
   void DrawModule() override;
   void AddNewLayer(int index, SongCanvasLayer layer);
   void DeleteLayer(int index);
   void SyncCanvasToLayers();
   void MoveLayerTo(int oldIndex, int newIndex);
   bool IsCanvasElementActive(SongCanvas_CanvasElement* element) const;
   void ElementRemoved(CanvasElement* element) override;
   void ReloadHeader();


   Canvas* mCanvas{ nullptr };
   FloatSlider* mMeasureSlider{ nullptr };
   FloatSlider* mTransportSlider{};
   std::vector<ofColor> mRowColors{};
   ClickButton* mResetButton;
   ClickButton* mPlayPauseButton;
   ClickButton* mSyncButton;
   CanvasScrollbar* mMainScrollbarHorizontal{ nullptr };
   DropdownList* mOnEndMeasureDropdown;
   TextEntry* mRackRenameTextBox;
   std::string mRackRenameString;

   TextEntry* mMeasureBaseTextbox;
   TextEntry* mMeasureCountTextbox;
   TextEntry* mMeasureEndTextbox;

   Checkbox* mLocalModeCheckbox;

   TransportListenerInfo* mTransportListenerInfo{ nullptr };

   UIFlowGrid* mRackGrid;
   CanvasTimeline* mCanvasTimeline;

   static constexpr int MaxLayers = 101; //Further increasing this may cause crashes.
   float mTime{ 0 };
   double mCanvasRelativeTime{ 0 };
   bool mPartCanvasDirty{ false }; //If true, the Canvas will regenerate next tick.
   double mFlashRackStartTime{ 0 };

   bool mFlashRackEndTime{ false };

   int mStartCanvasXOffset{ 0 };
   static const int MinRowSize = 12;
   static const int StandardRowSize = 32;
   static const int mStandardMeasureSize = 48;
   static const int mDefaultMeasureSpawnAmount = 12;

   static const int mOffsetFromTopSpacing = 50; //old val 38, 44
   static const int LayersListWidthSize = 150;
   static const int AdvancedConfigHSize = 100;
   static const int FlowGridRowHeightSize = 32;

   bool mLocalMode{ false }; //If false, runs on local timing.
   int mMeasureStart{ 0 };
   int mMeasureCount{ 0 };
   int mMeasureEnd{ 0 }; //Secondary viewing style, translated internally to measure count.
   float mMeasureSize{ 0 };
   bool mAutoScaleMeasureCount{ true }; //If true, automatically increases the amount of measures the panel holds.
   bool mLoopOnEnd{ false };
   bool mResetButtonAlsoStops{ false };
   bool mStartEndMeasureMode{ false };
   bool mReloadMeasureLoadFlag{ false };
   bool mLocalSynced{ true }; //Used in local mode's on end loop
   bool mLocalStopped{ false };

   bool mPreviewRackSounds{ true }; //Sends a sound signal every time a rack part is interacted with.

   ofVec2f mHeaderSplitter1;
   ofVec2f mHeaderSplitter2;

   int GetRackGridStartYOffset() const
   {
      return mOffsetFromTopSpacing + mCanvas->GetHeight() + 8;
   }

   int mInternalRackIDCounter = 0;
   int mExtraColumns;
   int mFlowGridRows = 2;
   int mPartNameCount = 4;

   int GetCanvasStartXOffset() const
   {
      if (expertPanelEnabled)
         return LayersListWidthSize + AdvancedConfigHSize;
      return LayersListWidthSize;
   }


   //Chunkifies a large canvas into chunks in order to speed up part detection.
   std::vector<SongCanvas_CanvasElement*> mCanvasChunkList[201];
   int mChunkAmount = 10;

   std::vector<SongCanvas_CanvasElement*> mActiveElements{};

   std::array<TextEntry*, MaxLayers> mLayerNameTextbox = {};
   std::array<Checkbox*, MaxLayers> mLayerEnableCheckbox = {};
   std::array<ClickButton*, MaxLayers> mLayerSettingsButton = {};
   /*
   ClickButton* mLayerSettingsButton1;
   ClickButton* mLayerSettingsButton2;
   ClickButton* mLayerSettingsButton3;
   ClickButton* mLayerSettingsButton4;
   ClickButton* mLayerSettingsButton5;*/

   std::vector<SongCanvasLayer> seqLayers{};
   std::vector<SongCanvasLayer> layerBuffer{}; //The reason for this is clumsy <>3

   ClickButton* mRackAddNewButton;

   DropdownList* mRackAddNewDropdown;
   DropdownList* mRackElementRightClickDropdown;
   DropdownList* mListDropdownOptions;

   SongCanvasRackElement* mRightClickDropdownElementContext;
   SongCanvasRackElement* mSelectedRackElement;


   enum RackElementRightClickOptions
   {
      enumNothing,
      enumRename,
      enumDelete,
   };
   RackElementRightClickOptions mRackElementRightClickIndex;

   enum RackAddNewElementOptions
   {
      enumEnabler = 0,
      enumPulser = 1,
      enumModulator = 2,
      enumSample = 3,
      enumOnePulse = 4,
   };
   RackAddNewElementOptions mRackAddNewElementIndex;

   enum LayerDropDownOptions
   {
      enumLDPNothing,
      enumLDPMoveUp,
      enumLDPMoveDown,
      enumLDPAddNewLayerBelow,
      enumLDPDelete,
   };

   enum EnumOnEndMeasure
   {
      enumOEMContinue = 0,
      enumOEMLoop = 1,
      enumOEMStop = 2,
   };



   int mOnEndMeasure{ 0 };
   int mPreviousGlobalEndMeasure{ -1 };
   int mPreviousLocalEndMeasure{ -1 };

   EnumSongCanvasStyle mGlobalModeColor { ESRed };
   EnumSongCanvasStyle mLocalModeColor { ESPink };

   LayerDropDownOptions mLayerDropDownOptions;
   int mLayerDropdownOptionButtonIndex;

   //Expert variables
   bool expertPanelEnabled = false;
};

enum class SongCanvasElementVariant
{
   Enabler = 0,
   Pulser = 1,
   LFO = 2,
   Sampler = 3,
   OnePulse = 4,
};

class PatchCableSource;
//Identifies a rack element. This class is unified and can potentially represent any rack variant, please use mVariantType to check and don't use stuff from the wrong variant <. >
class SongCanvasRackElement : public UIFlowGridElement, public ITimeListener
{
public:
   SongCanvasRackElement(float preferredWidth, SongCanvasElementVariant variantType, std::string name, SongCanvas* owner, const ofColor& overrideColor = ofColor::white);
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
   NoteInterval GetInterval() { return mPulserInterval; }
   void SetInterval(NoteInterval interval) { mPulserInterval = interval; }

   std::string* GetName() { return mElementName; }
   void SetRenameState(bool newState) { mRenameActive = newState; }
   ~SongCanvasRackElement();
   void OnTimeEvent(double time) override;
   PatchCableSource* GetEnablerCable() { return mEnablerCable; }
   PatchCableSource* GetPulserCable() { return mPulserCable; }
   int mInternalRackID;

   DropdownList* GetPulserIntervalDropdown() const { return mIntervalSelector; }

   SongCanvasElementVariant mVariantType;

private:
   PatchCableSource* mEnablerCable;
   PatchCableSource* mPulserCable;
   std::string* mElementName;
   SongCanvas* mSSParent;
   DropdownList* mIntervalSelector{ nullptr };
   NoteInterval mPulserInterval = kInterval_8n;
   TransportListenerInfo* mTransportListenerInfo{ nullptr };

   bool mActive{ false };
   bool mRenameActive = false;
   float mExcitePower{ 0 };
   float mExciteConstant{ 0 };
   float mExciteDrag{ 0 };
   float mVariantExtraWidth{ 0 };

   int mInternalID{ 0 };
   TextEntry* mElementRenameTextBox;
};
