#pragma once
#include "IDrawableModule.h"
#include "FlowGridElement.h"

//Docs in FlowGrid.cpp

struct FlowNameAssigment
{
   std::string internalName;
   std::string displayName;
   int index;
};

class IFlowGridListener
{
public:
   virtual ~IFlowGridListener() = default;

   virtual void onFlowGridNewSelection(FlowGridElement* element);
   virtual void onFlowGridResize(float newBoundsX, float newBoundsY) {};//The grid is resizing in some direction, ignore, and it may clip.
};

class FlowGrid : public IDrawableModule
{
public:
   FlowGrid(std::string name, int x, int y, int w, int h, int rows, IDrawableModule* parent, IFlowGridListener* listener);
   void Render() override;
   void MouseReleased() override;
   bool MouseMoved(float x, float y) override;
   void OnClicked(float x, float y, bool right) override;

   bool MouseScrolled(float x, float y, float scrollX, float scrollY, bool isSmoothScroll, bool isInvertedScroll) override;
   int GetRowCount() const { return mRows.size(); }

   void SetHighlightCol(double time, int col);
   int GetHighlightCol(double time) const;

   void SetDimensions(float width, float height);
   float GetWidth() const { return mWidth; }
   float GetHeight() const { return mHeight; }
   void SetBackgroundColour(float r, float g, float b, float a) { mBackgroundColor.set(r, g, b, a); }
   void DrawModule() override;


   void SetMaxRows(int rowNum){ mMaxRows = rowNum;}

   void AddFlowElement(FlowGridElement* newElement);
   void UpdateRow(int index);
   void RecalculateFlowGrid();
   void RemoveFlowElement(FlowGridElement* element);
   void AddRow();
   void PopRow();
   void ResizeFlowgrid();
   std::vector<FlowGridElement*> GetAllElements() { return mElementList; }

   void SetAllowDragAndDrop(bool setAllow) { mAllowDragAndDrop = setAllow; }
   void SetSelectedGridElement(FlowGridElement* element);

   FlowGridElement* GetSelectedGridElement() const { return mSelectedElement; }

   struct FlowGridRow
   {
      bool isOverfilled;//No more elements allowed to be moved.
      bool isFilled;//Currently packed, free elements cannot be moved there automatically.
      std::vector<FlowGridElement*> elements;
   };
   std::vector<FlowGridRow> mRows;

protected:
   int TryGetSlot(int targetRow);
   bool IsRowOverfilled(int row);

   void AddToRow(FlowGridElement* element, int row);
   void InsertToRow(FlowGridElement* element, int row, int index);

   void AddRowSilent();

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
   float mRowScalingSize[30];

   float mWidth{ 400 };
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

   int mMaxRows = -1;
   int mDebugIter = 0;
};
