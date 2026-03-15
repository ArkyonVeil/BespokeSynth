#pragma once

class IFlowGridListener
{
public:
   virtual ~IFlowGridListener() = default;

   virtual void FlowGridResizeRequest(float newBoundsX, float newBoundsY) {};//The grid is resizing in some direction, ignore, and it may clip.
};
