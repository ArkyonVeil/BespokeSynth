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
/*
  ==============================================================================

    AudioSplitter.cpp
    Created: 26 Aug 2023 7:18:00am
    Author:  Noxy Nixie

  ==============================================================================
*/
//A note from ArkyonVeil: The comments in this file were written by myself, mainly to help me figure out how audio works.
//Later corrections are appreciated if I made any mistakes in my analysis.


#include "AudioSplitter.h"
#include "ModularSynth.h"
#include "Profiler.h"
#include "PatchCableSource.h"

//Constructor
//AudioSplitter inherits from IAudioProcessor, which itself inherits from both IAudioSource and IAudioReceiver
//gBufferSize is a global required to initialize audio capable elements. By default, 960, but can be changed by the user.
//Refers to the size of the buffers in each audio processing step.
//Larger buffers batch more processing per call, thus more efficient. But since they take longer to fill up, audio latency is increased.
AudioSplitter::AudioSplitter()
: IAudioProcessor(gBufferSize)
{
}

//Step designed for creating controls.
//However, this is still not yet a full initialization. Not included here is an override for Init() which is called after some behind the scenes setup.
//And even after that, is the LoadState(), which isn't a needed override here since the IDrawableModule automatically handles saving cables.
//That one is only called when loading a module from a save.
void AudioSplitter::CreateUIControls()
{
   IDrawableModule::CreateUIControls(); //Does quite a few generic things. If the panel is doing something automatically, its probably done here.

   //Since this class inherits AudioSource, the IDrawableModule automatically makes an audio cable for it.
   //This code draws the second one you see in a fresh AudioSplitter
   auto cable = new PatchCableSource(this, kConnectionType_Audio);
   AddPatchCableSource(cable);
   mDestinationCables.push_back(cable);
}

void AudioSplitter::Process(double time)
{
   //Used to register the processing time of this module for the audio CPU usage.
   PROFILER(AudioSplitter);

   //Syncs our module with attached module's buffers. Preparing them for data transfer. + Some situational cleanup
   SyncBuffers();
   //Get this module's input buffer's number of active(in use) channels. Mono signal -> 1, Stereo -> 2, Audiophile -> NaN
   const auto numchannels = GetBuffer()->NumActiveChannels();

   //Iterate through all our cables
   for (const auto cablesource : GetPatchCableSources())
   {
      //In most audio modules, you'll see the target being acquired by calling this->GetTarget(), but cables also work.
      const auto target = dynamic_cast<IAudioReceiver*>(cablesource->GetTarget());
      if (target) //If they have a valid target
      {
         //Get their input buffer.
         ChannelBuffer* out = target->GetBuffer();
         //Set the number of channels that we'll be using on their input. Note: SyncBuffers() already does this via SyncOutputBuffer() so its probably redundant.
         out->SetNumActiveChannels(numchannels);

         //Copy our input buffer's channel data to their input buffer.
         for (int ch = 0; ch < numchannels; ++ch)
         {
            Add(out->GetChannel(ch), GetBuffer()->GetChannel(ch), GetBuffer()->BufferSize());
         }
      }
   }

   //Updates all audio cables with our buffer data for animation purposes. Animating them
   for (int ch = 0; ch < GetBuffer()->NumActiveChannels(); ++ch)
   {
      GetVizBuffer()->WriteChunk(GetBuffer()->GetChannel(ch), GetBuffer()->BufferSize(), ch);
   }

   //Flushes and resets our input buffer. Don't forget to call this, or you might get exploding audio levels.
   GetBuffer()->Reset();
}
//Not as often seen is PreRepatch, which does the same, but it's before a patching operation.
void AudioSplitter::PostRepatch(PatchCableSource* cableSource, bool fromUserClick)
{
   //As of March/2026, this does nothing since the IAudioSource does not override the Patchable's PostRepatch, which is an empty virtual.
   IAudioSource::PostRepatch(cableSource, fromUserClick);

   //Go through each of our output cables.
   for (int i = 0; i < mDestinationCables.size(); ++i)
   {
      //If it's the cable
      if (mDestinationCables[i] == cableSource)
      {

         //If the cable is the rightmost (newest/top of the array).
         if (i == mDestinationCables.size() - 1)
         {
            //If it's attached to a module, we add a new cable next to it. Fulfilling the AudioSplitter's dynamic expansion.
            if (cableSource->GetTarget())
            {
               auto cable = new PatchCableSource(this, kConnectionType_Audio);
               AddPatchCableSource(cable);
               mDestinationCables.push_back(cable);
            }
         }//Otherwise we remove that cable, to resize the splitter back. (Works on any cable)
         else if (cableSource->GetTarget() == nullptr)
         {
            RemoveFromVector(cableSource, mDestinationCables);
            RemovePatchCableSource(cableSource);
         }

         break;
      }
   }
}

//Render stuff unique to the module, called every frame.
//Note: this is called from IDrawableModule's Render(), which does quite a few more things.
void AudioSplitter::DrawModule()
{
   if (Minimized() || IsVisible() == false)
      return;

   //Move all the output cables to one coord, we'll offset them later.
   GetPatchCableSource()->SetManualPosition(20, 12);

   int offset{ 20 };
   for (const auto cablesource : mDestinationCables)
   {
      //Iteration based offset
      cablesource->SetManualPosition(offset += 20, 12);//Wait, you can set values INSIDE a function call? Oh wow - Ark
      cablesource->SetOverrideCableDir(ofVec2f(0, 1), PatchCableSource::Side::kBottom);//Cables draw their lines going down, looks visually neat.
   }
}
//How big the module should be, automatically based on the number of cables it contains, give or take some hardcoded padding.
void AudioSplitter::GetModuleDimensions(float& w, float& h)
{
   //Bespoke has a couple definitions to automate basic math operations. Useful!
   w = MAX(80, 40 + (20 * mDestinationCables.size()));
   h = 5;
   //Also, this is called every frame, so any resizing logic is processed per draw cycle.
}

//Not used here, but this is used to add options to the module options in the header's topright arrow.
void AudioSplitter::LoadLayout(const ofxJSONElement& moduleInfo)
{
   //Calling this here is probably a bug since it's called anyway by the engine on loading/applied options.
   //Going to leave it here regardless since this commenting exercise is an analysis, not a "fixing."
   SetUpFromSaveData();
}

//Loads the options described beforehand.
void AudioSplitter::SetUpFromSaveData()
{
}
