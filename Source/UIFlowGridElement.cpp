#include "FlowGrid.h"


FlowGridElement::FlowGridElement(ofColor baseColor,float preferredWidth)
{
   mPreferredWidth = preferredWidth;
   mWidth = preferredWidth;
   SetColor(baseColor);
}
FlowGridElement::~FlowGridElement()
{
}
void FlowGridElement::SetPreferredPosition(int row, float positionPercent)
{
   throw std::logic_error("Not implemented");//todo
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
void FlowGridElement::MouseMove(float x, float y)
{
}

void FlowGridElement::Draw()
{
   ofPushMatrix();
   ofTranslate(mX, mY);
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
   ofPopMatrix();
}
