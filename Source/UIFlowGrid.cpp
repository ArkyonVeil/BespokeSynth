/// UI Rules:
/// Elements are tossed into the element in vector order. From top left.
/// For an example see the SongCanvas's rack.
///
/// Elements may have two states: Free, Manual
/// Newly created elements are free by default and abide by the following rules:
/// 1. If a row has free space left with all elements maximized, it will allocate to that row, even if it needs to squeeze to fit.
/// 2. If a row has no free space left, it will repeat the check in the row below.
///
/// Manual have the following rules:
/// 1. They were moved directly by a user.
/// 2. They will not respect row maximization rules, they will slot into that row no matter how compressed it is.
/// 3. If an element is manually moved in a row. All elements in that row are considered manual (this should prevent elements going loose when loading/reloading)
/// 3.1. Free Elements that are successfully placed later in that row do not inherit the manual state.
///
/// The panel itself also has these features:
/// 1. If an element is being moved, an additional row is automatically made visible.
/// 2. Alternatively, a number of available rows by default can be also specified.
/// 3. If an element is selected, it resizes to its preferred size (within the bounds of the row), squeezing others further if needed.
/// 4. If that element is unselected, (by say, clicking another element or the empty rack, it will squeeze back.
///
/// Elements support two size definitions:
/// 1. Minimum: The minimum possible size it will squeeze into. If all elements of a row are at their minimum size from overpacking, new elements may not be moved to that row.
/// Please note, this is the minimum size before the row starts to block further additions due to overpacking. It may still get slightly smaller than this depending on various scenarios.
/// 2. Preferred: The default size of an element. If room is available, they will use as much as possible before packing.
/// In the case they are squeezed however, a selected element will always use its preferred size.
///
/// If the FlowGrid itself is resized, elements are reslotted in akin to reloading the panel.
/// Optionally the developer may read the panel's current minimum size, which is based on the most packed row. To prevent overpacking.
///
///
/// Created by ArkyonVeil
///

#include "UIFlowGrid.h"
#include "ModularSynth.h"


UIFlowGrid::UIFlowGrid(std::string name, int x, int y, int w, int rowHeight, int rows, IClickable* parent, ISignalListener* signalListener)
{
   SetName(name.c_str());
   SetPosition(x, y);
   mListener = signalListener;
   SetParent(parent);
   for (int i = 0; i < rows; i++)
      AddRowSilent();
   mWidth = w;
   mHeight = rowHeight * rows;
   mRowYSize = rowHeight;
}

void UIFlowGrid::Render()
{
   ofPushMatrix();
   ofTranslate(mX, mY);
   ofPushStyle();
   /*
   if (!mHovered)
      ofSetColor(0, 255, 0);
   else
   {
      ofSetColor(255, 255, 0);
   }*/

   ofSetColor(mBackgroundColor);
   ofFill();
   ofRect(0, 0, mWidth, mHeight);

   if (mDragging)
   {
      ofSetColor(255, 255, 255, 100);
      ofRect(mStartDragElementPos.x - mX, mStartDragElementPos.y - mY, mSelectedElement->GetWidth(), mSelectedElement->GetHeight());

      ofSetColor(ofColor::yellow);
      ofLine(mDragSnapIndicatorPos.x, mDragSnapIndicatorPos.y, mDragSnapIndicatorPos.x, mDragSnapIndicatorPos.y + mHeight / GetRowCount());
   }

   for (auto elm : mElementList)
   {
      elm->Draw();
   }

   ofPopStyle();
   ofPopMatrix();
}
void UIFlowGrid::OnClicked(float x, float y, bool right)
{
   if (mHovered && mLastHoveredElement != nullptr)
   {
      IUIControl::OnClicked(x, y, right);
      if (mSelectedElement != nullptr)
         mSelectedElement->SetHighlight(false);
      mSelectedElement = mLastHoveredElement;
      mSelectedElement->SetHighlight(true);
      mLastHoveredElement->OnMouseClick(right);
      mStartDragMouse = ofVec2f(x + GetPosition().x, y + GetPosition().y);
      mStartDragElementPos = mSelectedElement->GetRelativePosition();
      if (mSelectedElement->GetHovered())
         mDragToken = true;
      mPressed = true;
   }
}

void UIFlowGrid::MouseReleased()
{
   mDragToken = false;
   mPressed = false;
   if (mDragging)
   {
      //Time for the swaparoo.
      auto& outRow = mRows[mDragElementRow];
      auto& inRow = mRows[mSnapDragRow];

      auto r = std::find(outRow.begin(), outRow.end(), mSelectedElement);
      int idx = std::distance(outRow.begin(), r);


      if (mDragElementRow == mSnapDragRow && idx < mSnapDragIndex)
      {
         mSnapDragIndex -= 1;
      }
      outRow.erase(r);
      if (mSnapDragIndex < 0)
         mSnapDragIndex = 0;

      inRow.insert(inRow.begin() + mSnapDragIndex, mSelectedElement);

      RecalculateElements();

      //mSelectedElement = nullptr;
      //mLastHoveredElement = nullptr;
      mDragging = false;
   }
   IUIControl::MouseReleased();
}
bool UIFlowGrid::MouseMoved(float x, float y)
{
   float rX = x;
   float rY = y;
   x -= mX;
   y -= mY;
   bool isMouseOver = (x >= 0 && x < mWidth && y >= 0 && y < mHeight);

   //TheSynth->LogEvent("MouseMove "+std::to_string(mDebugIter),kLogEventType_Verbose);
   //mDebugIter++;
   //See which element we're hovering over...

   if (mPressed)
   {
      if (ofDistSquared(rX, rY, mStartDragMouse.x, mStartDragMouse.y) > mDragDistance && !mDragging && mDragToken)
      {
         mDragging = true;
         mDragToken = false;
         //Heckin' lazy, so I'll just learn their row location this way -_-
         bool foundRow = false;
         for (int x = 0; x < mRows.size(); ++x)
         {
            for (int y = 0; y < mRows[x].size(); ++y)
            {
               if (mRows[x][y] == mSelectedElement)
               {
                  mDragElementRow = x;
                  foundRow = true;
                  break;
               }
            }
            if (foundRow)
               break;
         }
      }
   }

   if (mDragging)
   {
      float dX = ofClamp(x, 0, mWidth - mSelectedElement->GetWidth());
      float dY = ofClamp(y, 0, mHeight - mSelectedElement->GetHeight());

      mSelectedElement->SetPosition(dX, dY);


      float offset = mRowXBorderOffset;
      int tRow = CLAMP(std::floor(y / mHeight * GetRowCount()), 0, GetRowCount() - 1);
      auto row = mRows[tRow];

      mSnapDragRow = tRow;
      if (row.size() > 0)
         for (int i = 0; i < row.size(); ++i)
         {
            float rowSizeMul = mRowScalingSize[tRow];

            offset += row[i]->GetPreferredWidth() * rowSizeMul / 2.0;

            //Do the check here.
            if (offset > x)
            {
               offset -= row[i]->GetWidth() / 2;
               if (offset > 8)
               {
                  offset -= mElementSpacing / 2 * rowSizeMul;
               }
               mDragSnapIndicatorPos = ofVec2f(offset, mRowYSize * tRow);
               mSnapDragIndex = i;
               break;
            }

            offset += row[i]->GetPreferredWidth() * rowSizeMul / 2.0;

            offset += mElementSpacing * rowSizeMul;

            if (i + 1 == row.size())
            {
               offset -= mElementSpacing / 2 * rowSizeMul;
               mDragSnapIndicatorPos = ofVec2f(offset, mRowYSize * tRow);
               mSnapDragIndex = row.size();
               break;
            }
         }
      else
      {
         mDragSnapIndicatorPos = ofVec2f(0, mRowYSize * tRow);
         mSnapDragIndex = 0;
      }
   }

   if (isMouseOver)
   {
      int hRow = std::floor(y / mHeight * mRows.size());
      hRow = std::clamp(hRow, 0, static_cast<int>(mRows.size() - 1));

      float offsetRange = 0;
      bool select = false;
      //TheSynth->LogEvent("Row:  " + std::to_string(hRow), kLogEventType_Verbose);

      if (mLastHoveredElement != nullptr)
      {
         mLastHoveredElement->SetHovered(false);
      }

      for (int i = 0; i < mRows[hRow].size(); ++i)
      {
         float rowSizeMul = mRowScalingSize[hRow];
         offsetRange += mRows[hRow][i]->GetPreferredWidth() * rowSizeMul + mElementSpacing * rowSizeMul;
         if (x < offsetRange && !select)
         {
            select = true;

            mRows[hRow][i]->MouseMove(x, y);
            mRows[hRow][i]->SetHovered(true);
            mLastHoveredElement = mRows[hRow][i];
         }
         else
         {
            mRows[hRow][i]->SetHovered(false);
         }
      }
   }
   else if (mHovered && !isMouseOver)
   {
      for (int i = 0; i < mElementList.size(); ++i)
      {
         mElementList[i]->SetHovered(false);
      }
   }
   mHovered = isMouseOver;

   return IUIControl::MouseMoved(x, y);
}
bool UIFlowGrid::MouseScrolled(float x, float y, float scrollX, float scrollY, bool isSmoothScroll, bool isInvertedScroll)
{
   return IUIControl::MouseScrolled(x, y, scrollX, scrollY, isSmoothScroll, isInvertedScroll);
}
void UIFlowGrid::SetHighlightCol(double time, int col)
{
}
int UIFlowGrid::GetHighlightCol(double time) const
{
   return 0;
}
void UIFlowGrid::SetDimensions(float width, float height)
{
   mWidth = width;
   mHeight = height;
   RecalculateElements();
}

UIFlowGrid::~UIFlowGrid()
{
}

void UIFlowGrid::SetFromMidiCC(float slider, double time, bool setViaModulator)
{
}
void UIFlowGrid::SetValue(float value, double time, bool forceUpdate)
{
}
void UIFlowGrid::SaveState(FileStreamOut& out)
{
}
void UIFlowGrid::LoadState(FileStreamIn& in, bool shouldSetValue)
{
}
void UIFlowGrid::AddElement(UIFlowGridElement* newElement, int row)
{
   newElement->SetFlowGrid(this);

   if (row != -1)
   {
      while (mRows.size() <= row)
      {
         AddRow();
      }
      mRows[row].push_back(newElement);
      mElementList.push_back(newElement);
      RecalculateElements();
      return;
   }

   float maxSpace = mWidth;
   //Verify if there's room in any Row
   for (int x = 0; x < mRows.size(); ++x)
   {
      float rowOccupiedSpace = 4;
      auto row = mRows[x];

      for (int y = 0; y < row.size(); ++y)
      {
         rowOccupiedSpace += row[y]->GetWidth() + mElementSpacing;
      }

      if (rowOccupiedSpace + newElement->GetPreferredWidth() <= maxSpace)
      {
         mRows[x].push_back(newElement);
         mElementList.push_back(newElement);
         break;
      }
      //If we are about to run out of room, add another row.
      if (x + 1 == mRows.size())
      {
         AddRow();
      }
   }


   RecalculateElements();
}

void UIFlowGrid::RecalculateElements()
{

   float maxRowWidth = mWidth;
   float YOffsetPerRow = mHeight / GetRowCount() - 4;
   float rowYOffset = mRowYBorderOffset;

   for (int x = 0; x < mRows.size(); ++x)
   {
      float rowXOffset = mRowXBorderOffset;

      float spaceOccupied = 4;

      //Add the occupied space, so that we sort if we must.
      for (size_t y = 0; y < mRows[x].size(); y++)
      {
         auto row = mRows[x];

         spaceOccupied += row[y]->GetPreferredWidth() + mElementSpacing;
      }
      if (spaceOccupied < 0)
         spaceOccupied = 1;
      float sizeMul = MIN(maxRowWidth / spaceOccupied, 1);

      mRowScalingSize[x] = sizeMul;

      for (size_t y = 0; y < mRows[x].size(); ++y)
      {
         auto row = mRows[x];

         row[y]->SetPosition(rowXOffset, rowYOffset);
         row[y]->SetSize(row[y]->GetPreferredWidth() * sizeMul, YOffsetPerRow);

         rowXOffset += row[y]->GetPreferredWidth() * sizeMul + mElementSpacing * sizeMul;
      }
      rowYOffset += mHeight / GetRowCount();
   }
}
void UIFlowGrid::RemoveElement(UIFlowGridElement* element)
{
   //TheSynth->LogEvent("Tried to delete an element.",LogEventType::kLogEventType_Verbose);
   auto& row = mRows[0];
   row.erase(std::find(row.begin(), row.end(), element));
   //mRows[0] = row;
   mElementList.erase(std::find(mElementList.begin(), mElementList.end(), element));
   delete element;
   RecalculateElements();
}

void UIFlowGrid::AddRow()
{
   //mRows.push_back(std::vector<UIFlowGridElement*>());
   mRows.emplace_back();

   SetDimensions(mWidth, mRows.size() * mRowYSize);
   mListener->ReceiveSignal(SignalId::ResizeRequest);
}

void UIFlowGrid::AddRowSilent()
{
   mRows.emplace_back();

   SetDimensions(mWidth, mRows.size() * mRowYSize);
}
void UIFlowGrid::RemoveRow(int row)
{
}
