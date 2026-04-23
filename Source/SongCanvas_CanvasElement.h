#pragma once
#include "CanvasElement.h"

class SongCanvas;
class SongCanvasRackElement;
class SongCanvas_CanvasElement : public CanvasElement
{
public:
   SongCanvas_CanvasElement(Canvas* canvas, int col, int row, float offset, float length);
   explicit SongCanvas_CanvasElement(Canvas* canvas); //Simplified constructor, uses default values and skips the setup based on selected call. For loading.
   void SetupBase(SongCanvasRackElement* templateElement);
   static CanvasElement* Create(Canvas* canvas, int col, int row) { return new SongCanvas_CanvasElement(canvas, col, row, 0, 1); }

   CanvasElement* CreateDuplicate() const override;

   SongCanvasRackElement* GetRackElement() { return mRackPart; }
   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in) override;
   bool IsResizable() const override { return mAllowResizing; };
   void SetAllowResize(bool allow) { mAllowResizing = allow; }
   uint32_t GetIndex() const { return mIndex; }
   Canvas* GetCanvas() const { return mCanvas; }
   bool UseCustomPosQuantization() override;
   float GetCustomPosQuantization(float input, int context) override;

   int GetRackElementId() const { return mRackParentID; }
   //float GetMinLength() override { return 0.25f; }

   //Added offsets to the current ones.
   float mTextDrawXOffset = 2;
   float mTextDrawYOffset = 0;

   ofColor mCurrentColor;
   ofColor mCurrentColorGrad;


private:
   void DrawContents(bool clamp, bool wrapped, ofVec2f offset) override;

   std::string mDisplayName;
   std::string* mName;
   std::string mNameCache;
   float mCachedNameSize;
   SongCanvas* mSongCanvas;
   SongCanvasRackElement* mRackPart;
   int mIndex{ 0 };
   int mRackParentID;
   bool mAllowResizing{ true };
};
