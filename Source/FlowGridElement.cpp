#include "FlowGrid.h"
#include "ModularSynth.h"

FlowGridElement::FlowGridElement(FlowGrid* grid)
{
   mFlowGridParent = grid;
   SetShouldDrawOutline(false);
}
FlowGridElement::~FlowGridElement()
{
   delete NameData;
}
void FlowGridElement::Render()
{//Simplified rendering code for higher performance (and less jank)
   if (!mShowing)
      return;

   float w, h;
   GetDimensions(w, h);

   ofPushMatrix();
   ofPushStyle();

   ofTranslate(mX, mY, 0);

   DrawModule();
  // DrawFrame(w, h, true, titleBarHeight, highlight);

   ofFill();

   ofPopMatrix();
   ofPopStyle();

}
void FlowGridElement::SetRect(ofRectangle rect)
{
   mWidth = rect.width;
   mHeight = rect.height;
   mX = rect.x;
   mY = rect.y;
}
void FlowGridElement::SetColor(ofColor color)
{
   mMainColor = color;
   mMainColor.a = 20;

   mHighlightColor = color;
   mHighlightColor.a = 50;

   mOutlineColor = color;
   mOutlineColor.a = 100;

   mHighlightOutlineColor = color;
   mHighlightOutlineColor.a = 130;
}
void FlowGridElement::SetColorOutline(ofColor color)
{
   mOutlineColor = color;
   mOutlineColor.a = 100;

   mHighlightOutlineColor = color;
   mHighlightOutlineColor.a = 130;
}

void FlowGridElement::SetColorsManually(ofColor mainColor, ofColor outlineColor, ofColor highlightColor, ofColor highlightOutlineColor)
{
   mMainColor = mainColor;
   mOutlineColor = outlineColor;
   mHighlightColor = highlightColor;
   mHighlightOutlineColor = highlightOutlineColor;
}

ofVec2f FlowGridElement::GetRelativePosition()
{
   auto pos = mFlowGridParent->GetPosition(true);
   return ofVec2f(pos.x + mX, pos.y + mY);
}

void FlowGridElement::OnMouseClick(bool rightClick)
{
   mFlowGridParent->SetSelectedGridElement(this);
}

bool FlowGridElement::MouseMoved(float x, float y)
{
   IDrawableModule::MouseMoved(x, y);
   if (gHoveredModule == this)
   {
      mHovered = true;
   }
   else
   {
      mHovered = false;
   }
   return false;
}
void FlowGridElement::MouseReleased()
{

}
void FlowGridElement::DrawModule()
{
   ofPushStyle();

   ofFill();
   if (!mHighlighted)
   {
      ofSetColor(mMainColor);
   }
   else
   {
      ofSetColor(mHighlightColor);
   }
   ofRect(0, 0, mWidth, mHeight);

   if (!mHighlighted)
      ofSetLineWidth(mOutlineThickness);
   else
      ofSetLineWidth(mOutlineThickness + 0.4F);
   ofNoFill();

   if (!mHovered && !mHighlighted)
      ofSetColor(mOutlineColor);
   else if (!mHovered && mHighlighted)
   {
      ofSetColor(mHighlightOutlineColor);
   }
   else
   {
      ofSetColor(0, 255, 255);
   }
   //ofFill();
   ofRect(0, 0, mWidth, mHeight);

   ofPopStyle();
}
