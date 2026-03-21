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


FlowGrid::FlowGrid(std::string name, int x, int y, int w, int rowHeight, int startNumRows, IDrawableModule* owner, IFlowGridListener* listener)
{
   SetName(name.c_str());
   SetPosition(x, y);
   mListener = listener;
   mOwner = owner;
   SetParent(owner);
   mMinRows = startNumRows;
   mWidth = w;
   mHeight = rowHeight * startNumRows;
   mRowYSize = rowHeight;

   SetShouldDrawOutline(false);
}
FlowGrid::~FlowGrid()
{
   delete mLocalContainer;
}
void FlowGrid::CreateUIControls()
{
   IDrawableModule::CreateUIControls();
   mLocalContainer = new ModuleContainer();
   mLocalContainer->SetOwner(this);
   for (int i = mRows.size(); i < mMinRows; i++)
      AddRow();
}

void FlowGrid::OnClicked(float x, float y, bool right)
{
   if (mHovered && mLastHoveredElement != nullptr)
   {
      if (mSelectedElement != nullptr)
         mSelectedElement->SetHighlight(false);
      mSelectedElement = mLastHoveredElement;
      mSelectedElement->SetHighlight(true);
      mLastHoveredElement->OnClicked(x,y,right);
      mStartDragMouse = ofVec2f(x + GetPosition().x, y + GetPosition().y);
      mStartDragElementPos = mSelectedElement->GetRelativePosition();
      if (mSelectedElement->GetHovered())
         mDragToken = true;
      mPressed = true;
   }
}

void FlowGrid::MouseReleased()
{
   mDragToken = false;
   mPressed = false;
   if (mDragging)
   {
      /*
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

      RecalculateFlowGrid();

      //mSelectedElement = nullptr;
      //mLastHoveredElement = nullptr;
      mDragging = false;*/
   }
}
bool FlowGrid::MouseMoved(float x, float y)
{
   IDrawableModule::MouseMoved(x, y);

   float rX = x + mX;
   float rY = y + mY;

   bool isMouseOver = (x >= 0 && x < mWidth && y >= 0 && y < mHeight);

   //TheSynth->LogEvent("MouseMove "+std::to_string(mDebugIter),kLogEventType_Verbose);
   //mDebugIter++;
   //See which element we're hovering over...
   /*
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
*/
   return false;
}
bool FlowGrid::MouseScrolled(float x, float y, float scrollX, float scrollY, bool isSmoothScroll, bool isInvertedScroll)
{
   return false;
}
void FlowGrid::SetHighlightCol(double time, int col)
{
}
int FlowGrid::GetHighlightCol(double time) const
{
   return 0;
}
void FlowGrid::SetDimensions(float width, float height)
{
   mWidth = width;
   mHeight = height;
   RecalculateFlowGrid();
}

void FlowGrid::DrawModule()
{
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
}
void FlowGrid::Render()
{
   IDrawableModule::Render();
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
void FlowGrid::AddFlowElement(FlowGridElement* newElement)
{
   newElement->SetFlowGrid(this);

   int r = TryGetSlot();

   assert(r != -1); //If you fail this, the grid has been specified to not have enough room, but no check was done to prevent this.
   //Now we have a rogue class object and nowhere to put it . <>(
   if (r == -1)
   {
      throw std::exception("Error: Tried to push in a new element to an already full FlowGrid.");
   }

   //Add it to the pipeline
   AddChild(newElement);
   mLocalContainer->AddModule(newElement);
   newElement->CreateUIControls();
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

//Updates cached data, and visuals. Does not trigger a resize.
void FlowGrid::UpdateRow(int index, bool updateFillState)
{
   if (mRows.size() <= index)
      return; //???

   if (mRows[index].elements.empty())
   {
      mRows[index].isFilled = false;
      mRows[index].isOverfilled = false;
      return;
   }

   FlowGridRow* row = &mRows[index];

   float totalPreferredWidth = 0; //How much we can stuff it before it starts to compress
   float totalMinimumWidth = 0; //Minimum size to lerp to, not necessarily the smallest possible size.
   float offset = mRowXBorderOffset;
   float totalSpacing = 0;

   int selectedIndex = -1; //If one is selected, in the squeeze step if applicable, we set it to its largest possible size.
   float selectedPreferredSize;

   float maxRowSize = mWidth - mRowXBorderOffset * 2;
   int yOffset = mRowYBorderOffset + mRowYSize * index + mElementYSpacing * index;


   //Get maximum size of all elements.
   //Also the minimum size.
   for (int i = 0; i < row->elements.size(); ++i)
   {
      auto el = row->elements[i];

      if (mSelectedElement == el)
      {
         selectedIndex = i;
         selectedPreferredSize = el->GetPreferredWidth();
         totalMinimumWidth += el->GetPreferredWidth(); //If selected, it renders at its maximum size.
      }
      else
      {
         totalPreferredWidth += el->GetPreferredWidth();
         totalMinimumWidth += el->GetMinimumWidth();
         totalSpacing += mElementXSpacing;
      }
   }

   if (totalPreferredWidth + totalSpacing < mWidth - mRowXBorderOffset * 2)
   {
      //Okay! There's still room.

      row->isFilled = false;
      row->isOverfilled = false;

      //Set the new positions
      for (int i = 0; i < row->elements.size(); ++i)
      {
         auto el = row->elements[i];
         row->elements[i]->SetRect(ofRectangle(offset, yOffset, MIN(maxRowSize, el->GetPreferredWidth()), mRowYSize));
         offset += el->GetPreferredWidth() + mElementXSpacing;
      }
      return;
   }
   //Okay we'll have to get squeezy.

   //Squeezy is much more complicated, to make it work, we get the oversize ratio and some multiplication.


   //First, for convenience's sake, we'll check the worst case scenario first. IE, minimum total is still larger than available row space.
   if (totalMinimumWidth + totalSpacing > maxRowSize)
   {
      //If this happens, we scale evenly down.
      //IE we start at minimum and down we go.
      //Don't forget the special case, it still renders at full size even in an overfilled scenario.
      offset = mRowXBorderOffset;
      float ratio = totalMinimumWidth / maxRowSize; //TODO, inspect this later, it may be wrong due to assumptions of a full sized selected element.
      for (int i = 0; i < row->elements.size(); ++i)
      {
         auto el = row->elements[i];
         float eSize;
         if (mSelectedElement != el)
         {
            eSize = el->GetMinimumWidth() / ratio;
         }
         else
         {
            eSize = el->GetPreferredWidth() / ratio;
         }

         el->SetRect(ofRectangle(offset, yOffset, eSize, mRowYSize));
         offset += eSize + mElementXSpacing;
      }

      if (updateFillState)
      {
         row->isFilled = true;
         row->isOverfilled = true;
      }
      return;
   }

   //TODO Test the block below.
   //This is the difficult one, where we squeeze items together based on a consistent ratio.
   //Apply formula: t = (S - (totalMinimumWidth)) / ((totalPreferredWidth - totalMinimumWidth)
   float availableSpace = maxRowSize - totalSpacing;
   float t = (availableSpace - totalMinimumWidth) / (totalPreferredWidth - totalMinimumWidth);
   offset = mRowXBorderOffset;
   for (int i = 0; i < row->elements.size(); ++i)
   {
      auto e = row->elements[i];
      float eSize;
      if (mSelectedElement == e)
      {
         //Selected element uses its preferred size
         eSize = e->GetPreferredWidth();
      }
      else
      {
         //Interpolate between minimum and preferred based on t
         eSize = e->GetMinimumWidth() + t * (e->GetPreferredWidth() - e->GetMinimumWidth());
      }
      e->SetRect(ofRectangle(offset, yOffset, eSize, mRowYSize));
      offset += eSize + mElementXSpacing;
   }
   if (updateFillState)
   {
      row->isFilled = true;
      row->isOverfilled = true;
   }
}


void FlowGrid::RecalculateFlowGrid()
{
   //Updates ALL rows
   for (int i = 0; i < mRows.size(); ++i)
   {
      UpdateRow(i, true);
   }
}
void FlowGrid::RemoveFlowElement(FlowGridElement* element)
{
   mElementList.erase(std::find(mElementList.begin(), mElementList.end(), element));
   RemoveChild(element);
   RecalculateFlowGrid();
}

void FlowGrid::AddRow()
{
   mRows.push_back(FlowGridRow{ false, false });
   ResizeFlowgrid();
}

//Removes the last row. Any flow elements in it WILL be deleted.
void FlowGrid::PopRow()
{
   if (mRows.size() == 0)
      return;

   for (int i = 0; i < mRows.back().elements.size(); ++i)
   {
      auto e = mRows.back().elements[i];
      mElementList.erase(std::find(mElementList.begin(), mElementList.end(), e));
      RemoveChild(e);
   }
   mRows.pop_back();
   ResizeFlowgrid();
}

void FlowGrid::ResizeFlowgrid()
{
   SetDimensions(mWidth, mRows.size() * mRowYSize);
   mListener->onFlowGridResize(mWidth, mRows.size() * mRowYSize);
}


void FlowGrid::SetSelectedGridElement(FlowGridElement* element)
{
   mSelectedElement = element;
   mListener->onFlowGridNewSelection(element);
}
