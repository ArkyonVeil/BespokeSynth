#pragma once
#include "IFlowGridListener.h"
#include "IUIControl.h"

//Docs in FlowGrid.cpp

struct FlowNameAssigment
{
   std::string internalName;
   std::string displayName;
   int index;
};

class FlowGridElement;

class FlowGrid : public IUIControl
{
public:
   FlowGrid(std::string name, int x, int y, int w, int h, int rows, IDrawableModule* parent, IFlowGridListener* listener);
   void Render() override;
   void MouseReleased() override;
   bool MouseMoved(float x, float y) override;
   void OnClicked(float x, float y, bool right) override;

   bool MouseScrolled(float x, float y, float scrollX, float scrollY, bool isSmoothScroll, bool isInvertedScroll) override;
   int GetRowCount() { return mRows.size(); }

   void SetHighlightCol(double time, int col);
   int GetHighlightCol(double time) const;

   void SetDimensions(float width, float height);
   float GetWidth() const { return mWidth; }
   float GetHeight() const { return mHeight; }
   void SetFromMidiCC(float slider, double time, bool setViaModulator) override;
   void SetBackgroundColour(float r, float g, float b, float a) { mBackgroundColor.set(r, g, b, a); }
   void SetValue(float value, double time, bool forceUpdate) override;
   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, bool shouldSetValue) override;

   void AddElement(FlowGridElement* newElement, int row = -1);
   void RecalculateElements();
   void RemoveElement(FlowGridElement* element);
   std::vector<FlowGridElement*> GetAllElements() { return mElementList; }
   void AddRow();
   void RemoveRow(int row);
   void SetDragAndDrop(bool setAllow) { mAllowDragAndDrop = setAllow; }

protected:
   void AddRowSilent();
   FlowNameAssigment* GetInternalNameForFlowElement(std::string name);
   void DisposeElement(FlowGridElement* element);

   struct FlowNameRecord
   {
      std::string name;
      int index;
      std::vector<int> freeIndexes {};
   };


   std::vector<FlowNameRecord> mFlowNameRecords;
private:
   void GetDimensions(float& width, float& height) override
   {
      width = mWidth;
      height = mHeight;
   }
   enum FlowGridDirection
   {
      Left,
      Right,
      Center
   };

   FlowGridDirection mSortDirection{ Left };


   FlowGridElement* mSelectedElement{ nullptr };
   FlowGridElement* mLastHoveredElement{ nullptr };
   IFlowGridListener* mListener;
   std::vector<FlowGridElement*> mElementList;
   std::vector<std::vector<FlowGridElement*>> mRows;
   float mRowScalingSize[30];

   float mWidth{ 200 };
   float mHeight{ 200 };
   bool mAllowDragAndDrop = { true }; //If it allows elements to be dragged around the gridspace by the user.
   float mRowYSize = {};

   float const mDragDistance { 12 }; //How far to drag in px before it considers a movement a "dragging" operation.

   float mElementSpacing = { 4 }; //The amount of space between each element.
   float mRowXBorderOffset = 2;
   float mRowYBorderOffset = 2;
   ofColor mBackgroundColor = { 0, 0, 0, 75 };

   int mElementNameIndex = 0;
   int mSnapDragIndex;
   int mSnapDragRow;
   bool mHovered{ false };
   bool mDragging{ false }; //Is it being dragged?
   bool mDragToken{ false }; //In a position where it could be dragged?
   bool mPressed{ false };
   int mDragElementRow;
   ofVec2f mStartDragMouse;
   ofVec2f mStartDragElementPos;
   IDrawableModule* mOwner;

   ofVec2f mDragSnapIndicatorPos{ -5, 0 };


   int mDebugIter = 0;



};


class FlowGridElement: public IUIControl
{
public:
   FlowGridElement(FlowGrid* grid);
   virtual ~FlowGridElement();

   //Due to single internal name compliance (which is also used for tooltips). FlowGrids automatically allocate names,
   //so the desired one is seen by the user, while the internal one does all the bookkeeping.
   virtual std::string GetPreferredName() = 0;
   void SetPreferredPosition(int row, float positionPercent);
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

   void SetPreferredSize(int width) { mPreferredWidth = width; }
   float GetPreferredWidth() const { return mPreferredWidth; }
   int GetWidth() const { return mWidth; }
   int GetHeight() const { return mHeight; }

   void SetFlowGrid(FlowGrid* parent) { mFlowGridParent = parent; }
   FlowGrid* GetFlowGrid() const { return mFlowGridParent; }

   void SetHovered(bool hovered) { mHovered = hovered; }
   bool GetHovered() { return mHovered; }

   void SetHighlight(bool highlight) { mHighlighted = highlight; }
   bool GetHighlighted() const { return mHighlighted; }
   void SetColor(ofColor color);
   void SetColorOutline(ofColor color);
   void SetColorsManually(ofColor mainColor, ofColor outlineColor, ofColor highlightColor, ofColor highlightOutlineColor);
   ofVec2f GetRelativePosition();
   virtual void OnMouseClick(bool rightClick) {}
   virtual void OnMouseRelease() {}
   virtual bool MouseMoved(float x, float y) override;
   virtual void MouseReleased() override;
   virtual void Draw();

   FlowGrid* mFlowGridParent;


   //IUIControl
   void SetFromMidiCC(float slider, double time, bool setViaModulator) override {}
   void SetValue(float value, double time, bool forceUpdate = false) override {}
   void KeyPressed(int key, bool isRepeat) override {};
   void SaveState(FileStreamOut& out) override {};
   void LoadState(FileStreamIn& in, bool shouldSetValue = true) override {};
   bool IsSliderControl() override { return false; }
   bool IsButtonControl() override { return false; }
   bool GetNoHover() const override { return true; }
   bool CanBeTargetedBy(PatchCableSource* source) const override {return false;};
   bool ShouldSerializeForSnapshot() const override { return true; }


protected:
   int mHeight = 0;
   int mWidth = 0;

   int mX = 0;
   int mY = 0;

   ofColor mMainColor;
   ofColor mHighlightColor;
   ofColor mOutlineColor;
   ofColor mHighlightOutlineColor;

   float mOutlineThickness{ 0.8F };
private:
   bool mHighlighted = false;
   bool mHovered = false;
   int mPreferredRow = -1;
   float mMinWidth = 30;
   float mPreferredWidth = 90;


};
