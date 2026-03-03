#pragma once
#include "CanvasElement.h"


enum class SongSequencerElementVariant;
class SongSequencerRackElement;
class SongSequencerCanvasElement : public CanvasElement
{
public:
   SongSequencerCanvasElement(Canvas* canvas, int col, int row, float offset, float length);
   void Setup(SongSequencerRackElement* templateElement);
   static CanvasElement* Create(Canvas* canvas, int col, int row) { return new SongSequencerCanvasElement(canvas, col, row, 0, 1); }
   
   CanvasElement* CreateDuplicate() const override;

   SongSequencerRackElement*  GetRackElement() {return mRackParent;}
   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in) override;

   int GetRackElementId() const { return mRackParentID;}
   //float GetMinLength() override { return 0.25f; }

   SongSequencerElementVariant GetVariantType(){return mElementVariant;}
   
private:
   void DrawContents(bool clamp, bool wrapped, ofVec2f offset) override;
   
   SongSequencerElementVariant mElementVariant;

   SongSequencerRackElement* mRackParent;
   int mRackParentID;
   std::string* mName;
   std::string mNameCache;
   std::string mDisplayName;
   float mCachedNameSize = -1;
   float mTextDrawXOffset = 2;

   ofColor mCurrentColor;
   ofColor mCurrentColorGrad;
   const ofColor mEnablerColor = ofColor(150,150,150);
   const ofColor mPulserColor = ofColor(180,180,0);
   const ofColor mLFOColor = ofColor::purple;
   const ofColor mSamplerColor = ofColor::green;
   const ofColor mOnePulseColor = ofColor(100,100,0);
};
