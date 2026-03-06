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
//
//  Bespoke
//  Aka the Song Canvas
//
//  Module assembled by ArkyonVeil on April/24 - June/24. Reheated in March/26
//
//         ██
//         ▓▓
// █▓      ██      ▓█
//   ▒█▒        ▒█▒
//        ████
//        ████
//   ▒█▒        ▒█▒
// █▓      ██      ▓█
//         ▓▓
//         ██
//

#include "SongCanvas.h"
#include "ModularSynth.h"
#include "SongCanvas_CanvasElement.h"

#include <cstring>
SongCanvas::SongCanvas()
{
   mRowColors.push_back(ofColor::black);
   seqLayers.reserve(MaxLayers + 1);
   mTransportPriority = kTransportPriorityVeryEarly;
   for (int i = 0; i < 5; i++)
   {
      layerBuffer.push_back(SongCanvasLayer{
      ofColor::white,
      0,
      true,
      "layer" + ofToString(i) });
   }
}

SongCanvas::~SongCanvas()
{
   mCanvas->SetListener(nullptr);
   TheTransport->RemoveAudioPoller(this);
}


void SongCanvas::CreateUIControls()
{
   IDrawableModule::CreateUIControls();

   if (expertPanelEnabled)
      mStartCanvasOffset = LayersListHSize + AdvancedConfigHSize;
   else
      mStartCanvasOffset = LayersListHSize;

   int cSize = mStandardMeasureSize * mDefaultMeasureSpawnAmount;
   mCanvas = new Canvas(this, mStartCanvasOffset, mOffsetFromTopSpacing, cSize, layerBuffer.size() * StandardRowSize, 1, 5, 12 * 4, &SongCanvas_CanvasElement::Create);
   AddUIControl(mCanvas);
   mCanvas->SetListener(this);

   mCanvas->SetDragMode(Canvas::kDragBoth);
   mCanvas->SetNumVisibleRows(5);
   mCanvas->SetMajorColumnInterval(4);
   //mCanvas->SetMajorColumnLineColor(ofColor{ 255, 255, 255, 50 });
   //mCanvas->SetMinorColumnLineColor(ofColor{ 255, 255, 255, 20 });

   //mCanvas->SetMajorColumnLineLength(1.0f);
   //mCanvas->SetMinorColumnLineLength(0.75f);
   mCanvas->SetInvertDragSnapBehavior(true);
   mCanvas->SetAllowElementPlacement(false);

   mMainScrollbarHorizontal = new CanvasScrollbar(mCanvas, "scrollh", CanvasScrollbar::Style::kHorizontal);
   AddUIControl(mMainScrollbarHorizontal);

   mTransportSlider = new FloatSlider(this, "measure", mStartCanvasOffset, 22, mCanvas->GetWidth(), 15, &mTime, 0, 32);
   mTransportSlider->SetNoHover(true);
   mTransportSlider->SetCableTargetable(false);
   mTransportSlider->SetTextAlpha(0);

   mModGrid = new UIFlowGrid("playrack", 8, GetModGridStartYOffset(), mCanvas->GetWidth() - 16 + GetCanvasStartXOffset(), 32, 2, this, this);

   mRackRenameTextBox = new TextEntry{ this, "rename", -500, -500, 7, &mRackRenameString };
   mRackRenameTextBox->SetRequireEnter(true);
   mRackRenameTextBox->SetFlexibleWidth(true);


   auto mgp = mModGrid->GetPosition(true);

   mRackAddNewButton = new ClickButton(this, "add", mgp.x + mModGrid->GetWidth(), mgp.y, ButtonDisplayStyle::kPlus);
   float bWidth;
   float bHeight;
   mRackAddNewButton->GetDimensions(bWidth, bHeight);
   mRackAddNewButton->SetPosition(mgp.x + mModGrid->GetWidth() - bWidth * 1.5, mgp.y);
   mRackAddNewButton->SetDimensions(bWidth * 1.5, mModGrid->GetHeight());
   mRackAddNewButton->SetIconAlignment(ButtonIconAlignment::kCenter);

   mModGrid->SetDimensions(mCanvas->GetWidth() - 16 + GetCanvasStartXOffset() - bWidth * 1.5f, mFlowGridRows * FlowGridRowHeightSize);

   //HACK, but this is just to avoid having to do further changes to the dropdown element.
   //TL DR: Don't draw it, but send over the events with the button.
   mRackAddNewDropdown = new DropdownList(this, "", mgp.x + mModGrid->GetWidth(), mgp.y, (int*)&mRackAddNewElementIndex);
   mRackAddNewDropdown->AddLabel("Enabler", RackAddNewElementOptions::enumEnabler);
   mRackAddNewDropdown->AddLabel("Pulser", RackAddNewElementOptions::enumPulser);
   mRackAddNewDropdown->AddLabel("OnePulse", RackAddNewElementOptions::enumOnePulse);

   mRackElementRightClickDropdown = new DropdownList(this, "", -100, -100, (int*)&mRackElementRightClickIndex);
   mRackElementRightClickDropdown->AddLabel("Delete", RackElementRightClickOptions::enumDelete);
   mRackElementRightClickDropdown->AddLabel("Rename", RackElementRightClickOptions::enumRename);

   mListDropdownOptions = new DropdownList(this, "", -100, -100, (int*)&mLayerDropDownOptions);

   mTransportTextBox = new TextEntry{ this, "transport", mStartCanvasOffset - 68, 22, 6, &mTime, 0, 99999 };
   //mTransportTextBox->SetFloatDecimalCount(2);


   mResetButton = new ClickButton{ this, "reset", static_cast<int>(mTransportTextBox->GetPosition().x) - 34, 22, ButtonDisplayStyle::kText };
   mPlayPauseButton = new ClickButton{ this, "play/pause", static_cast<int>(mResetButton->GetPosition().x) - 22, 22, ButtonDisplayStyle::kPlay };

   //Layers
   for (int i = 0; i < layerBuffer.size(); ++i)
   {
      AddNewLayer(i, layerBuffer[i]);
   }
   layerBuffer.clear();

   //Parts
   int dPartIter = 1;
   for (int i = 0; i < 3; ++i)
   {
      mModGrid->AddElement(new SongCanvasRackElement(90, SongCanvasElementVariant::Enabler, "Part " + std::to_string(dPartIter), this), 0);
      dPartIter++;
      IncrementInternalRackId();
   }


   //mLayerName[0]->SetNoHover(false);
}
void SongCanvas::Init()
{
   IDrawableModule::Init();
   mTransportListenerInfo = TheTransport->AddListener(this, kInterval_64n, OffsetInfo(0, true), true);
   TheTransport->AddAudioPoller(this);
}

void SongCanvas::DrawModule()
{
   mTime = TheTransport->GetMeasureTime(gTime);


   if (Minimized() || IsVisible() == false)
      return;

   float zoomPercent = abs(mCanvas->mViewStart - mCanvas->mViewEnd);

   int startCanvasOffset;
   if (expertPanelEnabled)
      startCanvasOffset = LayersListHSize + AdvancedConfigHSize;
   else
      startCanvasOffset = LayersListHSize;

   ofPushStyle();
   ofFill();
   //Draw the canvas.
   for (int i = 0; i < mCanvas->GetNumVisibleRows(); ++i)
   {
      ofColor color = GetRowColor(i + mCanvas->GetRowOffset());
      if (seqLayers[i].enabled)
      {
         if (i % 2 == 0)
            color.a = 50;
         else
            color.a = 30;
      }
      else
      {
         color.a = 255;
      }
      ofSetColor(color);

      float boxHeight = (float(mCanvas->GetHeight()) / mCanvas->GetNumVisibleRows());
      float y = mCanvas->GetPosition(true).y + i * boxHeight;

      ofRect(mCanvas->GetPosition(true).x, y, mCanvas->GetWidth(), boxHeight);
   }

   float drawPointOffset = startCanvasOffset;
   float tW = mCanvas->GetWidth();
   float tH = mOffsetFromTopSpacing + mCanvas->GetHeight();
   //ofSetColor(ofColor::clear);

   /*
   if (zoomPercent > 2)
      mCanvas->SetMinorColumnLineColor(ofColor::clear);
   else
   {
      mCanvas->SetMinorColumnLineColor(ofColor{ 255, 255, 255, 20 });
   }

   if (zoomPercent > 6)
      mCanvas->SetMajorColumnLineColor(ofColor::clear);
   else
   {
      if (zoomPercent > 4)
      {
         mCanvas->SetMajorColumnLineColor(ofColor{ 255, 255, 255, 20 });
      }
      else
         mCanvas->SetMajorColumnLineColor(ofColor{ 255, 255, 255, 50 });
   }*/

   mCanvas->Draw();
   //mCanvas->RescaleForZoom()

   ofColor softLineColor = ofColor{ 255, 255, 255, 20 };
   ofColor hardLineColor = ofColor{ 255, 255, 255, 50 };
   ofColor labelColor = ofColor{ 0, 0, 0, 130 };

   short iter = 0;
   float postZoomOffset = static_cast<float>(mStandardMeasureSize) / zoomPercent;
   float postZoomCanvasOffset = tW * mCanvas->mViewStart / zoomPercent;
   drawPointOffset -= postZoomCanvasOffset;

   mTransportSlider->SetDimensions(tW, 15);
   mTransportSlider->SetExtents(mCanvas->mViewStart * (tW / mStandardMeasureSize), mCanvas->mViewEnd * (tW / mStandardMeasureSize));
   mTransportSlider->Draw();
   mTransportTextBox->Draw();
   mResetButton->Draw();
   mPlayPauseButton->Draw();

   if (TheSynth->IsAudioPaused())
      mPlayPauseButton->SetDisplayStyle(ButtonDisplayStyle::kPlay);
   else
      mPlayPauseButton->SetDisplayStyle(ButtonDisplayStyle::kPause);


   int drawMeasureLabelIterInterval = MAX(1, std::ceil((zoomPercent * 2 - 1) / 2.5));
   int softLineClip = 1;
   int hardLineClip = 4;
   if (zoomPercent > 2.5)
      softLineClip = 2;
   if (zoomPercent > 4.0)
      softLineClip = 999999999;

   if (zoomPercent > 6.0)
      hardLineClip = 8;

   if (zoomPercent > 8.0)
      hardLineClip = 999999999;

   while (drawPointOffset < tW + startCanvasOffset)
   {
      if (drawPointOffset < startCanvasOffset)
      {
         iter++;
         drawPointOffset += postZoomOffset;
         continue;
      }
      if (iter % 4 == 0)
      {
         ofSetColor(hardLineColor);
      }
      else
      {
         ofSetColor(softLineColor);
      }
      if (iter % hardLineClip == 0 || iter % softLineClip == 0)
         ofLine(drawPointOffset, mOffsetFromTopSpacing - 16, drawPointOffset, mOffsetFromTopSpacing);

      if (iter % drawMeasureLabelIterInterval == 0)
      {
         ofSetColor(labelColor);
         DrawTextNormal(std::to_string(iter), drawPointOffset + 2, mOffsetFromTopSpacing - 2, 13);
      }
      //ofLine(drawPointerOffset, OffsetFromTopSpacing-20, drawPointerOffset, OffsetFromTopSpacing-5);
      drawPointOffset += postZoomOffset;
      iter++;
   }
   ofSetColor(ofColor::red);
   float markerLinePos = startCanvasOffset + mTime * postZoomOffset - postZoomCanvasOffset;
   if (markerLinePos > startCanvasOffset)
      ofLine(markerLinePos, mOffsetFromTopSpacing, markerLinePos, tH);
   ofSetColor(ofColor::grey);

   mMainScrollbarHorizontal->Draw();
   ofPopStyle();

   int s = seqLayers.size();
   float layerPosSpacing = mCanvas->GetHeight() / static_cast<float>(s);
   for (int i = 0; i < s; i++)
   {
      float midCentering = layerPosSpacing / 4; //Math on this is a bit spotty. Correct later.

      mLayerNameTextbox[i]->SetPosition(28, mOffsetFromTopSpacing + midCentering + i * layerPosSpacing);
      mLayerNameTextbox[i]->Draw();

      mLayerEnableCheckbox[i]->SetPosition(startCanvasOffset - 13, mOffsetFromTopSpacing + midCentering + i * layerPosSpacing);
      mLayerEnableCheckbox[i]->Draw();

      mLayerSettingsButton[i]->SetPosition(4, mOffsetFromTopSpacing + midCentering + i * layerPosSpacing);
      mLayerSettingsButton[i]->Draw();
   }
   //Draw the rack
   mModGrid->SetPosition(8, GetModGridStartYOffset());
   if (mFlashRackStartTime > 0)
   {
      mModGrid->SetBackgroundColour(255, 255, 255, CLAMP(40 + sin(ofGetGlobalTime() * 10) * 30, 0, 255));
      if (mFlashRackStartTime + 1 < ofGetGlobalTime())
      {
         mFlashRackStartTime = 0;
         mModGrid->SetBackgroundColour(0, 0, 0, 75);
      }
   }
   mModGrid->Draw();
   auto mgp = mModGrid->GetPosition(true);
   //Normally I'd put the following in something like a resize/setup method, but the Y coordinate
   //borks for some reason, and honestly I couldn't care less right now.
   mRackAddNewButton->SetPosition(mgp.x + mModGrid->GetWidth(), mgp.y);
   mRackAddNewButton->Draw();
   ofPushStyle();

   //DEBUG TEXT, UNCOMMENT FOR ENLIGHTENMENT
   /*
   std::string dText = std::to_string(mCanvas->GetLength()) + "\n";
   dText += std::to_string(mCanvas->GetNumCols()) + "\n";
   dText += std::to_string(mCanvasRelativeTime) + "\n";
   dText += std::to_string(mCanvas->GetWidth()) + "\n";
   DrawTextNormal(dText, 4, 8);*/
}

void SongCanvas::AddNewLayer(int index, SongCanvasLayer layer)
{
   seqLayers.push_back(layer);
   int layerCount = seqLayers.size();
   int lIndex = layerCount - 1;
   mCanvas->SetNumRows(layerCount);
   mCanvas->SetNumVisibleRows(layerCount);

   float layerPosSpacing = mCanvas->GetHeight() / static_cast<float>(layerCount);
   float midCentering = layerPosSpacing / 4;

   mLayerNameTextbox[lIndex] = new TextEntry(this, ("layer" + std::to_string(lIndex)).c_str(), 28, mOffsetFromTopSpacing + midCentering + lIndex * layerPosSpacing, 12, &seqLayers[lIndex].layerName);
   mLayerEnableCheckbox[lIndex] = new Checkbox(this, ("checkbox" + std::to_string(lIndex)).c_str(), mStartCanvasOffset - 8, mOffsetFromTopSpacing + midCentering + lIndex * layerPosSpacing, &seqLayers[lIndex].enabled);
   mLayerEnableCheckbox[lIndex]->SetDisplayText(false);
   mLayerSettingsButton[lIndex] = new ClickButton(this, ("setting" + std::to_string(lIndex)).c_str(), mStartCanvasOffset - 28, mOffsetFromTopSpacing + midCentering + lIndex * layerPosSpacing, ButtonDisplayStyle::kHamburger);
   //mCanvas->SetRowColor(i,ofColor::clear)
   MoveLayerTo(lIndex, index);
}
void SongCanvas::DeleteLayer(int index)
{
   int lIdx = seqLayers.size() - 1;
   MoveLayerTo(index, lIdx);

   mLayerNameTextbox[lIdx]->Delete();
   mLayerEnableCheckbox[lIdx]->Delete();
   mLayerSettingsButton[lIdx]->Delete();
   seqLayers.pop_back();

   mCanvas->SetNumRows(seqLayers.size());
   mCanvas->SetNumVisibleRows(seqLayers.size());
}

void SongCanvas::MoveLayerTo(int oldIndex, int newIndex)
{
   int idx = oldIndex;
   while (idx != newIndex)
   {
      if (idx > newIndex)
      {
         auto oL = GetAllCanvasElementsOfLayer(idx - 1);
         auto cL = GetAllCanvasElementsOfLayer(idx);

         for (int i = 0; i < oL.size(); ++i)
         {
            oL[i]->mRow = idx;
         }

         for (int i = 0; i < cL.size(); ++i)
         {
            cL[i]->mRow = idx - 1;
         }

         auto oA = seqLayers[idx - 1];
         auto cA = seqLayers[idx];

         seqLayers[idx - 1].layerName = cA.layerName;
         seqLayers[idx].layerName = oA.layerName;
         idx--;
      }
      else
      {
         auto oL = GetAllCanvasElementsOfLayer(idx + 1);
         auto cL = GetAllCanvasElementsOfLayer(idx);

         for (int i = 0; i < oL.size(); ++i)
         {
            oL[i]->mRow = idx;
         }

         for (int i = 0; i < cL.size(); ++i)
         {
            cL[i]->mRow = idx + 1;
         }

         auto oA = seqLayers[idx + 1];
         auto cA = seqLayers[idx];

         seqLayers[idx + 1] = cA;
         seqLayers[idx] = oA;
         idx++;
      }
   }
}

void SongCanvas::GetModuleDimensions(float& width, float& height)
{
   width = LayersListHSize + mCanvas->GetWidth();
   height = mCanvas->GetHeight() + mOffsetFromTopSpacing + 32 + mFlowGridRows * FlowGridRowHeightSize;
}
bool SongCanvas::IsCanvasElementActive(SongCanvas_CanvasElement* element) const
{
   for (int i = 0; i < mActiveElements.size(); ++i)
   {
      if (mActiveElements[i] == element)
         return true;
   }
   return false;
}

void SongCanvas::CanvasUpdated(Canvas* canvas)
{
   if (mCanvas == canvas)
   {
      auto elms = mCanvas->GetElements();

      //How many chunks should we have?
      int colNum = mCanvas->GetNumCols();

      mChunkAmount = CLAMP(colNum, 50, 1000);

      //Regenerate the canvas list.
      for (int i = 0; i < mChunkAmount; ++i)
      {
         mCanvasChunkList[i].clear();
      }

      for (int i = 0; i < elms.size(); i++)
      {

         int cStart = floor(elms[i]->GetStart() * mChunkAmount);
         int cEnd = ceil(elms[i]->GetEnd() * mChunkAmount);

         if (cStart < 0)
            cStart = 0;
         if (cEnd >= mChunkAmount)
         {
            cEnd = mChunkAmount - 1;
         }

         for (int y = cStart; y <= cEnd; ++y)
         {
            mCanvasChunkList[y].push_back(dynamic_cast<SongCanvas_CanvasElement*>(elms[i]));
         }

         //Workspace view resize check
         //Disabled, to enforce a more controlled format.
         /*
         if (elms[i]->GetEnd() > 0.8)
         {
            ResizeWorkspace(1.0 - elms[i]->GetEnd());
            break;
         }*/
      }
      //TheSynth->LogEvent("SongCanvas Regenerated",LogEventType::kLogEventType_Verbose);
   }
}

void SongCanvas::ResizeWorkspace(float diff)
{
   //Todo, allow downsizing.
   /*
   auto elms = mCanvas->GetElements();
   std::vector<float> pos(2 * elms.size());

   for (int i = 0; i < elms.size(); i++)
   {
      pos[i*2] = elms[i]->GetStart();
      pos[i*2+1] = elms[i]->GetEnd();
   }
   */
   mCanvas->SetLength(mCanvas->GetLength() + 1);
   /*
   for (int i = 0; i < elms.size(); i++)
   {
      elms[i]->SetStart(pos[i*2]/2,false);
      elms[i]->SetEnd(pos[i*2+1]/2);
   }*/

   mCanvas->SetNumCols(ceil(mCanvas->GetWidth() / (float)mStandardMeasureSize * mCanvas->GetLength()) * 4);
   //TheSynth->LogEvent(std::to_string(mCanvas->GetNumCols()),kLogEventType_Verbose);
}

ofColor SongCanvas::GetRowColor(int row) const
{
   return mRowColors[row % mRowColors.size()];
}

void SongCanvas::TextEntryComplete(TextEntry* entry)
{
   if (entry == mRackRenameTextBox)
   {
      mRightClickDropdownElementContext->SetName(mRackRenameTextBox->GetText());
      mRightClickDropdownElementContext->SetRenameState(false);
      mRackRenameTextBox->SetPosition(-500, -500);
      mRackRenameTextBox->CheckHover(-500, -500);
      //TheSynth->LogEvent("StringBeatEvent",kLogEventType_Verbose);
   }
}

void SongCanvas::FloatSliderUpdated(FloatSlider* slider, float oldVal, double time)
{
   if (slider == mTransportSlider)
   {
      TheTransport->SetMeasureTime(mTime);
   }
}
void SongCanvas::ButtonClicked(ClickButton* button, double time)
{
   if (button == mPlayPauseButton)
   {
      TheSynth->SetAudioPaused(!TheSynth->IsAudioPaused());
      return;
   }
   else if (button == mResetButton)
   {
      TheTransport->SetMeasureTime(0);
      return;
   }
   else if (button == mRackAddNewButton)
   {
      auto rp = mRackAddNewButton->GetPosition(true);
      mRackAddNewDropdown->SetPosition(rp.x, rp.y);
      mRackAddNewDropdown->OnClicked(1, 1, false);
      mRackAddNewDropdown->SetPosition(-2000, -2000);
      //DropdownClicked(mRackAddNewDropdown);
      return;
   }
   for (int i = 0; i < seqLayers.size(); ++i)
   {
      if (button == mLayerSettingsButton[i])
      {
         mLayerDropdownOptionButtonIndex = i;
         mListDropdownOptions->Clear();
         if (i > 0)
            mListDropdownOptions->AddLabel("Move up", LayerDropDownOptions::enumLDPMoveUp);
         if (i + 1 < seqLayers.size())
            mListDropdownOptions->AddLabel("Move down", LayerDropDownOptions::enumLDPMoveDown);

         if (seqLayers.size() + 1 < MaxLayers)
            mListDropdownOptions->AddLabel("Add new layer", LayerDropDownOptions::enumLDPAddNewLayerBelow);

         if (seqLayers.size() > 1)
         {
            mListDropdownOptions->AddLabel("---", LayerDropDownOptions::enumLDPNothing); //Spacer, makes it a little harder to butter finger deleting a layer full of content.
            mListDropdownOptions->AddLabel("Delete WIP", LayerDropDownOptions::enumLDPDelete);
         }

         auto rp = mLayerSettingsButton[i]->GetPosition(true);
         mListDropdownOptions->SetPosition(rp.x, rp.y);
         mListDropdownOptions->OnClicked(1, 1, false);
         mListDropdownOptions->SetPosition(-2000, -2000);
         return;
      }
   }
}
bool SongCanvas::MouseMoved(float x, float y)
{
   IDrawableModule::MouseMoved(x, y);
   mModGrid->MouseMoved(x, y);
   return false;
}
void SongCanvas::OnClicked(float x, float y, bool right)
{
   IDrawableModule::OnClicked(x, y, right);
   mModGrid->OnClicked(x - mModGrid->GetPosition().x, y - mModGrid->GetPosition().y, right);
}

void SongCanvas::MouseReleased()
{
   IDrawableModule::MouseReleased();
   mModGrid->MouseReleased();
}
void SongCanvas::DropdownUpdated(DropdownList* list, int oldVal, double time)
{
   if (list == mRackAddNewDropdown)
   {
      std::string newPartName = "Part " + std::to_string(mPartNameCount);
      mPartNameCount++;
      switch (mRackAddNewElementIndex)
      {
         case enumEnabler:
            mModGrid->AddElement(new SongCanvasRackElement(90, SongCanvasElementVariant::Enabler, newPartName, this));
            break;
         case enumPulser:
            mModGrid->AddElement(new SongCanvasRackElement(140, SongCanvasElementVariant::Pulser, newPartName, this));
            break;
         case enumModulator: break;
         case enumSample: break;
         case enumOnePulse:
            mModGrid->AddElement(new SongCanvasRackElement(90, SongCanvasElementVariant::OnePulse, newPartName, this));
            break;
         default:;
      }
      IncrementInternalRackId();
      return;
   }
   if (list == mRackElementRightClickDropdown)
   {
      switch (mRackElementRightClickIndex)
      {
         case enumNothing: break;
         case enumRename:
         {
            if (mRightClickDropdownElementContext != nullptr)
               mRightClickDropdownElementContext->SetRenameState(false);
            mRackRenameTextBox->SetText(*mRightClickDropdownElementContext->GetName());
            mRightClickDropdownElementContext->SetRenameState(true);
         }
         break;
         case enumDelete:
         {
            mRackRenameTextBox->SetPosition(-500, -500);
            mRackRenameTextBox->ClearInput();
            mRackRenameTextBox->ClearActiveKeyboardFocus(false);
            DeleteRackElement(mRightClickDropdownElementContext);
            mCanvas->SetAllowElementPlacement(false);
         }
         break;
      }
      return;
   }
   if (list == mListDropdownOptions)
   {
      int idx = mLayerDropdownOptionButtonIndex;
      if (mLayerDropDownOptions == LayerDropDownOptions::enumLDPMoveUp)
      {
         MoveLayerTo(idx, idx - 1);
      }
      else if (mLayerDropDownOptions == LayerDropDownOptions::enumLDPMoveDown)
      {
         MoveLayerTo(idx, idx + 1);
      }
      else if (mLayerDropDownOptions == LayerDropDownOptions::enumLDPDelete)
      {
         DeleteLayer(idx);
      }
      else if (mLayerDropDownOptions == LayerDropDownOptions::enumLDPAddNewLayerBelow)
      {
         AddNewLayer(idx + 1, SongCanvasLayer{
                              ofColor::white,
                              0,
                              true,
                              "layer" + ofToString(seqLayers.size())});
         //FeatureResize(0,20);//TODO bump up the size based on the current estimated size of a layer
      }
   }
   for (int i = 0; i < mModGrid->GetAllElements().size(); ++i)
   {
      auto e = dynamic_cast<SongCanvasRackElement*>(mModGrid->GetAllElements()[i]);
      switch (e->mVariantType)
      {
         case SongCanvasElementVariant::Pulser:
            if (list == e->GetPulserIntervalDropdown())
            {
               TransportListenerInfo* transportListenerInfo = TheTransport->GetListenerInfo(e);
               if (transportListenerInfo != nullptr)
               {
                  transportListenerInfo->mInterval = e->GetInterval();
                  transportListenerInfo->mOffsetInfo = OffsetInfo(0, false);
               }
            }
            break;
         default:;
      }
   }
}
void SongCanvas::SetNewRackDropdownContext(SongCanvasRackElement* element)
{
   mRightClickDropdownElementContext = element;
}
void SongCanvas::SetSelectedRackElement(SongCanvasRackElement* element)
{
   mSelectedRackElement = element;
   mCanvas->SetAllowElementPlacement(true);
}
void SongCanvas::SetupCanvasElement(SongCanvas_CanvasElement* element)
{
   element->Setup(mSelectedRackElement);
}
void SongCanvas::ElementAdditionSuppressed(float posX, float posY)
{
   mFlashRackStartTime = ofGetGlobalTime();
}
//Incoming event from a note bring removed from the base Canvas
void SongCanvas::ElementRemoved(CanvasElement* element)
{
   SongCanvas_CanvasElement* sElement = dynamic_cast<SongCanvas_CanvasElement*>(element);
   //If any of them are playing, we send notes off.
   for (int i = 0; i < mActiveElements.size(); ++i)
   {
      if (mActiveElements[i] == sElement)
      {
         sElement->GetRackElement()->OnExit();
         mActiveElements.erase(mActiveElements.begin() + i);
      }
   }
   mPartCanvasDirty = true;
   //Then we flag the Canvas as dirty, so we don't stress the DAW too much with pointless regenerations.
}

void SongCanvas::DeleteRackElement(SongCanvasRackElement* element) const
{
   auto res = GetAllCanvasElementsOfRack(element);
   for (auto re : res)
   {
      mCanvas->RemoveElement(re);
   }
   mModGrid->RemoveElement(element);
}
std::vector<SongCanvasRackElement*> SongCanvas::GetAllRackElements() const
{
   std::vector<SongCanvasRackElement*> output;
   auto elms = mModGrid->GetAllElements();
   for (int i = 0; i < elms.size(); ++i)
   {
      auto relm = dynamic_cast<SongCanvasRackElement*>(elms[i]);
      output.push_back(relm);
   }
   return output;
}

//Gets all the canvas elements which have said rack as a parent.
std::vector<SongCanvas_CanvasElement*> SongCanvas::GetAllCanvasElementsOfRack(const SongCanvasRackElement* element) const
{
   auto elms = mCanvas->GetElements();
   std::vector<SongCanvas_CanvasElement*> output;
   for (int i = 0; i < elms.size(); ++i)
   {
      auto celm = dynamic_cast<SongCanvas_CanvasElement*>(elms[i]);
      if (celm->GetRackElement() == element)
         output.push_back(celm);
   }
   return output;
}
std::vector<SongCanvas_CanvasElement*> SongCanvas::GetAllCanvasElementsOfLayer(const int layerIndex) const
{
   auto elms = mCanvas->GetElements();
   std::vector<SongCanvas_CanvasElement*> output;
   for (int i = 0; i < elms.size(); ++i)
   {
      if (elms[i]->mRow == layerIndex)
      {
         output.push_back(dynamic_cast<SongCanvas_CanvasElement*>(elms[i]));
      }
   }
   return output;
}


SongCanvasRackElement* SongCanvas::GetRackElementWithID(int id)
{
   auto RE = GetAllRackElements();
   for (int i = 0; i < RE.size(); ++i)
   {
      if (RE[i]->mInternalRackID == id)
      {
         return RE[i];
      }
   }
   return nullptr;
}

void SongCanvas::OnTransportAdvanced(float amount)
{
   //RESERVED, might get used later, depending on future module support.
}


void SongCanvas::ReceiveSignal(SignalId signalID)
{
   if (signalID == SignalId::ResizeRequest)
   {
      mFlowGridRows = mModGrid->GetRowCount();

      float bWSize;
      float bHSize;
      mRackAddNewButton->GetDimensions(bWSize, bHSize);

      mRackAddNewButton->SetDimensions(bWSize, mFlowGridRows * FlowGridRowHeightSize);
   }
}
void SongCanvas::DisposeElement(IClickable* element)
{
   RemoveUIControl(static_cast<IUIControl*>(element));
}

//Called on a 64n interval, which is very fast.
void SongCanvas::OnTimeEvent(double time)
{
   //First check if we have any cleanup to do.
   if (mPartCanvasDirty)
   {
      CanvasUpdated(mCanvas);
      mPartCanvasDirty = false;
   }

   //The 0.02f refers to a small nudge to help it activate modules at points where they can activate notes at the exact same time more reliably.
   mCanvasRelativeTime = (mTime + 0.02f) / ((double)mCanvas->GetNumCols() / 4);
   if (IsEnabled())
   {
      if (mCanvasRelativeTime <= 1)
      {
         int readChunkNum = floor(mCanvasRelativeTime * mChunkAmount);
         if (readChunkNum < 0)
            readChunkNum = 0;
         auto& cL = mCanvasChunkList[readChunkNum];

         //Process the chunk, activating/processing as needed.
         for (int i = 0; i < cL.size(); ++i)
         {
            if (cL[i]->GetStart() < mCanvasRelativeTime && cL[i]->GetEnd() > mCanvasRelativeTime)
            {
               if (!IsCanvasElementActive(cL[i]) & seqLayers[cL[i]->mRow].enabled)
               {
                  mActiveElements.push_back(cL[i]);
                  cL[i]->GetRackElement()->OnEnter();
               }
               if (seqLayers[cL[i]->mRow].enabled)
                  cL[i]->GetRackElement()->OnProcess();
               else
               {
                  cL[i]->GetRackElement()->OnExit();
                  for (int mAE = 0; mAE < mActiveElements.size(); ++mAE)
                  {
                     if (mActiveElements[mAE] == cL[i])
                     {
                        mActiveElements.erase(mActiveElements.begin() + mAE);
                     }
                  }
               }
            }
         }

         //Now cull the unused ones.
         for (int i = 0; i < mActiveElements.size(); ++i)
         {
            if (mActiveElements[i]->GetStart() > mCanvasRelativeTime || mActiveElements[i]->GetEnd() < mCanvasRelativeTime)
            {
               mActiveElements[i]->GetRackElement()->OnExit();
               mActiveElements.erase(mActiveElements.begin() + i);
               i--;
            }
         }
      }
   }
   else
   {
      //If we're disabled, clean up.
      for (int i = 0; i < mActiveElements.size(); ++i)
      {
         mActiveElements[i]->GetRackElement()->OnExit();
         mActiveElements.erase(mActiveElements.begin() + i);
         i--;
      }
   }

   /*
   for (int i = 0; i < mActiveElements.size(); ++i)
   {
      auto elm = mActiveElements[i];

      if (elm->GetVariantType() == SongCanvasElementVariant::Pulser)
      {
         auto rck = elm->GetRackElement();

         auto interv = rck->GetInterval();


      }

   }*/
}

//Attempt to resize based on the addition/removal of a feature.
void SongCanvas::FeatureResize(int extraW, int extraH)
{
   float h = mCanvas->GetHeight();
   float w = mCanvas->GetWidth();
   Resize(w + extraW, h + extraH);
}

void SongCanvas::Resize(float w, float h)
{
   w = MAX(w - GetCanvasStartXOffset(), 350);
   h = MAX(h - mOffsetFromTopSpacing, 100 + seqLayers.size() * MinRowSize);

   int multiple = std::ceil((w - LayersListHSize) / static_cast<float>(mStandardMeasureSize));

   w = LayersListHSize + multiple * mStandardMeasureSize - 6;

   mCanvas->SetDimensions(w, h);
   mCanvas->SetNumCols(ceil(mCanvas->GetWidth() / static_cast<float>(mStandardMeasureSize) * mCanvas->GetLength()) * 4);

   float bWSize;
   float bHSize;
   mRackAddNewButton->GetDimensions(bWSize, bHSize);
   mModGrid->SetDimensions(mCanvas->GetWidth() - 16 + GetCanvasStartXOffset() - bWSize * 1.5f, mFlowGridRows * FlowGridRowHeightSize);
   mModGrid->RecalculateElements();
   mRackAddNewButton->SetDimensions(bWSize, mFlowGridRows * FlowGridRowHeightSize);
}

void SongCanvas::LoadLayout(const ofxJSONElement& moduleInfo)
{
}

void SongCanvas::SetUpFromSaveData()
{
}
void SongCanvas::SaveLayout(ofxJSONElement& moduleInfo)
{
}
void SongCanvas::SaveState(FileStreamOut& out)
{
   out << GetModuleSaveStateRev(); //Updating to newer versions? Check this handy variable!
   //SEC-1 format 05/03/2026


   out << (int)seqLayers.size();

   //Step 1, save our canvas state
   for (int i = 0; i < seqLayers.size(); ++i)
   {
      out << seqLayers[i].enabled;
      out << seqLayers[i].layerName;
   }
   //Step 2, now the racks.
   auto racks = GetAllRackElements();
   out << mInternalRackIDCounter;
   out << (int)racks.size();
   for (int i = 0; i < racks.size(); ++i)
   {
      auto rack = racks[i];
      int variantType = static_cast<int>(rack->mVariantType);

      out << rack->GetName()->substr(); //Note to self: Saving a pointer is bad juju. ALSO, it HAS to be a substr, dunno why.
      out << variantType;
      out << rack->GetPreferredWidth();
      out << rack->mInternalRackID;
      switch (rack->mVariantType)
      {
         case SongCanvasElementVariant::Enabler:
            rack->GetEnablerCable()->SaveState(out);
            break;
         case SongCanvasElementVariant::Pulser:
            out << rack->GetInterval();
            rack->GetPulserCable()->SaveState(out);

            //out << rack->GetInterval();
            break;
         case SongCanvasElementVariant::LFO:
            break;
         case SongCanvasElementVariant::Sampler:
            break;
         case SongCanvasElementVariant::OnePulse:
            rack->GetPulserCable()->SaveState(out);
            break;
      }
   }
   //Step 3: Let's save the canvas states now.
   out << mCanvas->GetNumRows();
   out << mCanvas->GetNumCols();

   out << mCanvas->GetLength();
   out << mCanvas->GetWidth();
   out << mCanvas->GetHeight();

   out << mFlowGridRows;

   auto cvs = mCanvas->GetElements();
   out << (int)cvs.size();
   for (int i = 0; i < cvs.size(); ++i)
   {
      auto ce = static_cast<SongCanvas_CanvasElement*>(cvs[i]);
      out << ce->mRow;
      out << ce->mCol;
      out << ce->mLength;
      out << ce->GetStart();
      out << ce->GetEnd();

      out << ce->GetRackElementId();
   }
   out << mCanvas->mViewStart;
   out << mCanvas->mViewEnd;

   //More misc stuff
   out << mPartNameCount;
   out << IsEnabled();

   //Reserved variables SEC-1
   out << mGlobalMode;
   out << mStartMeasure;
   out << mEndMeasure;
   out << mAutoEndMeasure;
   out << mLoopOnEnd;
   out << mRedLoopEnd;
   out << mRedLoopStart;
   //End reserved variables SEC-1
}
void SongCanvas::LoadState(FileStreamIn& in, int rev)
{
   int canvasLayerCount;

   //Step 1, restore our canvas
   in >> canvasLayerCount;

   //Delete the starter layers first.
   for (int i = 0; i < 5; ++i)
   {
      DeleteLayer(0);
   }
   for (int i = 0; i < canvasLayerCount; ++i)
   {
      bool lD1;
      std::string lD2;
      in >> lD1;
      in >> lD2;

      AddNewLayer(i, SongCanvasLayer{ ofColor::white, 0, lD1, lD2 });
   }

   auto re = GetAllRackElements();

   //Step 2, clean up our starter rack.
   for (int i = 0; i < re.size(); ++i)
   {
      DeleteRackElement(re[i]);
   }

   //Step 3, now load our real rack.
   int numRackElements;
   in >> mInternalRackIDCounter;
   in >> numRackElements;
   for (int i = 0; i < numRackElements; ++i)
   {
      std::string eName = "Loaded";
      int eVariantType;
      float ePrefSize;
      int eRackId;
      in >> eName;
      in >> eVariantType;
      in >> ePrefSize;
      in >> eRackId;

      auto nrm = new SongCanvasRackElement(
      ePrefSize,
      static_cast<SongCanvasElementVariant>(eVariantType),
      eName,
      this);
      mModGrid->AddElement(nrm);
      nrm->mInternalRackID = eRackId;

      switch ((SongCanvasElementVariant)eVariantType)
      {
         case SongCanvasElementVariant::Enabler:
            nrm->GetEnablerCable()->LoadState(in);
            break;
         case SongCanvasElementVariant::Pulser:
            int n;
            in >> n;
            nrm->SetInterval(static_cast<NoteInterval>(n));
            nrm->GetPulserCable()->LoadState(in);
            break;
         case SongCanvasElementVariant::LFO:
            break;
         case SongCanvasElementVariant::Sampler:
            break;
         case SongCanvasElementVariant::OnePulse:
            nrm->GetPulserCable()->LoadState(in);
            break;
      }
   }
   //Step 3: time to set up our Canvas and scaling now

   int i1;
   float f1;
   float f2;
   in >> i1;
   mCanvas->SetNumRows(i1);
   in >> i1;
   mCanvas->SetNumCols(i1);
   in >> f1;
   mCanvas->SetLength(f1);
   in >> f1;
   in >> f2;

   mCanvas->SetDimensions(f1, f2);
   in >> mFlowGridRows;

   float bWSize;
   float bHSize;
   mRackAddNewButton->GetDimensions(bWSize, bHSize);
   mModGrid->SetDimensions(mCanvas->GetWidth() - 16 + GetCanvasStartXOffset() - bWSize * 1.5f, mFlowGridRows * FlowGridRowHeightSize);
   mModGrid->RecalculateElements();
   auto mgp = mModGrid->GetPosition(true);
   mRackAddNewButton->SetDimensions(bWSize, mFlowGridRows * FlowGridRowHeightSize);
   mRackAddNewButton->SetPosition(mgp.x + mModGrid->GetWidth(), mgp.y);
   //IDrawableModule::LoadState(in, rev); <-- Meanie :(

   std::vector<SongCanvas_CanvasElement*> celms;
   in >> i1;
   for (int i = 0; i < i1; ++i)
   {
      int row;
      int col;
      float length;
      float eStart;
      float eEnd;
      int rackId;

      in >> row;
      in >> col;
      in >> length;
      in >> eStart;
      in >> eEnd;
      in >> rackId;
      mSelectedRackElement = GetRackElementWithID(rackId);
      SongCanvas_CanvasElement* celm = new SongCanvas_CanvasElement(mCanvas, col, row, 0, length);
      celm->SetStart(eStart, true);
      celm->SetEnd(eEnd);
      mCanvas->AddElement(celm);
   }
   in >> mCanvas->mViewStart;
   in >> mCanvas->mViewEnd;

   CanvasUpdated(mCanvas);
   //More misc stuff.
   in >> mPartNameCount;
   bool enableState;
   in >> enableState;
   //Reserved variables SEC-1
   in >> mGlobalMode;
   in >> mStartMeasure;
   in >> mEndMeasure;
   in >> mAutoEndMeasure;
   in >> mLoopOnEnd;
   in >> mRedLoopEnd;
   in >> mRedLoopStart;
   //End reserved variables SEC-1
   SetEnabled(enableState);
}

///////////////////////////////
///SongCanvasRackElements///
///////////////////////////////


std::string TruncateString(std::string str, size_t width, bool show_ellipsis = true)
{
   if (str.length() > width)
   {
      if (show_ellipsis)
      {
         str.resize(width);
         return str.append("...");
      }
      else
      {
         str.resize(width);
         return str;
      }
   }
   return str;
}

std::string RemoveNonNumericalChars(const std::string& input)
{
   std::string result;
   for (char c : input)
   {
      if (c >= '0' && c <= '9')
      {
         result += c;
      }
   }
   return result;
}

SongCanvasRackElement::SongCanvasRackElement(float preferredWidth, SongCanvasElementVariant variantType, std::string name, SongCanvas* owner, const ofColor& overrideColor)
: UIFlowGridElement(preferredWidth, overrideColor)
{
   mElementName = new std::string(name);
   mSSParent = owner;
   mVariantType = variantType;
   mInternalRackID = mSSParent->GetInternalRackId();
   switch (variantType)
   {
      case SongCanvasElementVariant::Enabler:
         SetColor(ofColor::white);
         break;
      case SongCanvasElementVariant::Pulser:
         SetColor(ofColor::yellow);
         mVariantExtraWidth = 50;
         mTransportListenerInfo = TheTransport->AddListener(this, mPulserInterval, OffsetInfo(0, true), true);
         break;
      case SongCanvasElementVariant::LFO:

         break;
      case SongCanvasElementVariant::Sampler:

         break;
      case SongCanvasElementVariant::OnePulse:
         SetColor(ofColor(150, 150, 0));
         break;
   }
   CreateUIControls(owner);
   mHighlightOutlineColor = ofColor(0, 150, 255);
}

SongCanvasRackElement::~SongCanvasRackElement()
{
   switch (mVariantType)
   {
      case SongCanvasElementVariant::Enabler:
         mSSParent->RemovePatchCableSource(mEnablerCable);
         break;
      case SongCanvasElementVariant::Pulser:
         mSSParent->RemovePatchCableSource(mPulserCable);
         TheTransport->RemoveListener(this);
         mSSParent->DisposeElement(mIntervalSelector);
         mIntervalSelector->Delete();
         break;
      case SongCanvasElementVariant::LFO:
         break;
      case SongCanvasElementVariant::Sampler: break;
      case SongCanvasElementVariant::OnePulse:
         mSSParent->RemovePatchCableSource(mPulserCable);
         break;
      default:;
   }
}

void SongCanvasRackElement::CreateUIControls(SongCanvas* owner)
{
   switch (mVariantType)
   {
      case SongCanvasElementVariant::Enabler:
         mEnablerCable = new PatchCableSource(owner, kConnectionType_UIControl);
         owner->AddPatchCableSource(mEnablerCable);
         mEnablerCable->SetAllowMultipleTargets(true);
         break;
      case SongCanvasElementVariant::Pulser:
         mPulserCable = new PatchCableSource(owner, kConnectionType_Pulse);
         owner->AddPatchCableSource(mPulserCable);
         mPulserCable->SetAllowMultipleTargets(true);

         mIntervalSelector = new DropdownList(owner, "interval", 75, 2, (int*)(&mPulserInterval));

         mIntervalSelector->AddLabel("16", kInterval_16);
         mIntervalSelector->AddLabel("8", kInterval_8);
         mIntervalSelector->AddLabel("4", kInterval_4);
         mIntervalSelector->AddLabel("3", kInterval_3);
         mIntervalSelector->AddLabel("2", kInterval_2);
         mIntervalSelector->AddLabel("1n", kInterval_1n);
         mIntervalSelector->AddLabel("2n", kInterval_2n);
         mIntervalSelector->AddLabel("4n", kInterval_4n);
         mIntervalSelector->AddLabel("4nt", kInterval_4nt);
         mIntervalSelector->AddLabel("8n", kInterval_8n);
         mIntervalSelector->AddLabel("8nt", kInterval_8nt);
         mIntervalSelector->AddLabel("16n", kInterval_16n);
         mIntervalSelector->AddLabel("16nt", kInterval_16nt);
         mIntervalSelector->AddLabel("32n", kInterval_32n);
         mIntervalSelector->AddLabel("64n", kInterval_64n);
         mIntervalSelector->AddLabel("none", kInterval_None);
         mIntervalSelector->AddLabel("div", kInterval_CustomDivisor);
         break;
      case SongCanvasElementVariant::LFO: break;
      case SongCanvasElementVariant::Sampler: break;
      case SongCanvasElementVariant::OnePulse:
         mPulserCable = new PatchCableSource(owner, kConnectionType_Pulse);
         owner->AddPatchCableSource(mPulserCable);
         mPulserCable->SetAllowMultipleTargets(true);
         break;
      default:;
   }

   mElementRenameTextBox = owner->GetRackRenameTextbox();
   //mElementRenameTextBox->
   //mElementRenameTextBox = new TextEntry(owner, "", 32, 32, 8, &mElementName);
   //mTransportTextBox = new TextEntry{ this, "", startCanvasOffset - 68, 22, 6, &mTime, 0, 99999 };
}

void SongCanvasRackElement::Draw()
{
   UIFlowGridElement::Draw();

   ofPushStyle();
   ofSetColor(ofColor::white);
   if (mExciteConstant > 0) //Make the outline bounce for extra visual satisfaction.
   {
      float excConst = mExciteConstant + sin(ofGetGlobalTime() * 12) * 0.2F;
      if (mExcitePower < excConst)
         mExcitePower = excConst;
   }
   mExcitePower = MAX(0, mExcitePower - ofGetLastFrameTime() * 2);
   mExciteDrag = ofLerp(mExciteDrag, mExcitePower, ofGetLastFrameTime() * 12);
   mOutlineThickness = 0.8F + mExciteDrag * 1.2;

   auto rPos = GetRelativePosition();
   auto pos = GetFlowGrid()->GetPosition(true);
   float compressRefit = 12; //Offset so it's not at the element edge.
   if (mWidth < 90)
   { //Compensate for high compression
      compressRefit *= static_cast<float>(mWidth) / 90;
   }
   switch (mVariantType)
   {
      case SongCanvasElementVariant::Enabler:
         mEnablerCable->SetManualPosition(rPos.x + mWidth - compressRefit, rPos.y + mHeight / 2);
         break;
      case SongCanvasElementVariant::Pulser:
         mPulserCable->SetManualPosition(rPos.x + mWidth - compressRefit, rPos.y + mHeight / 2);
         ofPushMatrix();
         ofTranslate(-pos.x, -pos.y);
         mIntervalSelector->SetPosition(rPos.x + mWidth - compressRefit - 53, rPos.y + 7);
         mIntervalSelector->Draw();
         ofPopMatrix();

         break;
      case SongCanvasElementVariant::LFO:
         break;
      case SongCanvasElementVariant::Sampler:
         break;
      case SongCanvasElementVariant::OnePulse:
         mPulserCable->SetManualPosition(rPos.x + mWidth - compressRefit, rPos.y + mHeight / 2);
         break;
   }

   if (mRenameActive)
   {

      //mElementRenameTextBox->SetPosition(rPos.x, rPos.y + mHeight / 2);
      ofPushMatrix();
      ofTranslate(-pos.x, -pos.y);
      mElementRenameTextBox->SetPosition(rPos.x + 4, rPos.y + 7);
      mElementRenameTextBox->Draw();

      ofPopMatrix();

      int form = (static_cast<std::string>(mElementRenameTextBox->GetText()).size() - 10) * 6;
      if (form < 0)
         form = 0;
      SetPreferredSize(90 + form + mVariantExtraWidth);

      GetFlowGrid()->RecalculateElements();
   }
   else
   {
      int textSize = (mWidth - 15) / 7.0;
      std::string displayString;
      if (textSize <= 3)
      {
         displayString = RemoveNonNumericalChars(mElementName->c_str());
         if (displayString.empty())
            displayString = TruncateString(mElementName->c_str(), textSize, true);
      }
      else
      {
         displayString = TruncateString(mElementName->c_str(), textSize, true);
      }

      if (mWidth > 20)
         DrawTextNormal(displayString, mX + 8, mY + mHeight / 2 + 5.5);
   }

   ofPopStyle();
}

void SongCanvasRackElement::OnMouseClick(bool rightClick)
{
   if (rightClick)
   {
      auto p = GetRelativePosition();
      mSSParent->SetNewRackDropdownContext(this);
      mSSParent->GetRackRightClickDropdown()->SetPosition(p.x, p.y);
      mSSParent->GetRackRightClickDropdown()->OnClicked(1, 1, false);
      mSSParent->GetRackRightClickDropdown()->SetPosition(-500, -500);
   }
   else
   {
      mSSParent->SetSelectedRackElement(this);
   } //Todo, double click to rename
   //mElementName = "Clicked!";
}
void SongCanvasRackElement::SetName(std::string newName) const
{
   *mElementName = newName;
}

void SongCanvasRackElement::OnEnter()
{
   mActive = true;
   double time = NextBufferTime(mSSParent);
   switch (mVariantType)
   {
      case SongCanvasElementVariant::Enabler:

         mEnablerCable->AddHistoryEvent(time, true, 0);
         Excite(1);
         SetExciteConstant(0.6);
         for (auto* cable : mEnablerCable->GetPatchCables())
         {
            IUIControl* uicontrol = dynamic_cast<IUIControl*>(cable->GetTarget());
            if (uicontrol)
            {
               uicontrol->SetValue(1, time);
            }
         }

         break;
      case SongCanvasElementVariant::Pulser:

         break;


      case SongCanvasElementVariant::LFO: break;
      case SongCanvasElementVariant::Sampler: break;
      case SongCanvasElementVariant::OnePulse:
         const std::vector<IPulseReceiver*>& receivers = mPulserCable->GetPulseReceivers();
         mPulserCable->AddHistoryEvent(time, true, 0);
         mPulserCable->AddHistoryEvent(time + 15, false);
         Excite(1);
         for (auto* receiver : receivers)
            receiver->OnPulse(time, 1, 0);
         break;
   }
}
void SongCanvasRackElement::OnProcess()
{
   switch (mVariantType)
   {
      case SongCanvasElementVariant::Enabler:
         break;
      case SongCanvasElementVariant::Pulser:
         break;
      case SongCanvasElementVariant::LFO:
         break;
      case SongCanvasElementVariant::Sampler:
         break;
      case SongCanvasElementVariant::OnePulse:
         break;
      default:;
   }
}
void SongCanvasRackElement::OnExit()
{
   mActive = false;
   switch (mVariantType)
   {
      case SongCanvasElementVariant::Enabler:
         mEnablerCable->AddHistoryEvent(NextBufferTime(mSSParent), false, 0);
         SetExciteConstant(0);

         for (auto* cable : mEnablerCable->GetPatchCables())
         {
            IUIControl* uicontrol = dynamic_cast<IUIControl*>(cable->GetTarget());
            if (uicontrol)
            {
               uicontrol->SetValue(0, NextBufferTime(mSSParent));
            }
         }
         break;
      case SongCanvasElementVariant::Pulser: break;
      case SongCanvasElementVariant::LFO: break;
      case SongCanvasElementVariant::Sampler: break;
      case SongCanvasElementVariant::OnePulse: break;
      default:;
   }
}

void SongCanvasRackElement::OnTimeEvent(double time)
{
   if (IsActive() && mSSParent->IsEnabled())
   {
      const std::vector<IPulseReceiver*>& receivers = mPulserCable->GetPulseReceivers();
      mPulserCable->AddHistoryEvent(time, true, 0);
      mPulserCable->AddHistoryEvent(time + 15, false);
      Excite(1);
      for (auto* receiver : receivers)
         receiver->OnPulse(time, 1, 0);
   }
}

/*
void SongSequencerRackElement::PostRepatch(PatchCableSource* cableSource, bool fromUserClick)
{
   for (int i = 0; i < mControlCables.size(); ++i)
   {
      if (mControlCables[i] == cableSource)
      {
         if (i == mControlCables.size() - 1)
         {
            if (cableSource->GetTarget())
            {
               PatchCableSource* cable = new PatchCableSource(this, kConnectionType_Modulator);
               AddPatchCableSource(cable);
               mControlCables.push_back(cable);
            }
         }
         else if (cableSource->GetTarget() == nullptr && fromUserClick)
         {
            RemoveFromVector(cableSource, mControlCables);
         }

         break;
      }
   }
}
*/