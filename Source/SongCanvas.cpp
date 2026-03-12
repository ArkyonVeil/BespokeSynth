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

#include <cstring>


std::array<ofColor, 2> SongCanvas::ESCarbonColours{
   ofColor(255, 255, 255),
   ofColor(0, 0, 0),
};
std::array<ofColor, 3> SongCanvas::ESRGBColours{
   ofColor(255, 0, 0),
   ofColor(0, 255, 0),
   ofColor(0, 255, 255)
};
std::array<ofColor, 6> SongCanvas::ESPrideColours{
   ofColor(255, 0, 0),
   ofColor(255, 165, 0),
   ofColor(255, 255, 0),
   ofColor(0, 255, 0),
   ofColor(0, 0, 255),
   ofColor(148, 0, 211)
};
std::array<ofColor, 4> SongCanvas::ESTransColours{
   ofColor(91, 207, 250),
   ofColor(245, 171, 182),
   ofColor(255, 255, 255),
   ofColor(245, 171, 182)
};

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

void SongCanvas::Init()
{


   IDrawableModule::Init();
   mTransportListenerInfo = TheTransport->AddListener(this, kInterval_64n, OffsetInfo(0, true), true);
   TheTransport->AddAudioPoller(this);
}

void SongCanvas::CreateUIControls()
{
   IDrawableModule::CreateUIControls();


   if (expertPanelEnabled)
      mStartCanvasXOffset = LayersListWidthSize + AdvancedConfigHSize;
   else
      mStartCanvasXOffset = LayersListWidthSize;

   //Default size.
   mWidth = 720;
   mHeight = 300;
   //Default measure count.
   mMeasureCount = 12;
   int cSize = mStandardMeasureSize * mDefaultMeasureSpawnAmount;
   mCanvas = new Canvas(this, mStartCanvasXOffset, mOffsetFromTopSpacing, cSize, layerBuffer.size() * StandardRowSize, mMeasureCount, 5, mMeasureCount * 4, &SongCanvas_CanvasElement::Create);
   AddUIControl(mCanvas);
   mCanvas->SetListener(this);

   mCanvas->SetDragMode(Canvas::kDragBoth);
   mCanvas->SetNumVisibleRows(5);
   mCanvas->SetMajorColumnInterval(4);
   mCanvas->SetScrollZoomSpeed(5);
   mCanvas->SetInvertDragSnapBehavior(true);
   mCanvas->SetAllowElementPlacement(false);
   mCanvas->mViewEnd = mMeasureCount;
   mCanvas->mLoopEnd = mMeasureCount;

   mCanvasTimeline = new CanvasTimeline(mCanvas, "loop region");
   mCanvasTimeline->SetCanvasYOffset(-22);
   mCanvasTimeline->SetBaseColour(ofColor(25, 25, 25));
   mCanvasTimeline->SetListener(this);
   //mCanvasTimeline->SetHighlightColour(ofColor(200, 0, 0));
   //mCanvasTimeline->SetCornerHighlightColour(ofColor(200, 0, 0));
   AddUIControl(mCanvasTimeline);

   mMainScrollbarHorizontal = new CanvasScrollbar(mCanvas, "scrollh", CanvasScrollbar::Style::kHorizontal);
   AddUIControl(mMainScrollbarHorizontal);

   mMeasureSlider = new FloatSlider(this, "measurebar", mStartCanvasXOffset, mOffsetFromTopSpacing - 16, mCanvas->GetWidth(), 15, &mTime, 0, 32);
   mMeasureSlider->SetNoHover(true);
   mMeasureSlider->SetCableTargetable(false);
   mMeasureSlider->SetTextAlpha(0);

   mRackGrid = new UIFlowGrid("playrack", 8, GetRackGridStartYOffset(), mCanvas->GetWidth() - 16 + GetCanvasStartXOffset(), 32, 2, this, this);

   mRackRenameTextBox = new TextEntry{ this, "rename", -500, -500, 7, &mRackRenameString };
   mRackRenameTextBox->SetRequireEnter(true);
   mRackRenameTextBox->SetFlexibleWidth(true);


   auto mgp = mRackGrid->GetPosition(true);

   mRackAddNewButton = new ClickButton(this, "add", mgp.x + mRackGrid->GetWidth(), mgp.y, ButtonDisplayStyle::kPlus);
   float bWidth;
   float bHeight;
   mRackAddNewButton->GetDimensions(bWidth, bHeight);
   mRackAddNewButton->SetPosition(mgp.x + mRackGrid->GetWidth() - bWidth * 1.5, mgp.y);
   mRackAddNewButton->SetDimensions(bWidth * 1.5, mRackGrid->GetHeight());
   mRackAddNewButton->SetIconAlignment(ButtonIconAlignment::kCenter);

   mRackGrid->SetDimensions(mCanvas->GetWidth() - 16 + GetCanvasStartXOffset() - bWidth * 1.5f, mFlowGridRows * FlowGridRowHeightSize);

   //HACK, but this is just to avoid having to do further changes to the dropdown element.
   //TL DR: Don't draw it, but send over the events with the button.
   mRackAddNewDropdown = new DropdownList(this, "", mgp.x + mRackGrid->GetWidth(), mgp.y, (int*)&mRackAddNewElementIndex);
   mRackAddNewDropdown->AddLabel("Enabler", RackAddNewElementOptions::enumEnabler);
   mRackAddNewDropdown->AddLabel("Pulser", RackAddNewElementOptions::enumPulser);
   mRackAddNewDropdown->AddLabel("OnePulse", RackAddNewElementOptions::enumOnePulse);

   mRackElementRightClickDropdown = new DropdownList(this, "", -100, -100, (int*)&mRackElementRightClickIndex);
   mRackElementRightClickDropdown->AddLabel("Delete", RackElementRightClickOptions::enumDelete);
   mRackElementRightClickDropdown->AddLabel("Rename", RackElementRightClickOptions::enumRename);

   mListDropdownOptions = new DropdownList(this, "", -100, -100, (int*)&mLayerDropDownOptions);

   int headerYOffset = 8;
   int mHeaderOffset = 4;
   mPlayPauseButton = new ClickButton{ this, "play/pause", mHeaderOffset, headerYOffset, ButtonDisplayStyle::kPlay };
   mHeaderOffset += mPlayPauseButton->GetRect(true).width + 2;
   mResetButton = new ClickButton{ this, "reset", mHeaderOffset, headerYOffset, ButtonDisplayStyle::kText };
   mHeaderOffset += mResetButton->GetRect(true).width + 2;
   mTransportSlider = new FloatSlider(this, "transport", mHeaderOffset, headerYOffset, 54, 15, &mTime, 0, 12);
   mTransportSlider->SetShowName(false);
   mHeaderOffset += mTransportSlider->GetRect(true).width;

   mHeaderOffset += 16;

   // int measureYOffset = mOffsetFromTopSpacing-16;
   mMeasureBaseTextbox = new TextEntry(this, "measure", mHeaderOffset, headerYOffset, 3, &mMeasureStart, 0, 999999);
   mMeasureBaseTextbox->DrawLabel(true);
   mMeasureBaseTextbox->SetRequireEnter(false);

   mHeaderOffset += mMeasureBaseTextbox->GetRect(true).width + 6;
   mMeasureCountTextbox = new TextEntry(this, "count", mHeaderOffset, headerYOffset, 3, &mMeasureCount, 0, 999999);
   mMeasureCountTextbox->DrawLabel(true);
   mMeasureCountTextbox->SetRequireEnter(false);

   mMeasureEndTextbox = new TextEntry(this, "end", mHeaderOffset, headerYOffset, 3, &mMeasureEnd, 0, 999999);
   mMeasureEndTextbox->DrawLabel(true);
   mMeasureEndTextbox->SetRequireEnter(false);
   mMeasureEndTextbox->SetShowing(false);

   mSyncButton = new ClickButton(this, "sync", 0, 0, ButtonDisplayStyle::kText);
   mOnFinalMeasureDropdown = new DropdownList(this, "on finish", 0, 0, &mOnEndMeasure);
   mOnFinalMeasureDropdown->DrawLabel(true);
   mLocalModeCheckbox = new Checkbox(this, "local", 0, 0, &mLocalMode);

   mCanvasIntervalDropdown = new DropdownList(this, "canvas interval", 0, 0, &mCanvasIntervalInt);
   mCanvasIntervalDropdown->DrawLabel(false);
   mCanvasIntervalDropdown->AddLabel("1n", kInterval_1n);
   mCanvasIntervalDropdown->AddLabel("2n", kInterval_2n);
   mCanvasIntervalDropdown->AddLabel("4n", kInterval_4n);
   mCanvasIntervalDropdown->AddLabel("4nt", kInterval_4nt);
   mCanvasIntervalDropdown->AddLabel("8n", kInterval_8n);
   mCanvasIntervalDropdown->AddLabel("8nt", kInterval_8nt);
   mCanvasIntervalDropdown->AddLabel("16n", kInterval_16n);
   mCanvasIntervalDropdown->AddLabel("16nt", kInterval_16nt);
   mCanvasIntervalDropdown->AddLabel("32n", kInterval_32n);
   mCanvasIntervalDropdown->AddLabel("64n", kInterval_64n);

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
      mRackGrid->AddElement(new SongCanvasRackElement(90, SongCanvasElementVariant::Enabler, "Part " + std::to_string(dPartIter), this), 0);
      dPartIter++;
      IncrementInternalRackId();
   }
   UpdateEndMode();
   Resize(mWidth, mHeight);
   //mCanvas->mViewEnd = mMeasureCount;
   //mLayerName[0]->SetNoHover(false);
}

void SongCanvas::ReloadHeader()
{
   int headerXOffset = 4;
   int headerYOffset = 8;

   if (!mResetButtonAlsoStops)
   {
      if (mLocalMode)
         mResetButton->SetLabel("replay");
      else
         mResetButton->SetLabel("reset");
   }
   else
   {
      mResetButton->SetLabel("stop");
   }

   mPlayPauseButton->SetPosition(headerXOffset, headerYOffset);
   headerXOffset += mPlayPauseButton->GetRect(true).width + 2;
   mResetButton->SetPosition(headerXOffset, headerYOffset);
   headerXOffset += mResetButton->GetRect(true).width + 2;
   mTransportSlider->SetPosition(headerXOffset, headerYOffset);
   headerXOffset += mTransportSlider->GetRect(true).width + 2;
   if (mLocalMode && mOnEndMeasure == enumOEMLoop)
   {
      mSyncButton->SetPosition(headerXOffset, headerYOffset);
      headerXOffset += mSyncButton->GetRect(true).width + 2;
      mSyncButton->SetShowing(true);
   }
   else
   {
      mSyncButton->SetShowing(false);
   }

   headerXOffset += 16;
   mHeaderSplitter1 = ofVec2f(headerXOffset + 2, headerYOffset);
   headerXOffset += 4;

   if (!mLocalMode)
   {
      mMeasureBaseTextbox->SetPosition(headerXOffset, headerYOffset);
      headerXOffset += mMeasureBaseTextbox->GetRect(true).width + 2;
      mMeasureBaseTextbox->SetShowing(true);
   }
   else
   {
      mMeasureBaseTextbox->SetShowing(false);
   }
   if (!mStartEndMeasureMode)
   {
      mMeasureCountTextbox->SetShowing(true);
      mMeasureEndTextbox->SetShowing(false);
      mMeasureCountTextbox->SetPosition(headerXOffset, headerYOffset);
      headerXOffset += mMeasureCountTextbox->GetRect(true).width+8;
   }
   else
   {
      mMeasureEndTextbox->SetPosition(headerXOffset, headerYOffset);
      mMeasureCountTextbox->SetShowing(false);
      mMeasureEndTextbox->SetShowing(true);
      headerXOffset += mMeasureEndTextbox->GetRect(true).width+8;
   }

   mCanvasIntervalDropdown->SetPosition(headerXOffset, headerYOffset);
   headerXOffset += mCanvasIntervalDropdown->GetRect(true).width + 8;
   headerXOffset += 8;
   mHeaderSplitter2 = ofVec2f(headerXOffset, headerYOffset);
   headerXOffset += 4;
   mOnFinalMeasureDropdown->SetPosition(headerXOffset, headerYOffset);
   mOnFinalMeasureDropdown->Clear();
   if (!mLocalMode)
      mOnFinalMeasureDropdown->AddLabel("continue", enumOEMContinue);

   mOnFinalMeasureDropdown->AddLabel("stop", enumOEMStop);
   mOnFinalMeasureDropdown->AddLabel("loop", enumOEMLoop);
   switch (mOnEndMeasure) //Probably a better way to do this, but can't be arsed.
   {
      case enumOEMContinue:
         mOnFinalMeasureDropdown->SetLabel("continue", enumOEMContinue);
         break;
      case enumOEMStop:
         mOnFinalMeasureDropdown->SetLabel("stop", enumOEMStop);
         break;
      case enumOEMLoop:
         mOnFinalMeasureDropdown->SetLabel("loop", enumOEMLoop);
         break;
   }

   mLocalModeCheckbox->SetPosition(mWidth - 47, headerYOffset);
}
ofColor SongCanvas::GetFancyStyleColour(EnumSongCanvasStyle style, float time)
{
   switch (style)
   {
      case ESRed:
         return ofColor::red;
      case ESPink:
         return ofColor(255, 0, 238);
      case ESYellow:
         return ofColor::yellow;
      case ESCyan:
         return ofColor::cyan;
      case ESGreen:
         return ofColor::green;
      case ESOrange:
         return ofColor::orange;
      case ESPurple:
         return ofColor::purple;
      case ESBlue:
         return ofColor::blue;
      case ESWhite:
         return ofColor::white;
      case ESBlack:
         return ofColor::black;
      case ESGlass:
         return ofColor(166, 237, 255, 150);
      case ESCarbon:
         return ofColor::arrayInterpolate(ESCarbonColours, mTime);
      case ESCheckerboard:
         if ((int)floor(time) % 2 == 0)
            return ofColor(255, 255, 255);
         return ofColor(0, 0, 0);
      case ESTransport:
         if ((int)floor(time) % 2 == 0)
            return ofColor::cyan;
         return ofColor(255, 0, 238);
      case ESRGB:
         return ofColor::arrayInterpolate(ESRGBColours, mTime * 2);
      case ESPride:
         return ofColor::arrayInterpolate(ESPrideColours, mTime * 2);
      case ESTrans:
         return ofColor::arrayInterpolate(ESTransColours, mTime);
      default:
         return ofColor::red;
   }
}

void SongCanvas::UpdateEndMode()
{
   if (mLocalMode)
   {
      if (mPreviousLocalEndMeasure == -1)
      {
         mOnEndMeasure = enumOEMLoop;
      }
      else
         mOnEndMeasure = mPreviousLocalEndMeasure;

      if (mOnEndMeasure == enumOEMLoop)
      {
         mSyncButton->SetEnabled(!mLocalSynced);
      }
   }
   else
   {
      mTransportSlider->SetLineColour(ofColor::red);
      mMeasureSlider->SetLineColour(ofColor::red);

      if (mPreviousGlobalEndMeasure == -1)
      {
         mOnEndMeasure = enumOEMContinue;
      }
      else
      {
         mOnEndMeasure = mPreviousGlobalEndMeasure;
      }
   }
}

void SongCanvas::DrawModule()
{
   if (Minimized() || IsVisible() == false)
      return;

   int startCanvasOffset;
   if (expertPanelEnabled)
      startCanvasOffset = LayersListWidthSize + AdvancedConfigHSize;
   else
      startCanvasOffset = LayersListWidthSize;

   ofPushStyle();
   ofFill();
   if (mLocalMode)
   {
      mTransportSlider->SetLineColour(GetFancyStyleColour(mLocalModeColor, mTime));
      mMeasureSlider->SetLineColour(GetFancyStyleColour(mLocalModeColor, mTime));
   }
   else
   {
      mTransportSlider->SetLineColour(GetFancyStyleColour(mGlobalModeColor, mTime));
      mMeasureSlider->SetLineColour(GetFancyStyleColour(mGlobalModeColor, mTime));
   }
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

   float canvasFoot = mOffsetFromTopSpacing + mCanvas->GetHeight();


   //ofSetColor(ofColor::clear);


   mCanvas->Draw();

   ofSetColor(ofColor(255, 255, 255));
   //DrawTextNormal("viewCompression: "+ofToString(viewCompression), mCanvas->GetRect(true).x+4, mCanvas->GetRect(true).y+16);
   //DrawTextNormal("canvasViewStart: "+ofToString(mCanvas->mViewStart), mCanvas->GetRect(true).x+4, mCanvas->GetRect(true).y+32);
   //DrawTextNormal("canvasViewEnd: "+ofToString(mCanvas->mViewEnd), mCanvas->GetRect(true).x+4, mCanvas->GetRect(true).y+48);
   //mCanvas->RescaleForZoom()

   ofColor softLineColor = ofColor{ 255, 255, 255, 20 };
   ofColor hardLineColor = ofColor{ 255, 255, 255, 50 };
   ofColor labelColor = ofColor{ 0, 0, 0, 130 };


   mMeasureSlider->SetDimensions(mCanvas->GetWidth() + 2, 15); //+2 fixes a very slight but annoying visual disconnect between it and the Canvas transport line.
   mMeasureSlider->SetExtents(mMeasureStart + mCanvas->mViewStart, mMeasureStart + mCanvas->mViewEnd);
   //DrawTextNormal("measure", 4, 8);
   mTransportSlider->Draw();
   mResetButton->Draw();
   mPlayPauseButton->Draw();
   mCanvasTimeline->Draw();
   mMeasureSlider->Draw();
   mLocalModeCheckbox->Draw();
   mOnFinalMeasureDropdown->Draw();
   mCanvasIntervalDropdown->Draw();
   if (mLocalMode && mOnEndMeasure == enumOEMLoop)
   {
      mSyncButton->Draw();
   }

   ofSetColor(ofColor(150, 150, 150));
   ofLine(mHeaderSplitter1.x - 8, mHeaderSplitter1.y, mHeaderSplitter1.x - 8, mHeaderSplitter1.y + 15);
   ofLine(mHeaderSplitter2.x - 8, mHeaderSplitter2.y, mHeaderSplitter2.x - 8, mHeaderSplitter2.y + 15);
   ofPopStyle();

   mMeasureBaseTextbox->Draw();
   if (!mStartEndMeasureMode)
      mMeasureCountTextbox->Draw();
   else
      mMeasureEndTextbox->Draw();

   if (TheSynth->IsAudioPaused() || (mLocalMode && mLocalStopped))
      mPlayPauseButton->SetDisplayStyle(ButtonDisplayStyle::kPlay);
   else
      mPlayPauseButton->SetDisplayStyle(ButtonDisplayStyle::kPause);

   if (mShowRealTime)
   {
      float sDTime = round(mMeasureStart * (TheTransport->MsPerBar() / 1000) * 10) / 10;
      float eDTime = round(mMeasureCount * (TheTransport->MsPerBar() / 1000) * 10) / 10;
      float cDTime = round(mTime * (TheTransport->MsPerBar() / 1000) * 10) / 10;
      std::string timeField;

      std::string std2;

      if (mStartEndMeasureMode && !mLocalMode)
      {
         eDTime += sDTime;
         std2 = "-";
      }
      else
      {
         std2 = "";
      }

      std::string str1;
      if (fmod(cDTime, 1) == 0)
         str1 = ".0";
      int mins1 = floor(sDTime / 60);
      int mins2 = floor(eDTime / 60);
      int mins3 = floor(cDTime / 60);
      std::string strM1;
      std::string strM2;
      std::string strM3;
      if (mins1 > 0)
         strM1 = ofToString(mins1) + ":";
      if (mins2 > 0)
         strM2 = ofToString(mins2) + ":";
      if (mins3 > 0)
         strM3 = ofToString(mins3) + ":";

      std::string strStartMeasure;
      if (!mLocalMode)
         strStartMeasure = "(" + strM1 + ofToString(fmod(sDTime, 60)) + ")"+std2;
      std::string strMeasureCount =  "(" + strM2 + ofToString(fmod(eDTime, 60)) + ")";
      std::string strCurrentTime = " [" + strM3 + ofToString(fmod(cDTime, 60)) + str1 + "]";

      timeField = strStartMeasure + strMeasureCount + strCurrentTime;
      DrawTextMonoRightJustify(timeField, mLocalModeCheckbox->GetRect(true).x - 4, mLocalModeCheckbox->GetRect(true).y + 13);
   }
   float drawPointOffset = startCanvasOffset;
   float measuresVisible = mCanvas->mViewEnd - mCanvas->mViewStart; //12 = (ex:default)number of measures visible, 0 = infinitely zoomed in
   float measureOffset = mCanvas->GetWidth() / mMeasureCount * (mMeasureCount / measuresVisible);
   double viewCompression = 48 / measureOffset; //1~ is about the intended value for measures being around 48px long.
   int viewCullMultiplier = MAX(1, floor(viewCompression - 0.2));
   drawPointOffset -= measureOffset * mCanvas->mViewStart;
   int iter = 0;

   //If too many measures are visible on screen. Start culling.
   if (viewCullMultiplier > 2)
      mCanvas->SetMinorColumnLineColor(ofColor::clear);
   else
   {
      mCanvas->SetMinorColumnLineColor(ofColor{ 50, 50, 50 });
   }

   if (viewCullMultiplier > 8)
   {
      mCanvas->SetMajorColumnLineColor(ofColor{ 50, 50, 50, 100 });
   }
   else
   {
      if (viewCullMultiplier > 2)
      {
         mCanvas->SetMajorColumnLineColor(ofColor{ 50, 50, 50 });
      }
      else
         mCanvas->SetMajorColumnLineColor(ofColor{ 180, 180, 180 });
   }
   mCanvas->SetMajorColumnInterval(4 * viewCullMultiplier);

   //Draw lines over the timeline, aligned with the canvas.
   while (drawPointOffset < mWidth)
   {
      if (drawPointOffset < startCanvasOffset)
      {
         iter++;
         drawPointOffset += measureOffset * viewCullMultiplier;
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

      ofLine(drawPointOffset, mOffsetFromTopSpacing - 16, drawPointOffset, mOffsetFromTopSpacing);

      if (iter % 1 == 0)
      {
         ofSetColor(labelColor);

         int displayNum;
         if (!mLocalMode)
            displayNum = mMeasureStart + iter * viewCullMultiplier;
         else
            displayNum = iter * viewCullMultiplier;
         DrawTextNormal(ofToString(displayNum), drawPointOffset + 2, mOffsetFromTopSpacing - 2, 13);
      }
      drawPointOffset += measureOffset * viewCullMultiplier;
      iter++;
   }
   //Now the timeline position line
   if (!mLocalMode)
      ofSetColor(GetFancyStyleColour(mGlobalModeColor, mTime));
   else
      ofSetColor(GetFancyStyleColour(mLocalModeColor, mTime));
   float markerLinePos = startCanvasOffset + ofMap(mCanvasRelativeTime * mMeasureCount, mCanvas->mViewStart, mCanvas->mViewEnd, 0, mCanvas->GetWidth());
   if (markerLinePos > startCanvasOffset && markerLinePos < mWidth)
      ofLine(markerLinePos, mOffsetFromTopSpacing, markerLinePos, canvasFoot);
   ofSetColor(ofColor::grey);
   mMainScrollbarHorizontal->Draw();
   ofPopStyle();


   int s = seqLayers.size();
   for (int i = 0; i < s; i++)
   {
      mLayerNameTextbox[i]->Draw();
      mLayerEnableCheckbox[i]->Draw();
      mLayerSettingsButton[i]->Draw();
   }
   //Draw the rack
   if (mFlashRackStartTime > 0)
   {
      mRackGrid->SetBackgroundColour(255, 255, 255, CLAMP(40 + sin(ofGetGlobalTime() * 10) * 30, 0, 255));
      if (mFlashRackStartTime + 1 < ofGetGlobalTime())
      {
         mFlashRackStartTime = 0;
         mRackGrid->SetBackgroundColour(0, 0, 0, 75);
      }
   }
   mRackGrid->Draw();
   mRackAddNewButton->Draw();
   ofPushStyle();

   //DEBUG TEXT, UNCOMMENT FOR ENLIGHTENMENT
   /*
   auto canvasRect = mCanvas->GetRect(true);
   std::string dText = "View Cull: " + ofToString(viewCullMultiplier)+
      "\nZoomPercent: "+ofToString(measuresVisible)+
         "\nViewStart: "+ofToString(mCanvas->mViewStart)+
            "\nViewEnd: "+ofToString(mCanvas->mViewEnd)+
               "\nCanvas Width: "+ofToString(mCanvas->GetWidth())+
                  "\nPanel Width: "+ofToString(mWidth)+
                     "\nCanvas Length(measures): "+ofToString(mCanvas->GetLength())+
                        "\nCanvas Columns: "+ofToString(mCanvas->GetNumCols());
   DrawTextNormal(dText,canvasRect.x+4, canvasRect.y+10);*/
}
void SongCanvas::CanvasUpdated(Canvas* canvas)
{
   if (mCanvas == canvas)
   {
      auto elms = mCanvas->GetElements();

      //How many chunks should we have?
      int colNum = mCanvas->GetNumCols();

      mChunkAmount = CLAMP(colNum, 50, 200);

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
      }
   }
}
void SongCanvas::FeatureResize(int extraW, int extraH)
{
   float h = mHeight;
   float w = mWidth;
   Resize(w + extraW, h + extraH);
}
void SongCanvas::Resize(float w, float h)
{
   w = MAX(w, 450);
   h = MAX(h, 100 + seqLayers.size() * MinRowSize + mFlowGridRows * FlowGridRowHeightSize);

   if (mMeasureSize == 0)
   {
      mMeasureSize = (w - mStartCanvasXOffset) / mMeasureCount;
   }

   int multiple = std::ceil((w - LayersListWidthSize) / mMeasureSize);
   //Snap to a strict size.
   w = LayersListWidthSize + multiple * mMeasureSize;
   mWidth = w;
   mHeight = h;

   float canvasHeight = h - (16 + mOffsetFromTopSpacing + mFlowGridRows * FlowGridRowHeightSize);
   mCanvas->SetDimensions(w - mStartCanvasXOffset, canvasHeight);
   ReloadMeasures(false);
   //Layers <>V
   for (int i = 0; i < seqLayers.size(); ++i)
   {
      float layerPosSpacing = mCanvas->GetHeight() / static_cast<float>(seqLayers.size());
      float midCentering = layerPosSpacing / 4;
      mLayerNameTextbox[i]->SetPosition(28, mOffsetFromTopSpacing + midCentering + i * layerPosSpacing);
      mLayerEnableCheckbox[i]->SetPosition(mStartCanvasXOffset - 13, mOffsetFromTopSpacing + midCentering + i * layerPosSpacing);
      mLayerSettingsButton[i]->SetPosition(4, mOffsetFromTopSpacing + midCentering + i * layerPosSpacing);
   }


   int xEndRackSpacing = 46;
   mRackGrid->SetPosition(8, GetRackGridStartYOffset());
   mRackGrid->SetDimensions(mWidth - xEndRackSpacing, mFlowGridRows * FlowGridRowHeightSize);
   mRackGrid->RecalculateElements();
   mRackAddNewButton->SetPosition(8 + mWidth - xEndRackSpacing, GetRackGridStartYOffset());
   mRackAddNewButton->SetDimensions(28, mFlowGridRows * FlowGridRowHeightSize);

   ReloadHeader();
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
   mLayerEnableCheckbox[lIndex] = new Checkbox(this, ("checkbox" + std::to_string(lIndex)).c_str(), mStartCanvasXOffset - 8, mOffsetFromTopSpacing + midCentering + lIndex * layerPosSpacing, &seqLayers[lIndex].enabled);
   mLayerEnableCheckbox[lIndex]->SetDisplayText(false);
   mLayerSettingsButton[lIndex] = new ClickButton(this, ("setting" + std::to_string(lIndex)).c_str(), mStartCanvasXOffset - 28, mOffsetFromTopSpacing + midCentering + lIndex * layerPosSpacing, ButtonDisplayStyle::kHamburger);
   //mCanvas->SetRowColor(i,ofColor::clear)
   MoveLayerTo(lIndex, index);
}
void SongCanvas::DeleteLayer(int index)
{
   int lIdx = seqLayers.size() - 1;
   MoveLayerTo(index, lIdx);

   mLayerNameTextbox[lIdx]->RemoveFromOwner();
   mLayerEnableCheckbox[lIdx]->RemoveFromOwner();
   mLayerSettingsButton[lIdx]->RemoveFromOwner();

   auto layerEl = GetAllCanvasElementsOfLayer(lIdx);
   for (int i = 0; i < layerEl.size(); ++i)
   {
      mCanvas->RemoveElement(layerEl[i]);
   }

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

bool SongCanvas::IsCanvasElementActive(SongCanvas_CanvasElement* element) const
{
   for (int i = 0; i < mActiveElements.size(); ++i)
   {
      if (mActiveElements[i] == element)
         return true;
   }
   return false;
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
   if (entry == mMeasureBaseTextbox || entry == mMeasureCountTextbox || entry == mMeasureEndTextbox)
   {
      //Overrides any auto-scaling that may occur.
      ReloadMeasures(true);
      return;
   }
}
void SongCanvas::ReloadMeasures(bool overrideAutoFit)
{
   float oldViewEnd = mCanvas->mViewEnd;
   float oldMeasureCount = mCanvas->GetLength();
   bool loopMaxed = 0 == mCanvas->mLoopStart && mCanvas->GetLength() == mCanvas->mLoopEnd;
   if (mMeasureSize <= 0)
   {
      mMeasureSize = mStandardMeasureSize;
   }
   if (mStartEndMeasureMode)
   {
      mMeasureEnd = MAX(mMeasureStart + 1, mMeasureEnd);
      mMeasureCount = mMeasureEnd - mMeasureStart;
   }

   if (mReloadMeasureLoadFlag)
   {
      mReloadMeasureLoadFlag = false;
      mMeasureSize = mCanvas->GetWidth() / static_cast<float>(mMeasureCount);
   }
   if (mAutoScaleMeasureCount && !overrideAutoFit)
   {
      mMeasureCount = ceil(mCanvas->GetWidth() / mMeasureSize);
      mMeasureCount = MAX(1, mMeasureCount);
   }
   else
   {
      mMeasureCount = MAX(1, mMeasureCount); //Cannot hold less than one measure. Stuff is likely to break otherwise.
   }
   if (mStartEndMeasureMode) //Since measure count probably changed by this point, we ought to sync it again.
   {
      mMeasureEnd = mMeasureStart + mMeasureCount;
   }
   mMeasureSize = ceil(mCanvas->GetWidth() / static_cast<float>(mMeasureCount));
   mCanvas->SetLength(mMeasureCount); //This line is responsible for an untold amount of misery.
   if (mAlteredIntervalFlag)
   {
      mCanvas->RescaleNumCols(TheTransport->CountInStandardMeasure(mCanvasInterval) * mMeasureCount);
      mAlteredIntervalFlag = false;
   }
   else
   {
      mCanvas->SetNumCols(TheTransport->CountInStandardMeasure(mCanvasInterval) * mMeasureCount);
   }
   if (mAutoScaleMeasureCount && !overrideAutoFit)
   {
      mCanvas->mViewEnd = MIN(mMeasureCount, oldViewEnd * mMeasureCount / oldMeasureCount);
   }
   else
   {
      mCanvas->mViewEnd = MIN(mMeasureCount, mCanvas->mViewEnd);
   }
   mPartCanvasDirty = true;
   if (loopMaxed)
   { //If its already maxed, we sync it.
      mCanvas->mLoopStart = 0;
      mCanvas->mLoopEnd = mCanvas->GetLength();
   }
   else //Otherwise just ensure it's within bounds
   {
      mCanvas->mLoopEnd = MIN(mCanvas->GetLength(), mCanvas->mLoopEnd);
      if (mCanvas->mLoopStart >= mCanvas->mLoopEnd) //If its too small reset.
      {
         mCanvas->mLoopStart = 0;
         mCanvas->mLoopEnd = mCanvas->GetLength();
      }
      UserUpdatedCanvasTimeline(mCanvas->mLoopStart, mCanvas->mLoopEnd);
   }
   mTransportSlider->SetExtents(mMeasureStart, mMeasureStart + mMeasureCount);
   mMeasureSlider->SetExtents(mMeasureStart, mMeasureStart + mMeasureCount);
   mMeasureCountTextbox->UpdateDisplayString();
}

void SongCanvas::FloatSliderUpdated(FloatSlider* slider, float oldVal, double time)
{
   if (!mLocalMode)
   {
      if (slider == mMeasureSlider || slider == mTransportSlider)
      {
         TheTransport->SetMeasureTime(mTime);
      }
   }
   else
   {
      if (slider == mMeasureSlider || slider == mTransportSlider)
      {
         if (mLocalSynced && mOnEndMeasure == enumOEMLoop)
         {
            mLocalSynced = false;
            mSyncButton->SetEnabled(true);
         }
      }
   }
}
void SongCanvas::ButtonClicked(ClickButton* button, double time)
{
   if (button == mPlayPauseButton)
   {
      if (!mLocalMode)
      {
         TheSynth->SetAudioPaused(!TheSynth->IsAudioPaused());
      }
      else
      {
         mLocalStopped = !mLocalStopped;
      }
      return;
   }
   if (button == mResetButton)
   {
      if (!mLocalMode)
      {
         TheTransport->SetMeasureTime(mCanvas->mLoopStart);
         if (mResetButtonAlsoStops)
         {
            TheSynth->SetAudioPaused(!TheSynth->IsAudioPaused());
         }
      }
      else
      {
         mLocalStopped = false;
         mTime = mCanvas->mLoopStart;
         mLocalSynced = false;
         mSyncButton->SetEnabled(true);
         if (mResetButtonAlsoStops)
         {
            mLocalStopped = true;
         }
      }

      return;
   }
   if (button == mRackAddNewButton)
   {
      auto rp = mRackAddNewButton->GetPosition(true);
      mRackAddNewDropdown->SetPosition(rp.x, rp.y);
      mRackAddNewDropdown->OnClicked(1, 1, false);
      mRackAddNewDropdown->SetPosition(-2000, -2000);
      //DropdownClicked(mRackAddNewDropdown);
      return;
   }
   if (button == mSyncButton)
   {
      mLocalSynced = true;
      mSyncButton->SetEnabled(false);
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
            mListDropdownOptions->AddLabel("Delete", LayerDropDownOptions::enumLDPDelete);
         }

         auto rp = mLayerSettingsButton[i]->GetPosition(true);
         mListDropdownOptions->SetPosition(rp.x, rp.y);
         mListDropdownOptions->OnClicked(1, 1, false);
         mListDropdownOptions->SetPosition(-2000, -2000);
         return;
      }
   }
}
void SongCanvas::CheckboxUpdated(Checkbox* checkbox, double time)
{
   if (checkbox == mLocalModeCheckbox)
   {
      UpdateEndMode();
      ReloadHeader();
   }
}
bool SongCanvas::MouseMoved(float x, float y)
{
   IDrawableModule::MouseMoved(x, y);
   mRackGrid->MouseMoved(x, y);
   return false;
}
void SongCanvas::OnClicked(float x, float y, bool right)
{
   IDrawableModule::OnClicked(x, y, right);
   mRackGrid->OnClicked(x - mRackGrid->GetPosition().x, y - mRackGrid->GetPosition().y, right);
}
void SongCanvas::MouseReleased()
{
   IDrawableModule::MouseReleased();
   mRackGrid->MouseReleased();
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
            mRackGrid->AddElement(new SongCanvasRackElement(90, SongCanvasElementVariant::Enabler, newPartName, this));
            break;
         case enumPulser:
            mRackGrid->AddElement(new SongCanvasRackElement(140, SongCanvasElementVariant::Pulser, newPartName, this));
            break;
         case enumModulator: break;
         case enumSample: break;
         case enumOnePulse:
            mRackGrid->AddElement(new SongCanvasRackElement(90, SongCanvasElementVariant::OnePulse, newPartName, this));
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
                              "layer" + ofToString(seqLayers.size()) });
         FeatureResize(0, mCanvas->GetHeight() / seqLayers.size());
      }
   }
   if (list == mOnFinalMeasureDropdown)
   {
      if (mLocalMode)
      {
         mPreviousLocalEndMeasure = mOnFinalMeasureDropdown->GetValue();
      }
      else
      {
         mPreviousGlobalEndMeasure = mOnFinalMeasureDropdown->GetValue();
      }
      ReloadHeader();
      return;
   }
   if (list == mCanvasIntervalDropdown)
   {
      mCanvasInterval = (NoteInterval)mCanvasIntervalInt;
      mAlteredIntervalFlag = true;
      ReloadMeasures(false);
   }
   for (int i = 0; i < mRackGrid->GetAllElements().size(); ++i)
   {
      auto e = dynamic_cast<SongCanvasRackElement*>(mRackGrid->GetAllElements()[i]);
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
   mRackGrid->RemoveElement(element);
}
std::vector<SongCanvasRackElement*> SongCanvas::GetAllRackElements() const
{
   std::vector<SongCanvasRackElement*> output;
   auto elms = mRackGrid->GetAllElements();
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
void SongCanvas::UserUpdatedCanvasTimeline(float newLoopMin, float newLoopMax)
{
   if (newLoopMin == mCanvas->mViewStart && newLoopMax == mCanvas->mViewEnd)
   {
      mCanvasTimeline->SetBaseColour(ofColor(25, 25, 25));
   }
   else
   {
      mCanvasTimeline->SetBaseColour(ofColor(180, 0, 0));
   }
}
void SongCanvas::OnTransportAdvanced(float amount)
{
   if (!mLocalMode || (mLocalMode && mLocalSynced && mOnEndMeasure == enumOEMLoop))
      mTime = TheTransport->GetMeasureTime(gTime);
   else
   {
      if (!mLocalStopped)
         mTime += amount;
   }
   //First check if we have any cleanup to do.
   if (mPartCanvasDirty)
   {
      CanvasUpdated(mCanvas);
      mPartCanvasDirty = false;
   }

   //The 0.02f refers to a small nudge to help it activate modules at points where they can activate notes at the exact same time more reliably.
   if (!mLocalMode) //Global Time ops
   {

      bool timelineLoopActive = true;
      if (mCanvas->mLoopEnd == mCanvas->GetLength() && mCanvas->mLoopStart == 0)
         timelineLoopActive = false;

      if (timelineLoopActive)
      {
         if (mTime > mCanvas->mLoopEnd)
         {
            mTime = mCanvas->mLoopStart;
            TheTransport->SetMeasureTime(mTime);
         }
      }
      else if (mOnEndMeasure == EnumOnEndMeasure::enumOEMLoop)
      {
         if (mTime > mCanvas->GetLength())
         {
            mTime = mCanvas->mLoopStart;
            TheTransport->SetMeasureTime(mTime);
         }
      }
      else if (mOnEndMeasure == EnumOnEndMeasure::enumOEMStop)
      {
         if (mTime > mCanvas->GetLength())
         {
            mTime = mCanvas->mLoopStart;
            TheTransport->SetMeasureTime(mTime);
            TheSynth->SetAudioPaused(!TheSynth->IsAudioPaused());
         }
      }
      mCanvasRelativeTime = (mTime - mMeasureStart + 0.02f) / mMeasureCount;
   }
   else //Local time Ops
   {
      if (mOnEndMeasure == EnumOnEndMeasure::enumOEMLoop)
      {
         if (mLocalSynced)
         {
            mTime = mCanvas->mLoopStart + std::fmod(mTime, mCanvas->mLoopEnd - mCanvas->mLoopStart);
         }
         else
         {
            if (mTime > mCanvas->mLoopEnd)
            {
               mTime = mCanvas->mLoopStart;
            }
         }
      }
      else if (mOnEndMeasure == EnumOnEndMeasure::enumOEMStop)
      {
         if (mTime > mCanvas->mLoopEnd)
         {
            mTime = mCanvas->mLoopStart;
            mLocalStopped = true;
         }
      }
      mCanvasRelativeTime = (mTime + 0.02f) / mMeasureCount;
   }
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
}
void SongCanvas::ReceiveSignal(SignalId signalID)
{
   if (signalID == SignalId::ResizeRequest)
   {
      mFlowGridRows = mRackGrid->GetRowCount();

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
}

//Attempt to resize based on the addition/removal of a feature.

void SongCanvas::LoadLayout(const ofxJSONElement& moduleInfo)
{
   mModuleSaveData.LoadBool("show_real_time", moduleInfo, false);
   mModuleSaveData.LoadBool("auto_scale_measure_count", moduleInfo, true);
   mModuleSaveData.LoadBool("start_end_measure_mode", moduleInfo, false);
   mModuleSaveData.LoadBool("reset_button_also_stops", moduleInfo, false);
   EnumMap mapCols;
   mapCols["red"] = 0;
   mapCols["pink"] = 1;
   mapCols["yellow"] = 2;
   mapCols["cyan"] = 3;
   mapCols["green"] = 4;
   mapCols["orange"] = 5;
   mapCols["purple"] = 6;
   mapCols["blue"] = 7;
   mapCols["white"] = 8;
   mapCols["black"] = 9;
   mapCols["glass"] = 10;
   mapCols["carbon"] = 11;
   mapCols["checkerboard"] = 12;
   mapCols["transport"] = 13;
   mapCols["rgb"] = 14;
   mapCols["pride"] = 15;
   mapCols["trans"] = 16;
   mModuleSaveData.LoadEnum<EnumSongCanvasStyle>("global_colour_style", moduleInfo, 0, nullptr, &mapCols);
   mModuleSaveData.LoadEnum<EnumSongCanvasStyle>("local_colour_style", moduleInfo, 1, nullptr, &mapCols);
}
void SongCanvas::SetUpFromSaveData()
{
   mAutoScaleMeasureCount = mModuleSaveData.GetBool("auto_scale_measure_count");
   mStartEndMeasureMode = mModuleSaveData.GetBool("start_end_measure_mode");
   mResetButtonAlsoStops = mModuleSaveData.GetBool("reset_button_also_stops");
   mShowRealTime = mModuleSaveData.GetBool("show_real_time");

   mGlobalModeColor = mModuleSaveData.GetEnum<EnumSongCanvasStyle>("global_colour_style");
   mLocalModeColor = mModuleSaveData.GetEnum<EnumSongCanvasStyle>("local_colour_style");


   ReloadHeader();
   ReloadMeasures(false);
}
void SongCanvas::SaveLayout(ofxJSONElement& moduleInfo)
{
}
void SongCanvas::SaveState(FileStreamOut& out)
{
   out << GetModuleSaveStateRev(); //Updating to newer versions? Check this handy variable!
   //SEC-1 format 05/03/2026
   //REV-2 format 06/03/2026 - Resolves buggy panel size values.

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
   out << mWidth;
   out << mHeight;

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
   out << mLocalMode;
   out << mMeasureStart;
   out << mMeasureCount;
   out << mAutoScaleMeasureCount;
   //out << mLoopOnEnd;
   out << mCanvas->mLoopStart;
   out << mCanvas->mLoopEnd;
   //End reserved variables SEC-1

   //REV 4
   out << mPreviousGlobalEndMeasure;
   out << mPreviousLocalEndMeasure;
   out << mLocalStopped;
   out << mLocalSynced;
   out << mOnEndMeasure;
   out << mTime;

   //Rev5
   out << mCanvasIntervalInt;
}
void SongCanvas::LoadState(FileStreamIn& in, int rev)
{
   int canvasLayerCount;

   //restore our canvas
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

   //clean up our starter rack.
   for (int i = 0; i < re.size(); ++i)
   {
      DeleteRackElement(re[i]);
   }

   //now load our real rack.
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
      mRackGrid->AddElement(nrm);
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
   //time to set up our Canvas and scaling now

   int i1;
   float f1;
   float f2;
   in >> i1;
   mCanvas->SetNumRows(i1);
   in >> i1;
   //mCanvas->SetNumCols(i1);no longer used
   in >> f1;
   mCanvas->SetLength(f1);
   in >> f1;
   in >> f2;
   if (rev == 1)
   { //Buggy dimensions in this version.
      mWidth = f1 + 144;
      mHeight = f2 + 128;
   }
   else
   {
      mWidth = f1;
      mHeight = f2;
   }

   in >> mFlowGridRows;

   //Most of this can probably be thrown away.
   float bWSize;
   float bHSize;
   mRackAddNewButton->GetDimensions(bWSize, bHSize);
   mRackGrid->SetDimensions(mCanvas->GetWidth() - 16 + GetCanvasStartXOffset() - bWSize * 1.5f, mFlowGridRows * FlowGridRowHeightSize);
   mRackGrid->RecalculateElements();
   auto mgp = mRackGrid->GetPosition(true);
   mRackAddNewButton->SetDimensions(bWSize, mFlowGridRows * FlowGridRowHeightSize);
   mRackAddNewButton->SetPosition(mgp.x + mRackGrid->GetWidth(), mgp.y);
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

   float vS = 0;
   float vE = 0;
   in >> vS;
   in >> vE;
   if (rev < 3)
   {
      vS *= 12;
      vE *= 12;
   }
   mCanvas->mViewStart = vS;
   mCanvas->mViewEnd = vE;

   CanvasUpdated(mCanvas);
   //More misc stuff.
   in >> mPartNameCount;
   bool enableState;
   in >> enableState;
   if (rev > 1)
   {
      bool dump;
      in >> mLocalMode;
      in >> mMeasureStart;
      in >> mMeasureCount;
      in >> mAutoScaleMeasureCount;
      if (rev < 4)
         in >> dump; //This variable is no longer used.
      in >> mCanvas->mLoopStart;
      in >> mCanvas->mLoopEnd;
   }
   if (rev < 3)
   {
      mCanvas->mLoopStart = 0;
      mCanvas->mLoopEnd = mMeasureCount;
   }
   if (rev == 1)
   {
      mMeasureCount = ceil((mWidth - mStartCanvasXOffset) / (mCanvas->GetNumCols()));
   }
   if (rev >= 4)
   {
      in >> mPreviousGlobalEndMeasure;
      in >> mPreviousLocalEndMeasure;
      in >> mLocalStopped;
      in >> mLocalSynced;
      in >> mOnEndMeasure;
      float t;
      in >> t;
      if (mLocalMode)
      {
         mSyncButton->SetEnabled(!mLocalSynced);
         mTime = t;
      }
   }
   if (rev >= 5)
   {
      in >> mCanvasIntervalInt;
      mCanvasInterval = (NoteInterval)mCanvasIntervalInt;
   }
   UserUpdatedCanvasTimeline(mCanvas->mLoopStart, mCanvas->mLoopEnd);

   mReloadMeasureLoadFlag = true;
   SetEnabled(enableState);
   Resize(mWidth, mHeight);
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