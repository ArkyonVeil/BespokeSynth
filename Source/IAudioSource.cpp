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
//  IAudioSource.cpp
//  Bespoke
//
//  Created by Ryan Challinor on 12/16/15.
//
//

#include "IAudioSource.h"
#include "IAudioReceiver.h"
#include "PatchCableSource.h"


//Returns the IAudioReceiver module on that index, by default index 0. Should ALWAYS be an index that fits under the number of available audio output cables.
IAudioReceiver* IAudioSource::GetTarget(int index)
{
   assert(index < GetNumTargets());
   return GetPatchCableSource(index)->GetAudioReceiver();
}


void IAudioSource::SyncOutputBuffer(int numChannels)
{
   //Go through all the attached modules
   for (int i = 0; i < GetNumTargets(); ++i)
   {
      //If said module is in fact attached (not null)
      if (GetTarget(i))
      {
         //Gets the input buffer of the connected module.
         ChannelBuffer* out = GetTarget(i)->GetBuffer();
         //Sets the number of channels on the other module that we'll be using.
         out->SetNumActiveChannels(MAX(numChannels, out->NumActiveChannels()));
      }
   }
   GetVizBuffer()->SetNumChannels(numChannels);
}
