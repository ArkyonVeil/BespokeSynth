/**
    bespoke synth, a software modular synthesizer
    Copyright (C) 2021 Ryan Challinor (contact: awwbees@gmail.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/
//
//  IAudioReceiver.h
//  modularSynth
//
//  Created by Ryan Challinor on 12/11/12.
//
//

#pragma once

#include "ChannelBuffer.h"

class IAudioReceiver
{
public:
   enum InputMode
   {
      kInputMode_Mono,
      kInputMode_Multichannel
   };

   IAudioReceiver(int bufferSize)
   : mInputBuffer(bufferSize)
   {}
   virtual ~IAudioReceiver() {}
   virtual ChannelBuffer* GetBuffer() { return &mInputBuffer; }//Returns this module's input buffer.
   virtual InputMode GetInputMode() { return kInputMode_Multichannel; }//Defines if this module expects mono, or more channels. (If mono, it will crunch multi-channel to mono)

protected:
   void SyncInputBuffer();

private:
   ChannelBuffer mInputBuffer;
};
