#include "SongCanvas_CanvasElement.h"
#include "CanvasElement.h"
#include "SongCanvas.h"

SongCanvas_CanvasElement::SongCanvas_CanvasElement(Canvas* canvas, int col, int row, float offset, float length)
: CanvasElement(canvas, col, row, offset, length)
{
   mLength *= 4;
   mSongCanvas = static_cast<SongCanvas*>(canvas->GetListener());
   mSongCanvas->SetupCanvasElement(this);
}
SongCanvas_CanvasElement::SongCanvas_CanvasElement(Canvas* canvas)
: CanvasElement(canvas, 0, 0, 0, 4)
{
   mLength *= 4;
   mSongCanvas = static_cast<SongCanvas*>(canvas->GetListener());
}

void SongCanvas_CanvasElement::SetupBase(SongCanvasRackElement* templateElement)
{
   mName = templateElement->GetName();
   mNameCache = *mName;
   mRackPart = templateElement;
   mRackParentID = mRackPart->mInternalRackID;
   mRackPart->SetupCanvasPart(this);
   mIndex = mSongCanvas->GetNewCanvasElementId();
}

CanvasElement* SongCanvas_CanvasElement::CreateDuplicate() const
{
   SongCanvas_CanvasElement* element = new SongCanvas_CanvasElement(mCanvas, mCol, mRow, mOffset, mLength / 4);
   return element;
}

void SongCanvas_CanvasElement::DrawContents(bool clamp, bool wrapped, ofVec2f offset)
{
   ofPushStyle();
   ofFill();

   ofRectangle rect = GetRect(clamp, wrapped, offset);
   float fullHeight = rect.height;
   rect.height *= 0.95;
   rect.y += (fullHeight - rect.height) * .5f;
   if (rect.width > 0)
   {
      float addedTextYOffset = 0;
      bool isActive = mSongCanvas->IsEnabled() && mSongCanvas->IsLayerActive(mRow);
      int eDiv = 1; //Enabled Divisor
      if (isActive)
         ofSetColorGradient(mCurrentColorGrad, mCurrentColor, ofVec2f(rect.width / 2, rect.y + rect.height * 0.66), ofVec2f(rect.width / 2, rect.y + rect.height));
      else
      {
         eDiv = 2;
         auto colA1 = ofColor(mCurrentColor.r / 2, mCurrentColor.g / 2, mCurrentColor.b / 2);
         auto colA2 = ofColor(mCurrentColorGrad.r / 2, mCurrentColorGrad.g / 2, mCurrentColorGrad.b / 2);
         ofSetColorGradient(colA2, colA1, ofVec2f(ofLerp(rect.getMinX(), rect.getMaxX(), .5f), rect.y), ofVec2f(rect.getMaxX(), rect.y));
      }
      //ofSetColor(mCurrentColor);
      ofRect(rect, 2);

      //Draw unique rack based graphics for the canvas element
      mRackPart->DrawCanvasPartGraphics(this, rect);

      if (isActive)
         ofSetColor(ofColor::white);
      else
         ofSetColor(ofColor(125, 125, 125));


      //If the name differs, redo the size calcs.
      if (mNameCache != *mName)
      {
         mNameCache = *mName;
         mCachedNameSize = -1;
      }
      if (mCachedNameSize != rect.width)
      {
         float maxTextSize = rect.width - 8;
         mDisplayName = mNameCache;
         short alt = 0;

         while (GetStringWidth(mDisplayName, 9) > maxTextSize)
         {
            alt = 1;
            if (mDisplayName.find("Part ") != std::string::npos)
            {
               mDisplayName.erase(0, 5);
               if (rect.width > 18)
                  alt = 2;
               else
                  alt = 0;
               break;
            }
            mDisplayName.resize(mDisplayName.size() - 1);
            if (mDisplayName.empty())
            {
               mDisplayName = "";
               break;
            }
         }
         if (alt == 1)
            mDisplayName += "...";
         else if (alt == 2)
            mDisplayName = "..." + mDisplayName;
         if (rect.width < 12)
         {
            mDisplayName = "";
         }
         mCachedNameSize = rect.width;
      }


      DrawTextNormal(mDisplayName, rect.x + mTextDrawXOffset, rect.y + 9 + addedTextYOffset, 9);
   }


   ofPopStyle();
}

void SongCanvas_CanvasElement::SaveState(FileStreamOut& out)
{
   out << mRow;
   out << mCol;
   out << mLength;
   out << GetStart();
   out << GetEnd();
   out << mRackParentID;
   mRackPart->SaveCanvasPart(this, out);
}

void SongCanvas_CanvasElement::LoadState(FileStreamIn& in)
{
   in >> mRow;
   in >> mCol;
   in >> mLength;
   float val;
   in >> val;
   SetStart(val, true);
   in >> val;
   SetEnd(val);
   in >> mRackParentID;
   SetupBase(mSongCanvas->GetRackPartWithID(mRackParentID));
   mRackPart->LoadCanvasPart(this, in, mRackPart->GetModuleSaveStateRev());
}
bool SongCanvas_CanvasElement::UseCustomPosQuantization() {return mRackPart->UseCustomCanvasElementQuantization();}
float SongCanvas_CanvasElement::GetCustomPosQuantization(float input, int context) {return mRackPart->GetCustomCanvasElementQuantization(this,input,context);}