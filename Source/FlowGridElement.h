///////////////////////
///Flow Grid Element///
///////////////////////

//Created, because I keep writing to the wrong class header. -Ark
#pragma once


struct FlowNameAssigment;
class FlowGrid;

class FlowGridElement: public IDrawableModule
{
public:
   FlowGridElement(FlowGrid* grid);
   virtual ~FlowGridElement();

   //Due to single internal name compliance (which is also used for tooltips). FlowGrids automatically allocate names,
   //so the desired one is seen by the user, while the internal one does all the bookkeeping.
   FlowNameAssigment* NameData;

   void SetPosition(int x, int y)
   {
      mX = x;
      mY = y;
   }
   void SetSize(int width, int height)
   {
      mWidth = width;
      mHeight = height;
   }


   void SetMinimumSize(float minWidth) {mMinWidth = minWidth;}
   float GetMinimumWidth() {return mMinWidth;}
   void SetPreferredSize(int width) { mPreferredWidth = width; }
   float GetPreferredWidth() const { return mPreferredWidth; }
   float GetWidth() const { return mWidth; }
   float GetHeight() const { return mHeight; }
   void GetDimensions(float& width, float& height) override {width = mWidth; height = mHeight;};

   void SetFlowGrid(FlowGrid* parent) { mFlowGridParent = parent; }
   FlowGrid* GetFlowGrid() const { return mFlowGridParent; }

   void SetHovered(bool hovered) { mHovered = hovered; }
   bool GetHovered() { return mHovered; }

   void SetRect(ofRectangle rect);

   void SetHighlight(bool highlight) { mHighlighted = highlight; }
   bool GetHighlighted() const { return mHighlighted; }
   void SetColor(ofColor color);
   void SetColorOutline(ofColor color);
   void SetColorsManually(ofColor mainColor, ofColor outlineColor, ofColor highlightColor, ofColor highlightOutlineColor);
   ofVec2f GetRelativePosition();
   void OnClicked(float x, float y, bool right) override;
   bool MouseMoved(float x, float y) override;
   void MouseReleased() override;
   void DrawModule() override;

   FlowGrid* mFlowGridParent;

   //IDrawableModule overrides:
   bool HasTitleBar() const override {return false;}
   void Render() override;

protected:
   float mHeight = 0;
   float mWidth = 0;

   float mMinWidth = 30;
   float mPreferredWidth = 90;

   bool mIsManual { false };//If false, it's free. Used for slotting.

   ofColor mMainColor;
   ofColor mHighlightColor;
   ofColor mOutlineColor;
   ofColor mHighlightOutlineColor;

   float mOutlineThickness{ 0.8F };
private:
   bool mHighlighted = false;
   bool mHovered = false;
   int mPreferredRow = -1;

   int mDebugNum = 0;

};
