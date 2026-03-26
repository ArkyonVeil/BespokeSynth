#pragma once
#include "IDrawableModule.h"
#include "FlowGridElement.h"
#include "ModuleContainer.h"

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

   virtual void onFlowGridNewSelection(FlowGridElement* element){};
   virtual void onFlowGridResize(float newBoundsX, float newBoundsY){}; //The grid is resizing in some direction, ignore, and it may clip.
};

class FlowGridElementFactory
{
public:
   ~FlowGridElementFactory() = default;
   virtual FlowGridElement* Create(std::string typeName) = 0;
};

class FlowGrid
{
public:
   FlowGrid(int x, int y, int w, int h, int startNumRows, IDrawableModule* owner, IFlowGridListener* listener);
   ~FlowGrid();
   void CreateUIControls();
   void MouseReleased();
   bool MouseMoved(float x, float y);
   void OnClicked(float x, float y, bool right);

   bool MouseScrolled(float x, float y, float scrollX, float scrollY, bool isSmoothScroll, bool isInvertedScroll);
   int GetRowCount() const { return mRows.size(); }

   void SetDimensions(float width, float height);
   float GetWidth() const { return mWidth; }
   float GetHeight() const { return mHeight; }
   void SetBackgroundColour(float r, float g, float b, float a) { mBackgroundColor.set(r, g, b, a); }
   void DrawModule();

   void SetMaxRows(int rowNum) { mMaxRows = rowNum; }
   void SetMinRows(int rowNum) { mMinRows = rowNum; }

   void AddFlowElement(FlowGridElement* newElement, bool preSetup = false);
   void AddToRow(FlowGridElement* element, int row);
   void InsertToRow(FlowGridElement* element, int row, int index);
   void MoveToRow(FlowGridElement* element, int row, int index);
   void UpdateRow(int index, bool updateFillState);
   void RecalculateFlowGrid();
   void RemoveFlowElement(FlowGridElement* element);
   void ReturnName(FlowNameAssigment* nAssign);
   void AddRow();
   void PopRow();
   void ResizeFlowGrid();
   void InitAllFlowElements() const; //Only called automatically after the parent module is initialized.
   std::vector<FlowGridElement*> GetAllElements() { return mElementList; }

   void SetAllowDragAndDrop(bool setAllow) { mAllowDragAndDrop = setAllow; }
   void SetSelectedGridElement(FlowGridElement* element);
   FlowNameAssigment* GetInternalNameForFlowElement(std::string name);


   virtual void SaveElements(FileStreamOut& out);
   virtual void LoadElements(FlowGridElementFactory* factory, FileStreamIn& in);

   FlowGridElement* GetSelectedGridElement() const { return mSelectedElement; }
   FlowGridElement* GetHoveredGridElement() const { return mHoveredElement; }

   struct FlowGridRow
   {
      bool isOverfilled; //No more elements allowed to be moved.
      bool isFilled; //Currently packed, free elements cannot be moved there automatically.
      std::vector<FlowGridElement*> elements;
   };
   std::vector<FlowGridRow> mRows;

   void SetPosition(int x, int y)
   {
      mX = x;
      mY = y;
   };
   ofVec2f GetPosition() const { return ofVec2f{ mX, mY }; };

protected:
   int TryGetSlot(int targetRow);
   bool IsRowOverfilled(int row);

   struct FlowNameRecord
   {
      std::string name;
      int index;
      std::vector<int> freeIndexes{};
   };

   std::vector<FlowNameRecord> mFlowNameRecords;

private:
   enum FlowGridDirection
   {
      Left,
      Right,
      Center
   };


   FlowGridDirection mSortDirection{ Left };


   FlowGridElement* mSelectedElement{ nullptr };
   FlowGridElement* mHoveredElement{ nullptr };
   IFlowGridListener* mListener;
   std::vector<FlowGridElement*> mElementList;
   float mRowScalingSize[30];

   bool mAllowDragAndDrop = { true }; //If it allows elements to be dragged around the gridspace by the user.
   float mRowYSize = {};

   float const mDragDistance{ 12 }; //How far to drag in px before it considers a movement a "dragging" operation.

   float mElementXSpacing = { 4 }; //The amount of space between each element.
   float mElementYSpacing = { 2 };
   float mRowXBorderOffset = 2;
   float mRowYBorderOffset = 2;
   ofColor mBackgroundColor = { 0, 0, 0, 75 };

   int mElementNameIndex = 0;
   int mSnapDragIndex;
   int mSnapDragRow;
   ofRectangle mRackPartDragGhostRect;
   bool mHovered{ false };
   bool mDragging{ false }; //Is an element currently being dragged?
   ofVec2f mStartDragMouse;
   bool mPressed{ false };
   int mDragElementRow;
   ofVec2f mStartDragElementPos;
   IDrawableModule* mOwner;

   ofVec2f mDragSnapIndicatorPos{ -5, 0 };
   int mMaxRows = -1;
   int mMinRows = 2; //Number of rows to always display.
   int mDebugIter = 0;

   float mWidth = 0;
   float mHeight = 0;
   float mX = 0;
   float mY = 0;
};
