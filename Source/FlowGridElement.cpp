#include "FlowGrid.h"
#include "ModularSynth.h"


FlowGridElement::FlowGridElement(FlowGrid* grid)
{
   mFlowGridParent = grid;
}
FlowGridElement::~FlowGridElement()
{
   delete NameData;
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

void FlowGridElement::OnMouseClick(bool rightClick)
{
   mFlowGridParent->SetSelectedGridElement(this);
}

bool FlowGridElement::MouseMoved(float x, float y)
{
   return false;
}
void FlowGridElement::MouseReleased()
{

}
void FlowGridElement::DrawModule()
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
