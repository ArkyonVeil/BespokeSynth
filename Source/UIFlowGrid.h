#pragma once
#include "ISignalListener.h"
#include "IUIControl.h"

class UIFlowGridElement;
class UIFlowGrid : public IUIControl
{
public:
   UIFlowGrid(std::string name, int x, int y, int w, int h, int rows, IClickable* parent, ISignalListener* signalListener);
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
   void SetValue(float value, double time, bool forceUpdate) override;
   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, bool shouldSetValue) override;

   void AddElement(UIFlowGridElement* newElement, int row = -1);
   void RecalculateElements();
   void RemoveElement(UIFlowGridElement* element);
   std::vector<UIFlowGridElement*> GetAllElements(){return mElementList;}

   void AddRow();
   void RemoveRow(int row);
   void SetDragAndDrop(bool setAllow) {mAllowDragAndDrop = setAllow;}
   
protected:
   ~UIFlowGrid();
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
   
   FlowGridDirection mSortDirection {Left};


   UIFlowGridElement* mSelectedElement {nullptr};
   UIFlowGridElement* mLastHoveredElement {nullptr};
   ISignalListener* mListener;
   std::vector<UIFlowGridElement*> mElementList;
   std::vector<std::vector<UIFlowGridElement*>> mRows;
   float mRowScalingSize[30]; 
   
   float mWidth{ 200 };
   float mHeight{ 200 };
   bool mAllowDragAndDrop = {true};//If it allows elements to be dragged around the gridspace by the user.
   float mRowYSize = {};
   
   float mDragDistance = {12}; //How far to drag in px before it considers a movement a "dragging" operation. 

   float mElementSpacing = {4}; //The amount of space between each element.
   float mRowXBorderOffset = 2;
   float mRowYBorderOffset = 2;

   int mSnapDragIndex;
   int mSnapDragRow;
   bool mHovered {false};
   bool mDragging {false};//Is it being dragged?
   bool mDragToken {false};//In a position where it could be dragged?
   bool mPressed {false};
   int mDragElementRow;
   ofVec2f mStartDragMouse;
   ofVec2f mStartDragElementPos;

   ofVec2f mDragSnapIndicatorPos {-5,0};
   
   int mDebugIter = 0;

};


class UIFlowGridElement
{
public:
   UIFlowGridElement(float preferredWidth, ofColor baseColor);
   virtual ~UIFlowGridElement();
   void SetPreferredPosition(int row, float positionPercent);

   void SetPosition(int x, int y) {mX = x; mY = y;}
   void SetSize(int width, int height) {mWidth = width; mHeight = height;}
   void SetPreferredSize(int width) {mPreferredWidth = width;}
   float GetPreferredWidth() const { return mPreferredWidth;}
   int GetWidth() const { return mWidth; }
   int GetHeight() const { return mHeight; }
   
   void SetFlowGrid(UIFlowGrid* parent){mFlowGridParent = parent;}
   UIFlowGrid* GetFlowGrid() const { return mFlowGridParent;}

   void SetHovered(bool hovered) {mHovered = hovered;}
   bool GetHovered() {return mHovered;}
  
   
   void SetHighlight(bool highlight) { mHighlighted = highlight; }
   bool GetHighlighted() const { return mHighlighted; }
   void SetColor(ofColor color);
   void SetColorsManually(ofColor mainColor, ofColor outlineColor, ofColor highlightColor, ofColor highlightOutlineColor);
   ofVec2f GetRelativePosition();
   virtual void OnMouseClick(bool rightClick) {}
   virtual void OnMouseRelease(){}
   void MouseMove(float x, float y);
   virtual void Draw();
   
protected:
   int mHeight = 0;
   int mWidth = 0;
   
   int mX = 0;
   int mY = 0;
   
   ofColor mMainColor;
   ofColor mHighlightColor;
   ofColor mOutlineColor;
   ofColor mHighlightOutlineColor;

   float mOutlineThickness {0.8F};
private:
   float mPreferredPosition = -1;
   bool mHighlighted = false;
   bool mHovered = false;
   int mPreferredRow = -1;
   float mPreferredWidth = 90;
   UIFlowGrid* mFlowGridParent;


  
   
};
