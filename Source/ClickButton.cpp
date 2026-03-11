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
//  ClickButton.cpp
//  modularSynth
//
//  Created by Ryan Challinor on 12/4/12.
//
//

#include "ClickButton.h"
#include "IDrawableModule.h"
#include "SynthGlobals.h"

#include <cstring>

#include "PatchCableSource.h"

ClickButton::ClickButton(IButtonListener* owner, const char* label, int x, int y, ButtonDisplayStyle displayStyle /*= ButtonDisplayStyle::kText*/)
: mOwner(owner)
, mDisplayStyle(displayStyle)
{
   assert(owner);
   SetLabel(label);
   SetPosition(x, y);
   (dynamic_cast<IDrawableModule*>(owner))->AddUIControl(this);
   SetParent(dynamic_cast<IClickable*>(owner));
   SetShouldSaveState(false);
}

ClickButton::ClickButton(IButtonListener* owner, const char* label, IUIControl* anchor, AnchorDirection anchorDirection, ButtonDisplayStyle displayStyle /*= ButtonDisplayStyle::kText*/)
: ClickButton(owner, label, -1, -1, displayStyle)
{
   PositionTo(anchor, anchorDirection);
}

ClickButton::~ClickButton()
{
}

void ClickButton::SetLabel(const char* label)
{
   SetName(label);
   UpdateWidth();
}

void ClickButton::UpdateWidth()
{
   if (mDisplayStyle == ButtonDisplayStyle::kText || mDisplayStyle == ButtonDisplayStyle::kSampleIcon || mDisplayStyle == ButtonDisplayStyle::kFolderIcon)
   {
      mWidth = GetStringWidth(GetDisplayName()) + 3 + .25f * strnlen(GetDisplayName().c_str(), 50);
      if (mDisplayStyle == ButtonDisplayStyle::kSampleIcon || mDisplayStyle == ButtonDisplayStyle::kFolderIcon)
         mWidth += 20;
   }
}

void ClickButton::Render()
{
   ofPushStyle();

   float w, h;
   GetDimensions(w, h);

   ofColor color, textColor;
   IUIControl::GetColors(color, textColor);
   ofFill();
   ofSetColor(0, 0, 0, gModuleDrawAlpha * .5f);
   ofRect(mX + 1, mY + 1, w, h);
   DrawBeacon(mX + w / 2, mY + h / 2);
   float press = ofClamp((1 - (gTime - mClickTime) / 200), 0, 1);
   color.r = ofLerp(color.r, 0, press);
   color.g = ofLerp(color.g, 0, press);
   color.b = ofLerp(color.b, 0, press);
   if (!mEnabled)
   {
      color.a = 130;
      textColor.a = 130;
   }
   ofSetColor(color);
   ofRect(mX, mY, w, h);

   float iX = mX + w * mIconOffsetMulX + mIconOffsetX;
   float iY = mY + h * mIconOffsetMulY + mIconOffsetY;

   if (mDisplayStyle == ButtonDisplayStyle::kText)
   {
      ofSetColor(textColor);
      DrawTextNormal(GetDisplayName(), iX + 2, iY + 12);
   }
   else if (mDisplayStyle == ButtonDisplayStyle::kPlay)
   {
      ofSetColor(textColor);
      ofFill();
      ofTriangle(iX + 5, iY + 2, iX + 5, iY + 12, iX + 15, iY + 7);
   }
   else if (mDisplayStyle == ButtonDisplayStyle::kPause)
   {
      ofSetColor(textColor);
      ofFill();
      ofRect(iX + 5, iY + 2, 4, 10, 0);
      ofRect(iX + 11, iY + 2, 4, 10, 0);
   }
   else if (mDisplayStyle == ButtonDisplayStyle::kStop)
   {
      ofSetColor(textColor);
      ofFill();
      ofRect(iX + 5, iY + 2, 10, 10, 0);
   }
   else if (mDisplayStyle == ButtonDisplayStyle::kGrabSample)
   {
      ofSetColor(textColor);
      for (int i = 0; i < 5; ++i)
      {
         float height = (i % 2 == 0) ? 6 : 10;
         float x = iX + 4 + i * 3;
         ofLine(x, iY + 7 - height / 2, x, iY + 7 + height / 2);
      }
   }
   else if (mDisplayStyle == ButtonDisplayStyle::kSampleIcon)
   {
      ofSetColor(textColor);
      for (int i = 0; i < 5; ++i)
      {
         float height = (i % 2 == 0) ? 6 : 10;
         float x = iX + 4 + i * 3;
         ofLine(x, iY + 7 - height / 2, x, iY + 7 + height / 2);
      }
      DrawTextNormal(GetDisplayName(), iX + 22, iY + 12);
   }
   else if (mDisplayStyle == ButtonDisplayStyle::kFolderIcon)
   {
      ofSetColor(textColor);
      ofFill();
      ofRect(iX + 2, iY + 2, 7, 5, 2);
      ofRect(iX + 2, iY + 4, 16, 9, 2);
      DrawTextNormal(GetDisplayName(), iX + 22, iY + 12);
   }
   else if (mDisplayStyle == ButtonDisplayStyle::kArrowRight)
   {
      ofSetColor(textColor);
      ofLine(iX + 6, iY + 3, iX + 14, iY + 7);
      ofLine(iX + 6, iY + 11, iX + 14, iY + 7);
   }
   else if (mDisplayStyle == ButtonDisplayStyle::kArrowLeft)
   {
      ofSetColor(textColor);
      ofLine(iX + 14, iY + 3, iX + 6, iY + 7);
      ofLine(iX + 14, iY + 11, iX + 6, iY + 7);
   }
   else if (mDisplayStyle == ButtonDisplayStyle::kPlus)
   {
      ofSetColor(textColor);
      ofSetLineWidth(1.5f);
      ofLine(iX + 10, iY + 3, iX + 10, iY + 12);
      ofLine(iX + 6, iY + 7.5f, iX + 14, iY + 7.5f);
   }
   else if (mDisplayStyle == ButtonDisplayStyle::kMinus)
   {
      ofSetColor(textColor);
      ofSetLineWidth(1.5f);
      ofLine(iX + 6, iY + 7.5f, iX + 14, iY + 7.5f);
   }
   else if (mDisplayStyle == ButtonDisplayStyle::kHamburger)
   {
      ofSetColor(textColor);
      ofSetLineWidth(1.0f);
      ofLine(iX + 6, iY + 4.5f, iX + 14, iY + 4.5f);
      ofLine(iX + 6, iY + 7.5f, iX + 14, iY + 7.5f);
      ofLine(iX + 6, iY + 10.5f, iX + 14, iY + 10.5f);
   }

   ofPopStyle();

   DrawHover(mX, mY, w, h);
}

bool ClickButton::ButtonLit() const
{
   return mClickTime + 200 > gTime;
}

bool ClickButton::CanBeTargetedBy(PatchCableSource* source) const
{
   if (source->GetConnectionType() == kConnectionType_Pulse)
      return true;
   return IUIControl::CanBeTargetedBy(source);
}

void ClickButton::OnPulse(double time, float velocity, int flags)
{
   if (velocity > 0)
      DoClick(time);
}

void ClickButton::OnClicked(float x, float y, bool right)
{
   if (right)
      return;

   DoClick(NextBufferTime(false));
}

void ClickButton::DoClick(double time)
{
   mClickTime = time;
   if (mEnabled)
      mOwner->ButtonClicked(this, time);
}

void ClickButton::MouseReleased()
{
   mClickTime = 0;
}

bool ClickButton::MouseMoved(float x, float y)
{
   CheckHover(x, y);
   return false;
}

void ClickButton::SetIconAlignment(ButtonIconAlignment alignment)
{
   //Generally icons have bounds in the x20 y20 range
   switch (alignment)
   {
      case ButtonIconAlignment::kTopLeft: //<--Default
         mIconOffsetX = 0;
         mIconOffsetY = 0;
         mIconOffsetMulX = 0;
         mIconOffsetMulY = 0;
         break;
      case ButtonIconAlignment::kTopMiddle:
         mIconOffsetX = -10;
         mIconOffsetY = 0;
         mIconOffsetMulX = 0.5f;
         mIconOffsetMulY = 0;
         break;
      case ButtonIconAlignment::kTopRight:
         mIconOffsetX = -20;
         mIconOffsetY = 0;
         mIconOffsetMulX = 1;
         mIconOffsetMulY = 0;
         break;
      case ButtonIconAlignment::kCenterLeft:
         mIconOffsetX = 0;
         mIconOffsetY = -10;
         mIconOffsetMulX = 0;
         mIconOffsetMulY = 0.5f;
         break;
      case ButtonIconAlignment::kCenter:
         mIconOffsetX = -10;
         mIconOffsetY = -10;
         mIconOffsetMulX = 0.5f;
         mIconOffsetMulY = 0.5f;
         break;
      case ButtonIconAlignment::kCenterRight:
         mIconOffsetX = -20;
         mIconOffsetY = -10;
         mIconOffsetMulX = 1;
         mIconOffsetMulY = 0.5f;
         break;
      case ButtonIconAlignment::kBottomLeft:
         mIconOffsetX = 0;
         mIconOffsetY = -20;
         mIconOffsetMulX = 0;
         mIconOffsetMulY = 1;
         break;
      case ButtonIconAlignment::kBottomMid:
         mIconOffsetX = -10;
         mIconOffsetY = -20;
         mIconOffsetMulX = 0.5f;
         mIconOffsetMulY = 1;
         break;
      case ButtonIconAlignment::kBottomRight:
         mIconOffsetX = -20;
         mIconOffsetY = -20;
         mIconOffsetMulX = 1;
         mIconOffsetMulY = 1;
         break;
      default:;
   }
}
void ClickButton::SetFromMidiCC(float slider, double time, bool setViaModulator)
{
   if (slider > 0)
      DoClick(time);
   else
      MouseReleased();
}

void ClickButton::SetValue(float value, double time, bool forceUpdate /*= false*/)
{
   if (value > 0)
      DoClick(time);
   else
      MouseReleased();
}

float ClickButton::GetMidiValue() const
{
   if (ButtonLit())
      return 1;
   return 0;
}

std::string ClickButton::GetDisplayValue(float val) const
{
   if (val > 0)
      return "click";
   return "_";
}

void ClickButton::Increment(float amount)
{
   DoClick(NextBufferTime(false));
}
