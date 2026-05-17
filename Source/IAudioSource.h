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
//  IAudioSource.h
//  modularSynth
//
//  Created by Ryan Challinor on 11/22/12.
//
//

#pragma once

#include "RollingBuffer.h"
#include "SynthGlobals.h"
#include "IPatchable.h"

class IAudioReceiver;

#define VIZ_BUFFER_SECONDS .1f

class IAudioSource : public virtual IPatchable
{
public:
   IAudioSource()
   : mVizBuffer(VIZ_BUFFER_SECONDS * gSampleRate) //Initialize the visual buffer.
   {}
   virtual ~IAudioSource() {}
   //Called by the Audio Processing thread to do audio work. Here, work is done resetting audio buffers, writing to attached modules buffers, applying effects to samples, etc.
   //At a rate of 48000hz/48k samples per second, and a default buffer size of 256. Expect this to run 187.5 times per second.
   virtual void Process(double time) = 0;
   IAudioReceiver* GetTarget(int index = 0);
   virtual int GetNumTargets() { return 1; }
   RollingBuffer* GetVizBuffer() { return &mVizBuffer; } //Gets this module's visual buffer for animating audio cables and other FX.

protected:
   void SyncOutputBuffer(int numChannels);

private:
   RollingBuffer mVizBuffer;
};
