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

#include "FlowGrid.h"

#include "ModularSynth.h"
#include "PatchCableSource.h"


FlowGrid::FlowGrid(int x, int y, int w, int rowHeight, int startNumRows, IDrawableModule* owner, IFlowGridListener* listener)
{
   SetPosition(x, y);
   mListener = listener;
   mOwner = owner;
   mMinRows = startNumRows;
   mWidth = w;
   mRowYSize = rowHeight;
   mHeight = rowHeight * startNumRows + mRowYBorderOffset * 2 + mElementYSpacing * (startNumRows - 1);
}
FlowGrid::~FlowGrid()
{
   for (auto el : mElementList)
   {
      DeleteFlowElement(el);
   }
}
void FlowGrid::CreateUIControls()
{
   for (int i = mRows.size(); i < mMinRows; i++)
      AddRow();
}

void FlowGrid::OnClicked(float x, float y, bool right)
{
   x -= mX;
   y -= mY;
   //Center the cords.
   if (mHovered && mHoveredElement != nullptr && gHoveredUIControl == nullptr)
   {
      float hX, hY;
      mHoveredElement->GetPosition(hX, hY, true);
      if (mHoveredElement->TestIntercepts(hX, hY, false)) //So we don't accidentally drag the module while dragging cables
         return;
      mSelectedElement = mHoveredElement;
      if (mSelectedElement != mLastSelectedElement)
      {
         if (mLastSelectedElement == nullptr)
         {
            mLastSelectedElement = mSelectedElement;
            mSelectedElement->UpdateRow();
         }
         else
         {
            int rowNew = GetRowIndexOfElement(mSelectedElement);
            int rowOld = GetRowIndexOfElement(mLastSelectedElement);
            if (rowNew != rowOld)
               UpdateRow(rowOld, false);
            UpdateRow(rowNew, false);
            mLastSelectedElement = mSelectedElement;
         }
      }
      mStartDragMouse = ofVec2f(x, y);
      mRackPartDragGhostRect = mSelectedElement->GetRectRelativeToGrid();
      mPressed = true;
      mRowCountOnDragStart = mRows.size();
   }
   else
   {
      int rowToUpdate = -1;
      if (mSelectedElement != nullptr)
      {
         mListener->onFlowGridSelectionCleared(mSelectedElement);
         rowToUpdate = GetRowIndexOfElement(mSelectedElement);
         mSelectedElement = nullptr;
         UpdateRow(rowToUpdate, false);
      }

      if (mHoveredElement != nullptr) //We also reset the hovered state if this happens. So you can have a clean workspace.
      {
         int rowHov = GetRowIndexOfElement(mHoveredElement);
         mHoveredElement = nullptr;
         if (rowHov != rowToUpdate)
            UpdateRow(rowHov, false);
      }
   }
}

bool FlowGrid::MouseMoved(float x, float y)
{

   x -= mX;
   y -= mY;
   //Offset out stuff
   mHovered = x >= 0 && x < mWidth && y >= 0 && y < mHeight;

   //Pass the current hovering events.
   mHoveredElement = nullptr;
   for (auto el : mElementList)
   {
      if (el->GetRectRelativeToGrid().contains(x, y))
      {
         mHoveredElement = el;
         if (mLastHoveredElement != mHoveredElement)
         {
            if (mPressed)
               break; //We ignore hovers if we're currently pressing something, just as dragging. Makes the UI less jerky.
            if (mLastHoveredElement == nullptr)
            {
               mHoveredElement->UpdateRow();
               mLastHoveredElement = mHoveredElement;
               break;
            }
            int rowNew = GetRowIndexOfElement(mHoveredElement);
            int rowOld = GetRowIndexOfElement(mLastHoveredElement);
            if (rowNew != rowOld)
               UpdateRow(rowOld, false);
            mLastHoveredElement = mHoveredElement;
            UpdateRow(rowNew, false);
         }
         break;
      }
   }
   if (mPressed == true && mStartDragMouse.distanceSquared(ofVec2f(x, y)) > 10)
   {
      mDragging = true;
      mDraggedElement = mSelectedElement;
      float elPX, elPY;
      elPX = mX + mRackPartDragGhostRect.x + x - mStartDragMouse.x;
      elPY = mY + mRackPartDragGhostRect.y + y - mStartDragMouse.y;
      elPX = CLAMP(elPX, mX, mX + mWidth - mSelectedElement->GetWidth());
      elPY = CLAMP(elPY, mY, mY + mHeight - mSelectedElement->GetHeight());
      mSelectedElement->SetPosition(elPX, elPY);

      //If hovering on the current lowest row, spawn one more.
      float suggestionBorder = mRowYBorderOffset + (mRowCountOnDragStart - 1) * mRowYSize;
      if (y > suggestionBorder && !mSuggestedRowActive && mRows.size() < mMaxRows)
      {
         mSuggestedRowActive = true;
         mSkipGridRecalculation = true;
         AddRow();
         mSkipGridRecalculation = false;
      }
      else if (mSuggestedRowActive && y <= suggestionBorder - 12) //Pop it if too far.
      {
         mSuggestedRowActive = false;
         mSkipGridRecalculation = true;
         PopRow();
         mSkipGridRecalculation = false;
      }


      //Go through the rows and find to the most likely place to snap to.
      int rowSnap = MAX(0, floor(y / (mRows.size() * mRowYSize + mRowYBorderOffset * 2) * mRows.size()));
      rowSnap = MIN(mRows.size() - 1, rowSnap);
      auto rowObj = mRows[rowSnap];

      if (rowObj.isOverfilled)
      {
         if (rowSnap != mDragElementRow)
         {
            return false; //Cannot snap/move to overfilled rows.
         }
         //If its from the same row, it may still be moved around.
      }

      float xOffset = mRowXBorderOffset;
      float xSnapPos = mRowXBorderOffset;

      mSnapDragRow = rowSnap;
      mSnapDragIndex = 0;

      int idx = 0;
      for (auto el : mRows[rowSnap].elements)
      {
         idx++;
         xOffset += el->GetWidth() + mElementXSpacing;

         //Once xOffset is larger than x, we stop jumping
         //If x is roughly in the first half of the previous element, we place it before the previous element.
         //Otherwise we place it ahead of the previous element.

         mSnapDragIndex++;
         if (xOffset > x || idx == mRows[rowSnap].elements.size())
         {
            float diff = xOffset - x;
            if (el->GetWidth() / 2 > diff)
            {
               xSnapPos = xOffset - mElementXSpacing / 2;
            }
            else
            {
               //if (idx < mRows[rowSnap].elements.size())
               mSnapDragIndex--;
               xSnapPos = xOffset - el->GetWidth() - mElementXSpacing * 1.5;
            }
            break;
         }
      }
      mSnapDragIndex = CLAMP(mSnapDragIndex, 0, mRows[rowSnap].elements.size());
      mDragSnapIndicatorPos = ofVec2f(xSnapPos, mRowYBorderOffset + rowSnap * mRowYSize);
   }

   return false;
}

void FlowGrid::MouseReleased()
{
   mPressed = false;
   mRowCountOnDragStart = -1;
   mDraggedElement = nullptr;
   if (mDragging)
   {
      mDragging = false;
      mSelectedElement->SetPosition(mX + mRackPartDragGhostRect.x, mY + mRackPartDragGhostRect.y);

      MoveToRow(mSelectedElement, mSnapDragRow, mSnapDragIndex);
   }
   //Check if we have displayed a preview row, if its empty, we pop it.
   if (mSuggestedRowActive)
   {
      mSuggestedRowActive = false;
      if (mRows[mRows.size() - 1].elements.empty())
      {
         PopRow();
      }
   }
}

bool FlowGrid::MouseScrolled(float x, float y, float scrollX, float scrollY, bool isSmoothScroll, bool isInvertedScroll)
{
   return false;
}
void FlowGrid::SetDimensions(float width, float yRowSize)
{
   if (yRowSize != -1)
      mRowYSize = yRowSize;
   mWidth = width;
   mHeight = mRowYSize * mRows.size() + (MAX(0, mRows.size() - 1)) * mElementYSpacing + mRowYBorderOffset * 2;
   RecalculateFlowGrid();
}

float FlowGrid::GetMinWidth() const
{
   float maxRowMinWidth = 0;
   for (auto& r : mRows)
   {
      if (r.elements.empty())
         continue;

      float rowTotal = 2 * mRowXBorderOffset;
      for (int i = 0; i < r.elements.size(); ++i)
      {
         rowTotal += r.elements[i]->GetMinWidth();
         if (i + 1 < static_cast<int>(r.elements.size()))
            rowTotal += mElementXSpacing;
      }
      maxRowMinWidth = MAX(maxRowMinWidth, rowTotal);
   }
   return maxRowMinWidth;
}
void FlowGrid::DrawModule()
{
   ofPushMatrix();
   ofTranslate(mX, mY);
   ofPushStyle();

   if (!mHovered)
      ofSetColor(mBackgroundColor);
   else
   {
      int hColAdd = 6;
      ofSetColor(ofColor(mBackgroundColor.r + hColAdd, mBackgroundColor.g + hColAdd, mBackgroundColor.b + hColAdd, mBackgroundColor.a));
   }

   ofFill();
   ofRect(0, 0, mWidth, mHeight);

   if (mDragging)
   {
      ofSetColor(255, 255, 255, 100);
      ofNoFill();
      ofRect(mRackPartDragGhostRect.x, mRackPartDragGhostRect.y, mRackPartDragGhostRect.width, mRackPartDragGhostRect.height);

      ofSetColor(ofColor::yellow);
      ofLine(mDragSnapIndicatorPos.x, mDragSnapIndicatorPos.y, mDragSnapIndicatorPos.x, mDragSnapIndicatorPos.y + mHeight / GetRowCount());
   }
   /*
   //Debug colours, shows the state/formula of the row:
   for (int i = 0; i < mRows.size(); ++i)
   {
      float offset = mRowYBorderOffset + mRowYSize*i;
      if (mRows[i].isFilled)//Formula 1
      {
         ofSetColor(ofColor::yellow);
         ofLine(15,offset,15,offset+mRowYSize);
      }
      if (mRows[i].isOverfilled)//Formula 2
      {
         ofSetColor(ofColor::red);
         ofLine(17,offset,17,offset+mRowYSize);
      }
   }*/

   ofPopStyle();
   ofPopMatrix();


   //Draw in parent space.
   for (auto elm : mElementList)
   {
      elm->Render();
   }
}

//Tries to find an available slot in the grid, returns -1 if none available. Otherwise, returns the row.
//Specify a row for a manually moved element.
int FlowGrid::TryGetSlot(int targetRow = -1)
{
   int nonMaxed = -1; //Used for emergencies.
   for (int i = 0; i < mRows.size(); ++i)
   {
      if (nonMaxed == -1)
      {
         if (!mRows[i].isOverfilled)
         {
            nonMaxed = i;
            //If we later find we can't add any more rows, we can allow it here.
         }
      }
      if (targetRow == i) //Manual placement
      {
         if (mRows[i].isOverfilled)
         {
            return -1; //None available in desired slot.
         }
         else //Available, return the slot number
         {
            return i;
         }
      }
      else
      {
         if (!mRows[i].isFilled)
         {
            return i; //Can put it here.
         }
         //nope, keep going.
      }
   }
   //No slots found, can we make a new row?
   if (mRows.size() == -1 || mRows.size() < mMaxRows)
   {
      //We can make one there.
      return mRows.size();
   }
   if (nonMaxed != -1)
   {
      //We're capped, but there's still vacant space.
      return nonMaxed;
   }
   //DISASTER, nothing available, are you trying to stress test this or something?
   return -1;
}

//Checks if the row is overfilled, thus blocking new modules from moving in.
bool FlowGrid::IsRowOverfilled(int row)
{
   if (mRows[row].isOverfilled)
      return true;
   return false;
}


//Please check with TryGetSlot to see if it's possible to add first, or it WILL crash.
void FlowGrid::AddFlowElement(FlowGridElement* newElement, bool preSetup)
{
   newElement->SetFlowGrid(this);

   int r = TryGetSlot();

   assert(r != -1); //If you fail this, the grid has been specified to not have enough room, but no check was done to prevent this.
   //Now we have a rogue class object and nowhere to put it . <>(

   if (!preSetup)
   {
      //Add it to the pipeline
      newElement->SetName(newElement->mElementTypeName.c_str());
      newElement->SetTypeName(newElement->mElementTypeName, newElement->GetModuleCategory());
      newElement->CreateUIControls();
      if (mOwner->IsInitialized())
      {
         newElement->Init();
      }
      //auto rec =  GetInternalNameForFlowElement(newElement->mElementTypeName);
   }
   //newElement->NameData = rec;
   newElement->SetParent(mOwner);
   mOwner->AddChild(newElement);

   mElementList.push_back(newElement);

   AddToRow(newElement, r);
}

void FlowGrid::AddToRow(FlowGridElement* element, int row)
{
   while (mRows.size() <= row)
   {
      AddRow();
   }

   mRows[row].elements.push_back(element);
   UpdateRow(row, true);
}

void FlowGrid::InsertToRow(FlowGridElement* element, int row, int index)
{
   while (mRows.size() > row)
   {
      AddRow();
   }
   //This might go wrong, up to testing to find out! <>D
   mRows[row].elements.insert(mRows[row].elements.begin() + index, element);
   UpdateRow(row, true);
}

void FlowGrid::MoveToRow(FlowGridElement* element, int row, int index)
{
   //First we find out in which row it is.
   int sourceRow = -1;
   int sourceIndex = -1;
   int rowIdx = 0;
   for (auto lRow : mRows)
   {
      for (int i = 0; i < lRow.elements.size(); ++i)
      {
         if (lRow.elements[i] == element) //Found it
         {
            sourceRow = rowIdx;
            sourceIndex = i;
            if (row == sourceRow) //Same row move
            {
               //Either of these indexes will result in a no-move situation so we skip it.
               if (sourceIndex == index || sourceIndex == index - 1)
               {
                  //ofLog() << "FlowGrid move rejected: sR" + ofToString(sourceRow) + " sI" + ofToString(sourceIndex) + " -> R" + ofToString(row) + " I" + ofToString(index);
                  return;
               }
            }
         }
      }
      rowIdx++;
   }
   assert(sourceIndex != -1 && sourceRow != -1); //For an element to be "Moved" it needs to already exist in the grid. Otherwise use InsertToRow()


   //Then we move it to its proper location.
   if (sourceRow != row) //Different row move
   {
      //Remove
      mRows[sourceRow].elements.erase(mRows[sourceRow].elements.begin() + sourceIndex);

      //Put in
      mRows[row].elements.insert(mRows[row].elements.begin() + index, element);
      //ofLog() << "FlowGrid move: sR" + ofToString(sourceRow) + " sI" + ofToString(sourceIndex) + " -> R" + ofToString(row) + " I" + ofToString(index);
      UpdateRow(sourceRow, true);
      UpdateRow(row, true);
   }
   else //Same row
   {
      //Remove
      mRows[sourceRow].elements.erase(mRows[sourceRow].elements.begin() + sourceIndex);

      //Put in
      std::vector<FlowGridElement*>::iterator tIdx;
      bool isLast = false;
      if (sourceIndex > index)
      {
         //ofLog() << "FlowGrid move: sR" + ofToString(sourceRow) + " sI" + ofToString(sourceIndex) + " -> R" + ofToString(row) + " I" + ofToString(index);
         tIdx = mRows[sourceRow].elements.begin() + index;
      }
      else
      {
         //ofLog() << "FlowGrid move: sR" + ofToString(sourceRow) + " sI" + ofToString(sourceIndex) + " -> R" + ofToString(row) + " I" + ofToString(index - 1);
         if (index - 1 == mRows[sourceRow].elements.size())
            isLast = true;
         else
            tIdx = mRows[sourceRow].elements.begin() + index - 1;
      }
      if (!isLast)
         mRows[row].elements.insert(tIdx, element);
      else
         mRows[row].elements.push_back(element);
      UpdateRow(row, true);
   }
   CheckCleanupRows();
}

void FlowGrid::CheckCleanupRows()
{
   while (mRows.size() > mMinRows)
   {
      if (mRows[mRows.size() - 1].elements.empty())
      {
         PopRow();
      }
      else
      {
         break;
      }
   }
}

//Updates cached data, and visuals. Does not trigger a resize.
void FlowGrid::UpdateRow(int index, bool updateFillState)
{
   int const rowCount = mRows.size();
   if (rowCount <= index || index < 0)
      return; //Invalid request.

   if (mRows[index].elements.empty())
   {
      mRows[index].isFilled = false;
      mRows[index].isOverfilled = false;
      return;
   }

   FlowGridRow* row = &mRows[index];
   int const elCount = static_cast<int>(row->elements.size());

   float totalPreferredWidth = 0; //How much we can stuff it before it starts to compress
   float totalCompactWidth = 0; //Minimum size to lerp to, not necessarily the smallest possible size. Less priority than selected modules.
   float totalMinWidth = 0; //Absolute minimum size all(except selected) modules can be squeezed into, takes priority over selected.
   float offset = mRowXBorderOffset;
   float totalSpacing = mElementXSpacing * (elCount - 1); //Same priority as module's compact size. Minimum is 0.

   int selectedIndex = -1; //If one is selected, in the squeeze step if applicable, we set it to its largest possible size.
   float selectedPreferredWidth = 0;

   int hoveredIndex = -1;
   float hoveredPreferredWidth = 0;

   float maxRowSize = mWidth - mRowXBorderOffset * 2;
   int yOffset = mRowYBorderOffset + mRowYSize * index + mElementYSpacing * index;


   //Collect data on all the elements.
   for (int i = 0; i < row->elements.size(); ++i)
   {
      auto el = row->elements[i];

      if (mSelectedElement == el)
      {
         //We don't include the selected one in the math for the main compression calcs.
         selectedIndex = i;
         selectedPreferredWidth = el->GetPreferredWidth();
      }
      else if (mHoveredElement == el) //Neither hovered ones.
      {
         hoveredIndex = i;
         hoveredPreferredWidth = el->GetPreferredWidth();
      }
      else
      {
         totalPreferredWidth += el->GetPreferredWidth();
         totalCompactWidth += el->GetCompactWidth();
         totalMinWidth += el->GetMinWidth();
      }
   }

   //0 -> Room available. (Mod size/Selected -> Preferred Size | Spacing -> default)
   if (selectedPreferredWidth + hoveredPreferredWidth + totalPreferredWidth + totalSpacing < maxRowSize)
   {
      //Okay! There's still room.
      row->isFilled = false;
      row->isOverfilled = false;

      //Set the new positions
      for (int i = 0; i < row->elements.size(); ++i)
      {
         auto el = row->elements[i];
         row->elements[i]->SetRectRelativeToGrid(ofRectangle(offset, yOffset, MIN(maxRowSize, el->GetPreferredWidth()), mRowYSize));
         offset += el->GetPreferredWidth() + mElementXSpacing;
      }
      RowNotifyPostResize(index);
      return;
   }

   //Okay we'll have to get squeezy.

   //Note Spacing is treated as a module with min 0, compact 4.
   //We'll start by setting aside the space we'll dedicate for the selected module. Usually preferredSize, but exceptions may apply.
   float priorityModuleRatio = MIN(1, (maxRowSize - totalMinWidth) / (hoveredPreferredWidth + selectedPreferredWidth));
   float ratio;
   float spaceSize;
   int formula = 1;
   //Now we need to know which formula we'll apply for compression.

   //1 -> Squeezed, non-overfilled. (Mod size -> Preferred Size ~ Compact Size | Selected -> Preferred Size | Spacing -> default)
   if (selectedPreferredWidth + hoveredPreferredWidth + totalCompactWidth + totalSpacing <= maxRowSize)
   {

      float scaleRange = totalPreferredWidth - totalCompactWidth; //Aka difference
      ratio = (maxRowSize - totalSpacing - selectedPreferredWidth - hoveredPreferredWidth - totalCompactWidth) / scaleRange;
      spaceSize = mElementXSpacing;

      //Example, for posterity’s sake.
      //Row -> 150
      //Spacing -> 4
      //A -> Con 30 Pref 60
      //B -> Con 30 Pref 45
      //C -> Con 30 Pref 95
      //Ratio -> ?

      //Solution
      //Difference Total = 60-30 + 45-30 + 95-30 = 110
      //Ratio -> (Row - Spacing * (ElementCount-1) - ConTotal) / DifferenceTotal
      //Ratio = (150-4*(3-1)-90) / 110 = 0.472

      // A -> 30 + (60-30) * 0.472 = 44.16
      // B -> 30 + (45-30) * 0.472 = 37.08
      // C -> 30 + (95-30) * 0.472 = 60.68
      // To be correct, All 3 combined should be equal to 142

      //Solution: Remove the stuff that doesn't matter from the calculations.
   }
   //2 -> Squeezed, overfilled. (Mod size -> Compact Size ~ Min Size | Selected -> Preferred Size | Spacing -> default ~ 0)
   else if (hoveredPreferredWidth + selectedPreferredWidth + totalMinWidth <= maxRowSize)
   {
      formula = 2;

      float scaleRange = totalSpacing + totalCompactWidth - totalMinWidth;
      ratio = (maxRowSize - totalMinWidth - selectedPreferredWidth - hoveredPreferredWidth) / scaleRange;
      spaceSize = mElementXSpacing * ratio;
   }
   //3 -> Squeezed, minimum. (Mod size -> Min Size | Selected -> Constrained Size | Spacing -> 0)
   else
   {
      formula = 3;
      spaceSize = 0;
   }

   //Final setup
   offset = mRowXBorderOffset;
   for (int i = 0; i < elCount; ++i)
   {
      auto el = row->elements[i];
      float elWidth = 0;

      if (i == selectedIndex)
      {
         elWidth = selectedPreferredWidth * priorityModuleRatio;
      }
      else if (i == hoveredIndex)
      {
         elWidth = hoveredPreferredWidth * priorityModuleRatio;
      }
      else
      {
         if (formula == 1)
         {
            elWidth = el->GetCompactWidth() + (el->GetPreferredWidth() - el->GetCompactWidth()) * ratio;
         }
         else if (formula == 2)
         {
            elWidth = el->GetMinWidth() + (el->GetCompactWidth() - el->GetMinWidth()) * ratio;
         }
         else
         {
            elWidth = el->GetMinWidth();
         }
      }

      row->elements[i]->SetRectRelativeToGrid(ofRectangle(offset, yOffset, elWidth, mRowYSize));
      if (i + 1 < elCount)
         offset += elWidth + spaceSize;
   }

   if (updateFillState)
   {
      if (formula == 1)
      {
         row->isFilled = true;
         row->isOverfilled = false;
      }
      else
      {
         row->isFilled = true;
         row->isOverfilled = true;
      }
   }

   RowNotifyPostResize(index);
}

void FlowGrid::RowNotifyPostResize(int row) const
{
   for (auto r : mRows[row].elements)
   {
      r->OnPostResize();
   }
}

int FlowGrid::GetRowIndexOfElement(FlowGridElement* element) const
{
   if (element == nullptr)
      return -1;
   if (element->mPreferredRow != -1)
      return element->mPreferredRow;

   int idx = 0;
   for (auto row : mRows)
   {
      for (auto el : row.elements)
      {
         if (el == element)
            return idx;
      }
      idx++;
   }
   return -1;
}

void FlowGrid::RecalculateFlowGrid()
{
   if (mSkipGridRecalculation)
      return;
   //Updates ALL rows
   for (int i = 0; i < mRows.size(); ++i)
   {
      UpdateRow(i, true);
   }
}
void FlowGrid::ScheduleDeletion(FlowGridElement* element)
{
   DeregisterElement(element);
   mDisposalQueue.push_back(element);//Schedule for annihilation.
}
void FlowGrid::DeleteFlowElement(FlowGridElement* element)
{
   if (!element->IsDeleted())
   {
      DeregisterElement(element);
   }
   delete element;
}
void FlowGrid::DeregisterElement(FlowGridElement* element)
{
   element->MarkAsDeleted();
   bool erased = false;
   for (auto& r : mRows)
   {
      for (int i = 0; i < r.elements.size(); ++i)
      {
         if (r.elements[i] == element)
         {
            r.elements.erase(r.elements.begin() + i);
            erased = true;
            break;
         }
      }
      if (erased)
         break;
   }
   mElementList.erase(std::find(mElementList.begin(), mElementList.end(), element));
   mOwner->RemoveChild(element);
   RecalculateFlowGrid();
   CheckCleanupRows();
}
int FlowGrid::DisposeScheduled()
{
   int cleans = 0;
   while (!mDisposalQueue.empty())
   {
      const auto d = mDisposalQueue[mDisposalQueue.size() - 1];
      delete d;
      mDisposalQueue.pop_back();
      cleans++;
   }
   return cleans;
}

void FlowGrid::ReturnName(FlowNameAssigment* nAssign)
{
   for (auto rec : mFlowNameRecords)
   {
      if (rec.name == nAssign->internalName)
      {
         rec.freeIndexes.push_back(nAssign->index);
      }
   } //Marks the name index free for reuse.
   delete nAssign;
}

void FlowGrid::AddRow()
{
   mRows.push_back(FlowGridRow{ false, false });
   ResizeFlowGrid();
}

//Removes the last row. Any flow elements in it WILL be deleted.
void FlowGrid::PopRow()
{
   if (mRows.size() == 0 || mRows.size() <= mMinRows)
      return;

   for (int i = 0; i < mRows.back().elements.size(); ++i)
   {
      auto e = mRows.back().elements[i];
      mElementList.erase(std::find(mElementList.begin(), mElementList.end(), e));
   }
   mRows.pop_back();
   ResizeFlowGrid();
}

void FlowGrid::ResizeFlowGrid()
{
   float oldX = mWidth;
   float oldY = mHeight;
   SetDimensions(mWidth);
   mListener->onFlowGridResize(mWidth, mHeight, oldX, oldY);
}
void FlowGrid::InitAllFlowElements() const
{
   for (auto el : mElementList)
   {
      if (!el->IsInitialized())
      {
         el->Init();
      }
   }
}


void FlowGrid::SetSelectedGridElement(FlowGridElement* element)
{
   mSelectedElement = element;
   mListener->onFlowGridNewSelection(element);
}

FlowNameAssigment* FlowGrid::GetInternalNameForFlowElement(std::string name)
{
   FlowNameRecord* record = nullptr;
   for (int i = 0; i < mFlowNameRecords.size(); ++i) //Get a match
   {
      if (mFlowNameRecords[i].name == name)
      {
         record = &mFlowNameRecords[i];
      }
   }
   if (record == nullptr)
   {
      mFlowNameRecords.push_back(FlowNameRecord{
      name, 0, {} });
      return new FlowNameAssigment{ name + ofToString(0), name, 0 };
   }

   int idx;
   if (!record->freeIndexes.empty())
   {
      idx = record->freeIndexes.back();
      record->freeIndexes.pop_back();
   }
   else
   {
      record->index++;
      idx = record->index;
   }

   return new FlowNameAssigment{ name + ofToString(idx), name, idx };
}
void FlowGrid::SaveElements(FileStreamOut& out)
{
   out << static_cast<int>(mElementList.size());
   for (auto el : mElementList)
   {
      //Bespoke, in general, needs to have module Revs saved twice (1 in the save caller/the modules's SaveState() + another if IDrawable's Save() is called)
      //This is because IDrawable's Load extracts one rev and when combined with LoadState() requiring another separate, equal rev. The result
      //a double extract. If this promise is not kept, expect corruption/load exceptions being thrown.
      //This is why most >0 modules save the rev at the start of the SaveState(). Even though calling IDrawable's SaveState() already saves the rev.
      //TL DR: When saving, ALSO save the module's rev on your own.
      out << el->GetFlowGridElementType();
      out << el->GetModuleSaveStateRev();
      el->SaveState(out);
   }
}

//Takes in a factory and uses it to restore the elements within. Destroys the factory when done.
void FlowGrid::LoadElements(FlowGridElementFactory* factory, FileStreamIn& in)
{
   int elementCount = 0;
   in >> elementCount;
   for (int i = 0; i < elementCount; ++i)
   {
      int revCheck;
      std::string type;
      in >> type;
      int rev = 0;
      in.Peek(&revCheck, sizeof(int));
      if (revCheck != 0)//Rev 0 FGE's did not manually save the revision.
         in >> rev; //So we need to manually extract the revision so it lines up later.
      auto el = factory->Create(type);
      mListener->OnElementLoaded(el);
      el->SetName(el->mElementTypeName.c_str());
      el->SetTypeName(el->mElementTypeName, el->GetModuleCategory());
      el->CreateUIControls();
      el->Init();
      AddFlowElement(el, true);
      el->LoadState(in, rev);
   }
   delete factory;
}