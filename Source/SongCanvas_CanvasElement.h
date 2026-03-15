#pragma once
#include "CanvasElement.h"


class SongCanvas;
enum class SongCanvasElementVariant;
class SongCanvasRackElement;
class SongCanvas_CanvasElement : public CanvasElement
{
public:
   SongCanvas_CanvasElement(Canvas* canvas, int col, int row, float offset, float length);
   void Setup(SongCanvasRackElement* templateElement);
   static CanvasElement* Create(Canvas* canvas, int col, int row) { return new SongCanvas_CanvasElement(canvas, col, row, 0, 1); }

   CanvasElement* CreateDuplicate() const override;

   SongCanvasRackElement* GetRackElement() { return mRackParent; }
   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in) override;

   int GetRackElementId() const { return mRackParentID; }
   //float GetMinLength() override { return 0.25f; }

   SongCanvasElementVariant GetVariantType() { return mElementVariant; }

private:
   void DrawContents(bool clamp, bool wrapped, ofVec2f offset) override;

   SongCanvasElementVariant mElementVariant;

   SongCanvasRackElement* mRackParent;
   int mRackParentID;
   std::string* mName;
   std::string mNameCache;
   std::string mDisplayName;
   float mCachedNameSize = -1;
   float mTextDrawXOffset = 2;
   SongCanvas* mSongCanvas;

   ofColor mCurrentColor;
   ofColor mCurrentColorGrad;
   const ofColor mEnablerColor = ofColor(180, 180, 180);
   const ofColor mEnablerColor2 = ofColor(100, 100, 100);
   const ofColor mEnablerInvertColor = ofColor(100, 100, 100);
   const ofColor mEnablerInvertColor2 = ofColor(50, 50, 50);
   const ofColor mPulserColor = ofColor(180, 180, 0);
   const ofColor mLFOColor = ofColor::purple;
   const ofColor mSamplerColor = ofColor(40,180,40);
   const ofColor mSamplerColor2 = ofColor(20,70,20);
   const ofColor mOnePulseColor = ofColor(100, 100, 0);
};
