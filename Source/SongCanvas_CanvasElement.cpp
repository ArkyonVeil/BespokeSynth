#include "SongCanvas_CanvasElement.h"
#include "CanvasControls.h"
#include "CanvasElement.h"
#include "SongCanvas.h"

SongCanvas_CanvasElement::SongCanvas_CanvasElement(Canvas* canvas, int col, int row, float offset, float length)
: CanvasElement(canvas, col, row, offset, length)
{
   //canvas->SetZoomSpeed(10);
   mLength *= 4;
   SongCanvas* seq = static_cast<SongCanvas*>(canvas->GetListener());
   seq->SetupCanvasElement(this);
}

void SongCanvas_CanvasElement::Setup(SongCanvasRackElement* templateElement)
{
   mElementVariant = templateElement->mVariantType;
   mName = templateElement->GetName();
   mNameCache = *mName;
   mRackParent = templateElement;
   mRackParentID = mRackParent->mInternalRackID;
   switch (templateElement->mVariantType)
   {
      case SongCanvasElementVariant::Enabler:
      {
         mCurrentColor = mEnablerColor;
         mCurrentColorGrad = ofColor(
         MAX(0, mCurrentColor.r - 30),
         MAX(0, mCurrentColor.g - 30),
         MAX(0, mCurrentColor.b - 30));
      }
      break;
      case SongCanvasElementVariant::Pulser:
      {
         mCurrentColor = mPulserColor;
         mCurrentColorGrad = ofColor(
         MAX(0, mCurrentColor.r - 90),
         MAX(0, mCurrentColor.g - 90),
         MAX(0, mCurrentColor.b - 90));
      }
      break;
      case SongCanvasElementVariant::LFO:
      {
         mCurrentColor = mLFOColor;
      }
      break;
      case SongCanvasElementVariant::Sampler:
      {
         mCurrentColor = mSamplerColor;
      }
      break;
      case SongCanvasElementVariant::OnePulse:
      {
         mCurrentColor = mOnePulseColor;
         mTextDrawXOffset = 4;
         mCurrentColorGrad = ofColor(
         MAX(0, mCurrentColor.r - 30),
         MAX(0, mCurrentColor.g - 30),
         MAX(0, mCurrentColor.b - 30));
      }
      break;
      default:;
   }
}

CanvasElement* SongCanvas_CanvasElement::CreateDuplicate() const
{
   SongCanvas_CanvasElement* element = new SongCanvas_CanvasElement(mCanvas, mCol, mRow, mOffset, mLength / 4);
   //element->mVelocity = mVelocity;
   //element->mVoiceIdx = mVoiceIdx;
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
      ofSetColorGradient(mCurrentColorGrad, mCurrentColor, ofVec2f(ofLerp(rect.getMinX(), rect.getMaxX(), .5f), rect.y), ofVec2f(rect.getMaxX(), rect.y));
      //ofSetColor(mCurrentColor);
      ofRect(rect, 0);

      switch (mElementVariant)
      {
         case SongCanvasElementVariant::Enabler:
         {
         }
         break;
         case SongCanvasElementVariant::Pulser:
         {
         }
         break;
         case SongCanvasElementVariant::LFO:
         {
         }
         break;
         case SongCanvasElementVariant::Sampler:
         {
         }
         break;
         case SongCanvasElementVariant::OnePulse:
            ofRectangle seamRect = rect;
            seamRect.width = MIN(rect.x, 3);
            ofSetColor(ofColor(180, 180, 0)); //Draw a seam.
            ofRect(seamRect, 0);
            break;
      }

      ofSetColor(ofColor::white);

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


      DrawTextNormal(mDisplayName, rect.x + mTextDrawXOffset, rect.y + 9, 9);
   }


   ofPopStyle();
}

namespace
{
   const int kNCESaveStateRev = 0;
}

void SongCanvas_CanvasElement::SaveState(FileStreamOut& out)
{
   CanvasElement::SaveState(out);

   //out << kNCESaveStateRev;

   //out << mVelocity;
}

void SongCanvas_CanvasElement::LoadState(FileStreamIn& in)
{
   CanvasElement::LoadState(in);

   //int rev;
   //in >> rev;
   //LoadStateValidate(rev <= kNCESaveStateRev);

   //in >> mVelocity;
}