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
   mHeight = rowHeight * startNumRows;
   mRowYSize = rowHeight;
}
FlowGrid::~FlowGrid()
{
   for (auto el : mElementList)
   {
      RemoveFlowElement(el);
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
   if (mHovered && mHoveredElement != nullptr)
   {
      mSelectedElement = mHoveredElement;
      mStartDragMouse = ofVec2f(x, y);
      mRackPartDragGhostRect = mSelectedElement->GetRectRelativeToGrid();
      mPressed = true;
   }
   else
   {
      mSelectedElement = nullptr;
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
         break;
      }
   }
   if (mPressed == true && mStartDragMouse.distanceSquared(ofVec2f(x, y)) > 5)
   {
      mDragging = true;

      float elPX, elPY;
      elPX = mX + mRackPartDragGhostRect.x + x - mStartDragMouse.x;
      elPY = mY + mRackPartDragGhostRect.y + y - mStartDragMouse.y;
      elPX = CLAMP(elPX, mX, mX + mWidth - mSelectedElement->GetWidth());
      elPY = CLAMP(elPY, mY, mY + mHeight - mSelectedElement->GetHeight());
      mSelectedElement->SetPosition(elPX, elPY);
      //Go through the rows and find to the most likely place to snap to.
      int rowSnap = MAX(0, floor(y / (mRows.size() * mRowYSize + mRowYBorderOffset * 2) * mRows.size()));
      rowSnap = MIN(mRows.size() - 1, rowSnap);
      //TODO if we're at lowest row, spawn a new one if possible.

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
   if (mDragging)
   {
      mDragging = false;
      mSelectedElement->SetPosition(mX + mRackPartDragGhostRect.x, mY + mRackPartDragGhostRect.y);

      MoveToRow(mSelectedElement, mSnapDragRow, mSnapDragIndex);
   }
}

bool FlowGrid::MouseScrolled(float x, float y, float scrollX, float scrollY, bool isSmoothScroll, bool isInvertedScroll)
{
   return false;
}
void FlowGrid::SetDimensions(float width, float height)
{
   mWidth = width;
   mHeight = height;
   RecalculateFlowGrid();
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
   ofPopStyle();
   ofPopMatrix();

   //Draw in SongCanvas space.
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
   newElement->CreateUIControls();
   if (mOwner->IsInitialized())
   {
      newElement->Init();
   }
   //auto rec =  GetInternalNameForFlowElement(newElement->mElementTypeName);

   newElement->SetName(newElement->mElementTypeName.c_str());
   newElement->SetTypeName(newElement->mElementTypeName, kModuleCategory_Other);
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
                  ofLog() << "FlowGrid move rejected: sR" + ofToString(sourceRow) + " sI" + ofToString(sourceIndex) + " -> R" + ofToString(row) + " I" + ofToString(index);
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
      ofLog() << "FlowGrid move: sR" + ofToString(sourceRow) + " sI" + ofToString(sourceIndex) + " -> R" + ofToString(row) + " I" + ofToString(index);
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
         ofLog() << "FlowGrid move: sR" + ofToString(sourceRow) + " sI" + ofToString(sourceIndex) + " -> R" + ofToString(row) + " I" + ofToString(index);
         tIdx = mRows[sourceRow].elements.begin() + index;
      }
      else
      {
         ofLog() << "FlowGrid move: sR" + ofToString(sourceRow) + " sI" + ofToString(sourceIndex) + " -> R" + ofToString(row) + " I" + ofToString(index - 1);
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
         row->elements[i]->SetRectRelativeToGrid(ofRectangle(offset, yOffset, MIN(maxRowSize, el->GetPreferredWidth()), mRowYSize));
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

         el->SetRectRelativeToGrid(ofRectangle(offset, yOffset, eSize, mRowYSize));
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
      e->SetRectRelativeToGrid(ofRectangle(offset, yOffset, eSize, mRowYSize));
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
   //ReturnName(element->NameData);
   mOwner->RemoveChild(element);
   delete element;
   RecalculateFlowGrid();
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
   if (mRows.size() == 0)
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
   SetDimensions(mWidth, mRows.size() * mRowYSize);
   mListener->onFlowGridResize(mWidth, mRows.size() * mRowYSize);
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
      out << el->GetFlowGridElementType();
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
      std::string type;
      in >> type;
      auto el = factory->Create(type);
      el->LoadState(in, el->GetModuleSaveStateRev());
      AddFlowElement(el);
   }
   delete factory;
}
