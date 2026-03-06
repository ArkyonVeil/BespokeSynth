#include "UIFlowGrid.h"


UIFlowGridElement::UIFlowGridElement(float preferredWidth, ofColor baseColor)
{
   mPreferredWidth = preferredWidth;
   mWidth = preferredWidth;
   SetColor(baseColor);
}
UIFlowGridElement::~UIFlowGridElement()
{
}
void UIFlowGridElement::SetPreferredPosition(int row, float positionPercent)
{
}
void UIFlowGridElement::SetColor(ofColor color)
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

void UIFlowGridElement::SetColorsManually(ofColor mainColor, ofColor outlineColor, ofColor highlightColor, ofColor highlightOutlineColor)
{
   mMainColor = mainColor;
   mOutlineColor = outlineColor;
   mHighlightColor = highlightColor;
   mHighlightOutlineColor = highlightOutlineColor;
}

ofVec2f UIFlowGridElement::GetRelativePosition()
{
   auto pos = mFlowGridParent->GetPosition(true);
   return ofVec2f(pos.x + mX, pos.y + mY);
}
void UIFlowGridElement::MouseMove(float x, float y)
{
}

void UIFlowGridElement::Draw()
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
