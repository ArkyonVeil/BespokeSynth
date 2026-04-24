///////////////////////
///Flow Grid Element///
///////////////////////

//Created, because I keep writing to the wrong class header. -Ark
#pragma once

struct FlowNameAssigment;
class FlowGrid;

class FlowGridElement : public IDrawableModule
{
public:
   FlowGridElement(FlowGrid* grid, std::string elementTypeName);
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
   //IDrawableModule
   void CreateUIControls() override;
   void Init() override;
   void Render() override;
   bool HasTitleBar() const override { return false; };
   void SetEnabled(bool enabled) override { true; }; //Must be true, or the IDrawableModule will try to spawn an enabled checkbox which will crash it.
   bool CanMinimize() override { return false; }; //Must be false so <IDrawableModule* IClickable::GetModuleParent()> traverses the chain correctly.
   ofRectangle CustomTooltipBounds() const override { return { 0, 0, mWidth, mHeight }; };

   //FlowGrid sizing rules.
   // > Preferred size is the max size allowed to this module, it will occupy as much as possible.
   // Selected modules have priority over others, and will display at their preferred size if possible, even if squeezed.
   // > Compact size is the minimum size it may be squeezed into before further modules being moved to the same row are blocked.
   // > Min Size is the minimum possible size this module may be compacted to.
   // As it takes priority over the selected module, it will not be squeezed further.
   virtual float GetPreferredWidth() const { return 60; }
   void SetCompactSize(float compactWidth) { mCompactWidth = compactWidth; }
   float GetCompactWidth() { return mCompactWidth; }
   float GetMinWidth() { return mMinWidth; }


   float GetWidth() const { return mWidth; }
   float GetHeight() const { return mHeight; }
   void GetDimensions(float& width, float& height) override
   {
      width = mWidth;
      height = mHeight;
   };

   void SetFlowGrid(FlowGrid* parent) { mFlowGridParent = parent; }
   FlowGrid* GetFlowGrid() const { return mFlowGridParent; }

   void SetHovered(bool hovered) { mHovered = hovered; }
   bool IsHovered() const { return mHovered; }

   ofRectangle GetRect() const { return { mX, mY, mWidth, mHeight }; };
   ofRectangle GetRectRelativeToGrid() const;
   ofRectangle GetRectLocal() const { return { 0, 0, mWidth, mHeight }; };
   void SetRect(ofRectangle rect);
   void SetRectRelativeToGrid(ofRectangle rect);
   virtual bool TestIntercepts(float x, float y, bool right);

   void SetHighlight(bool highlight) { mSelected = highlight; }
   bool GetHighlighted() const { return mSelected; }
   void SetColor(ofColor color);
   void SetColorOutline(ofColor color);
   void SetColorsManually(ofColor mainColor, ofColor outlineColor, ofColor highlightColor, ofColor highlightOutlineColor);
   ofVec2f GetRelativePosition();
   void OnClicked(float x, float y, bool right) override;
   bool MouseMoved(float x, float y) override;
   void MouseReleased() override;
   void DrawModule() override;
   FlowGrid* mFlowGridParent;
   //Updates the current row for module resizing, call when you made a change that should alter the element's preferred size.
   void UpdateRow();
   //Called after the module has been resized in the grid. Use to show/hide things depending on the new size.
   //You are strongly encouraged, NOT to do any resizing/preferred width altering here.
   virtual void OnPostResize(){};

   std::string mElementTypeName;
   int mPreferredRow = -1;

   virtual std::string GetFlowGridElementType() const = 0;
   int GetModuleSaveStateRev() const override { return 1; };//Major fuckup in rev0.

protected:
   bool mShowing = true;

   float mCompactWidth = 30;

   bool mIsManual{ false }; //If false, it's free. Used for slotting.

   ofColor mMainColor;
   ofColor mHighlightColor;
   ofColor mOutlineColor;
   ofColor mHighlightOutlineColor;

   float mOutlineThickness{ 0.8F };

private:
   bool mSelected = false;
   bool mHovered = false;
   bool mInitialized = false;
   float mDebugNumX = 0;
   float mDebugNumY = 0;
   float mMinWidth = 10;
};
