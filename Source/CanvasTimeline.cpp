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
/*
  ==============================================================================

    CanvasTimeline.cpp
    Created: 20 Mar 2021 12:35:05am
    Author:  Ryan Challinor

  ==============================================================================
*/

#include "CanvasTimeline.h"
#include "Canvas.h"
#include "ModularSynth.h"

CanvasTimeline::CanvasTimeline(Canvas* canvas, std::string name)
: mCanvas(canvas)
{
   SetName(name.c_str());
   SetParent(canvas->GetModuleParent());
}

void CanvasTimeline::Render()
{
   ofRectangle canvasRect = mCanvas->GetRect(true);
   SetPosition(canvasRect.x, canvasRect.y + mCanvasYOffset);
   SetDimensions(canvasRect.width, 10);
   bool maxed = mCanvas->mLoopStart == 0 && mCanvas->mLoopEnd == mCanvas->GetLength();
   bool shiftPreview = false;
   if (GetKeyModifiers() & kModifier_Shift)
   {
      if (gHoveredUIControl == this)
         if (!mHideShiftTemp)
            shiftPreview = true;
   }
   else
      mHideShiftTemp = false;
   if (mShiftOverride)
   {
      shiftPreview = true;
   }


   ofPushMatrix();
   ofTranslate(mX, mY);

   float startX = ofMap(mCanvas->mLoopStart, mCanvas->mViewStart, mCanvas->mViewEnd, 0, mWidth);
   float endX = ofMap(mCanvas->mLoopEnd, mCanvas->mViewStart, mCanvas->mViewEnd, 0, mWidth);

   if (mClick && (mHoverMode == HoverMode::kStart || mHoverMode == HoverMode::kMiddle))
      startX += mDragOffset.x;
   if (mClick && (mHoverMode == HoverMode::kEnd || mHoverMode == HoverMode::kMiddle))
      endX += mDragOffset.x;

   ofPushStyle();
   if (mClick && mHoverMode == HoverMode::kMiddle)
   {
      ofSetColor(150, 150, 150);
      ofNoFill();
      float quantizedStart = GetQuantizedForX(startX, HoverMode::kMiddle);
      float quantizedStartX = ofMap(quantizedStart, mCanvas->mViewStart, mCanvas->mViewEnd, 0, mWidth);
      float quantizedEnd = GetQuantizedForX(endX, HoverMode::kMiddle);
      float quantizedEndX = ofMap(quantizedEnd, mCanvas->mViewStart, mCanvas->mViewEnd, 0, mWidth);
      ofRect(quantizedStartX, 0, quantizedEndX - quantizedStartX, mHeight / 2, 0);
   }
   if (mHoverMode == HoverMode::kMiddle)
   {
      if (!maxed && !shiftPreview)
         ofSetColor(mHighlightColour);
      else
         ofSetColor(mBaseSmallHighlight);
   }
   else
   {
      if (!shiftPreview)
         ofSetColor(mBaseColour);
      else
         ofSetColor(mBaseSmallHighlight);
   }
   ofFill();
   if (mClick)
      ofRect(startX, 0, endX - startX, mHeight / 2, 0);
   else
   {
      ofRect(MAX(startX, 0), 0, MIN(mWidth, endX - startX), mHeight / 2, 0);
   }
   if (!shiftPreview)
   {
      if (mClick && mHoverMode == HoverMode::kStart)
      {
         ofSetColor(150, 150, 150);
         ofNoFill();
         float quantized = GetQuantizedForX(startX, HoverMode::kStart);
         float quantizedX = ofMap(quantized, mCanvas->mViewStart, mCanvas->mViewEnd, 0, mWidth);
         DrawTriangle(quantizedX, 1);
      }
      if (mHoverMode == HoverMode::kStart)
         ofSetColor(mCornerHighlightColour);
      else
         ofSetColor(mCornerBaseColour);
      ofFill();
      if (startX >= 0 || mClick)
         DrawTriangle(startX, 1);
      else
      {
         ofSetLineWidth(2);
         ofLine(0, 0, 0, mHeight);
      }


      if (mClick && mHoverMode == HoverMode::kEnd)
      {
         ofSetColor(150, 150, 150);
         ofNoFill();
         float quantized = GetQuantizedForX(endX, HoverMode::kEnd);
         float quantizedX = ofMap(quantized, mCanvas->mViewStart, mCanvas->mViewEnd, 0, mWidth);
         DrawTriangle(quantizedX, -1);
      }
      if (mHoverMode == HoverMode::kEnd)
         ofSetColor(mCornerHighlightColour);
      else
         ofSetColor(mCornerBaseColour);
      ofFill();
      if (endX <= mWidth || mClick) //Probably best to make this a variable.
         DrawTriangle(endX, -1);
      else
      {
         ofSetLineWidth(2);
         ofLine(mWidth, 0, mWidth, mHeight);
      }
   }
   //Hover shift behaviors
   if (shiftPreview)
   {
      if (!mShiftOverride)
      {
         ofSetColor(150, 150, 150);
         ofNoFill();
         float quantized = GetQuantizedForX(mMousePos.x, HoverMode::kNone);
         float quantizedX = ofMap(quantized, mCanvas->mViewStart, mCanvas->mViewEnd, 0, mWidth);
         DrawTriangle(quantizedX - 12, 1);
         DrawTriangle(quantizedX + 12, -1);
      }
      else
      {
         ofSetColor(150, 150, 150);
         ofNoFill();
         float quantizedClick = GetQuantizedForX(mModuleClickMousePos.x, HoverMode::kNone);
         float quantizedNow = GetQuantizedForX(mMousePos.x, HoverMode::kNone);
         float quantizedXClick = ofMap(quantizedClick, mCanvas->mViewStart, mCanvas->mViewEnd, 0, mWidth);
         float quantizedXNow = ofMap(quantizedNow, mCanvas->mViewStart, mCanvas->mViewEnd, 0, mWidth);
         DrawTriangle(MIN(quantizedXClick - 12, quantizedXNow - 12), 1);
         DrawTriangle(MAX(quantizedXClick + 12, quantizedXNow + 12), -1);
         ofFill();
         ofSetColor(mHighlightColour);
         ofRect(MIN(quantizedXClick, quantizedXNow), 0, MAX(quantizedXClick, quantizedXNow) - MIN(quantizedXClick, quantizedXNow), mHeight / 2, 0);
      }
   }
   ofPopStyle();

   ofPopMatrix();
}

void CanvasTimeline::DrawTriangle(float posX, int direction)
{
   ofBeginShape();
   ofVertex(posX, 0);
   ofVertex(posX, mHeight);
   ofVertex(posX + mHeight * direction, 0);
   ofVertex(posX, 0);
   ofEndShape();
}

float CanvasTimeline::GetQuantizedForX(float posX, HoverMode clampSide)
{
   float pos = ((posX / mWidth) * (mCanvas->mViewEnd - mCanvas->mViewStart)) + mCanvas->mViewStart;
   bool control = GetKeyModifiers() & kModifier_Command;
   if (mDisableControlSnapping)
      control = false;
   if (!control)
   {
      int measure = CLAMP(int(pos + .5f), 0, mCanvas->GetLength());
      if (clampSide == HoverMode::kStart)
      {
         if (measure >= mCanvas->mLoopEnd)
            measure = mCanvas->mLoopEnd - 1;
      }
      if (clampSide == HoverMode::kEnd)
      {
         if (measure <= mCanvas->mLoopStart)
            measure = mCanvas->mLoopStart + 1;
      }
         return measure;
   }
   else
   {
      float cPerM = mCanvas->GetNumCols()/mCanvas->GetLength();
      pos = round(pos*(cPerM))/cPerM;
      float interval = CLAMP(pos, 0, mCanvas->GetLength());
      if (clampSide == HoverMode::kStart)
      {
         if (interval >= mCanvas->mLoopEnd)
            interval = mCanvas->mLoopEnd - 1;
      }
      if (clampSide == HoverMode::kEnd)
      {
         if (interval <= mCanvas->mLoopStart)
            interval = mCanvas->mLoopStart + 1;
      }
      return interval;
   }
}

void CanvasTimeline::OnClicked(float x, float y, bool right)
{
   mClickMousePos.set(TheSynth->GetRawMouseX(), TheSynth->GetRawMouseY());
   mModuleClickMousePos = ofVec2f(x, y);
   mDragOffset.set(0, 0);
   mHideShiftTemp = false;
   if (GetKeyModifiers() & kModifier_Shift)
      mShiftOverride = true;
   else
      mClick = true;
}

void CanvasTimeline::MouseReleased()
{
   if (mClick)
   {
      float loopLength = mCanvas->mLoopEnd - mCanvas->mLoopStart;
      if (mHoverMode == HoverMode::kStart || mHoverMode == HoverMode::kMiddle)
      {
         float startX = ofMap(mCanvas->mLoopStart, mCanvas->mViewStart, mCanvas->mViewEnd, 0, mWidth);
         startX += mDragOffset.x;
         float quantized = GetQuantizedForX(startX, mHoverMode);
         mCanvas->mLoopStart = quantized;
      }
      if (mHoverMode == HoverMode::kEnd || mHoverMode == HoverMode::kMiddle)
      {
         float endX = ofMap(mCanvas->mLoopEnd, mCanvas->mViewStart, mCanvas->mViewEnd, 0, mWidth);
         endX += mDragOffset.x;
         float quantized = GetQuantizedForX(endX, mHoverMode);
         mCanvas->mLoopEnd = quantized;
      }

      if (mHoverMode == HoverMode::kMiddle)
      {
         mCanvas->mLoopStart = ofClamp(mCanvas->mLoopStart, 0, mCanvas->GetLength() - loopLength);
         mCanvas->mLoopEnd = ofClamp(mCanvas->mLoopEnd, loopLength, mCanvas->GetLength());
      }
      if (mListener)
         mListener->UserUpdatedCanvasTimeline(mCanvas->mLoopStart, mCanvas->mLoopEnd);
   }
   if (mShiftOverride)
   {
      float quantizedClick = GetQuantizedForX(mModuleClickMousePos.x, HoverMode::kNone);
      float quantizedNow = GetQuantizedForX(mMousePos.x, HoverMode::kNone);

      float minValue = MIN(quantizedNow, quantizedClick);
      float maxValue = MAX(quantizedNow, quantizedClick);

      if (minValue == maxValue)
      {
         mCanvas->mLoopStart = 0;
         mCanvas->mLoopEnd = mCanvas->GetLength();
      }
      else
      {
         mCanvas->mLoopStart = minValue;
         mCanvas->mLoopEnd = maxValue;
      }
      mHideShiftTemp = true; //Used to stop the panel flickering slightly after completing the operation.
      if (mListener)
         mListener->UserUpdatedCanvasTimeline(mCanvas->mLoopStart, mCanvas->mLoopEnd);
   }
   mShiftOverride = false;
   mClick = false;
}

bool CanvasTimeline::MouseMoved(float x, float y)
{
   CheckHover(x, y);
   mMousePos = ofVec2f(x, y);

   if (!mClick)
   {
      mHoverMode = HoverMode::kNone;

      float startX = ofMap(mCanvas->mLoopStart, mCanvas->mViewStart, mCanvas->mViewEnd, 0, mWidth);
      float endX = ofMap(mCanvas->mLoopEnd, mCanvas->mViewStart, mCanvas->mViewEnd, 0, mWidth);

      ofRectangle betweenRect(startX, 0, endX - startX, mHeight);
      if (betweenRect.contains(x, y))
         mHoverMode = HoverMode::kMiddle;

      ofRectangle loopStartRect(startX, 0, mHeight, mHeight);
      if (loopStartRect.contains(x, y))
         mHoverMode = HoverMode::kStart;

      ofRectangle loopEndRect(endX - mHeight, 0, mHeight, mHeight);
      if (loopEndRect.contains(x, y))
         mHoverMode = HoverMode::kEnd;
   }
   else
   {
      mDragOffset = (ofVec2f(TheSynth->GetRawMouseX(), TheSynth->GetRawMouseY()) - mClickMousePos) / gDrawScale;
   }

   return false;
}

bool CanvasTimeline::MouseScrolled(float x, float y, float scrollX, float scrollY, bool isSmoothScroll, bool isInvertedScroll)
{
   return false;
}

void CanvasTimeline::SaveState(FileStreamOut& out)
{
}

void CanvasTimeline::LoadState(FileStreamIn& in, bool shouldSetValue)
{
}
