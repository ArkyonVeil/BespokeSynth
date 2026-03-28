#include "FlowGrid.h"
#include "ModularSynth.h"
#include "PatchCableSource.h"

FlowGridElement::FlowGridElement(FlowGrid* grid, std::string elementTypeName)
{
   mElementTypeName = elementTypeName;
   mFlowGridParent = grid;
}
FlowGridElement::~FlowGridElement()
{
}
void FlowGridElement::CreateUIControls()
{
   IDrawableModule::CreateUIControls();
}
void FlowGridElement::Init()
{
   IDrawableModule::Init();
}
void FlowGridElement::Render()
{
   if (!mShowing)
      return;

   ofPushMatrix();
   ofPushStyle();

   ofTranslate(mX, mY, 0);

   DrawModule();

   ofFill();

   ofPopMatrix();
   ofPopStyle();
   /*
   for (auto source : GetPatchCableSources())
   {
      source->UpdatePosition(false);
      source->DrawSource();
   }*/
}
ofRectangle FlowGridElement::GetRectRelativeToGrid() const
{
   return ofRectangle(mX - mFlowGridParent->GetPosition().x, mY - mFlowGridParent->GetPosition().y, mWidth, mHeight);
}
void FlowGridElement::SetRect(ofRectangle rect)
{
   mWidth = rect.width;
   mHeight = rect.height;
   mX = rect.x;
   mY = rect.y;
}

void FlowGridElement::SetRectRelativeToGrid(ofRectangle rect) //Sets rect and offsets mX/mY based on the FlowGrid's position.
{
   mWidth = rect.width;
   mHeight = rect.height;
   mX = rect.x + mFlowGridParent->GetPosition().x;
   mY = rect.y + mFlowGridParent->GetPosition().y;
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
   auto pos = mFlowGridParent->GetPosition();
   return ofVec2f(pos.x + mX, pos.y + mY);
}

void FlowGridElement::OnClicked(float x, float y, bool right)
{
   mFlowGridParent->SetSelectedGridElement(this);
}
bool FlowGridElement::MouseMoved(float x, float y)
{
   IDrawableModule::MouseMoved(x, y);
   //x=x-mFlowGridParent->GetPosition(true).x;
   //y=y-mFlowGridParent->GetPosition(true).y;
   mDebugNumX = x;
   mDebugNumY = y;

   if (GetRectLocal().contains(x, y))
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
   IDrawableModule::MouseReleased();
}
void FlowGridElement::DrawModule()
{
   ofPushStyle();
   ofFill();

   mSelected = mFlowGridParent->GetSelectedGridElement() == this;

   if (!mSelected)
   {
      if (!mHovered)
         ofSetColor(mMainColor);
      else
         ofSetColor(ofColor::lerp(mMainColor,ofColor::white,0.075f));
   }
   else
   {
      ofSetColor(mHighlightColor);
   }
   ofRect(0, 0, mWidth, mHeight);

   if (!mSelected || !mHovered)
      ofSetLineWidth(mOutlineThickness);
   else
      ofSetLineWidth(mOutlineThickness + 0.4F);
   ofNoFill();

   if (!mHovered && !mSelected)
      ofSetColor(mOutlineColor);
   else if (!mHovered && mSelected)
   {
      ofSetColor(0, 255, 255);
   }
   else
   {
      ofSetColor(mHighlightOutlineColor);
   }
   //ofFill();
   ofRect(0, 0, mWidth, mHeight);
   /* DEBUG TEXT
   //DrawTextNormal(ofToString(mDebugNum),2,15);
   if (gHoveredSubModule != nullptr)
      DrawTextNormal(ofToString(gHoveredSubModule->GetDisplayName()), 2, 15);
   //DrawTextNormal(ofToString((int)mDebugNumX)+" | "+ofToString((int)mDebugNumY),2,15);*/
   ofPopStyle();
}
void FlowGridElement::UpdateRow()
{
   mFlowGridParent->UpdateRow(mFlowGridParent->GetRowIndexOfElement(this), true);
}
