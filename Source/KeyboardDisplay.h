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
//  KeyboardDisplay.h
//  Bespoke
//
//  Created by Ryan Challinor on 5/12/16.
//  Tweaked by ArkyonVeil on April/2024
//

#ifndef __Bespoke__KeyboardDisplay__
#define __Bespoke__KeyboardDisplay__

#include "HoverSelectModule.h"
#include "IDrawableModule.h"
#include "NoteEffectBase.h"


#include <unordered_map>

class KeyboardDisplay : public NoteEffectBase, public HoverSelectModule
{
public:
   KeyboardDisplay();
   static IDrawableModule* Create() { return new KeyboardDisplay(); }
   void OnHoverEnter() override;
   void OnHoverExit() override;
   void OnSelect() override;
   void OnDeselect() override;
   void Exit() override;
   static bool AcceptsAudio() { return false; }
   static bool AcceptsNotes() { return true; }
   static bool AcceptsPulses() { return false; }

   void CreateUIControls() override;

   void SetEnabled(bool enabled) override;

   //INoteReceiver
   void PlayNote(double time, int pitch, int velocity, int voiceIdx = -1, ModulationParameters modulation = ModulationParameters()) override;

   void MouseReleased() override;
   void KeyPressed(int key, bool isRepeat) override;
   void KeyReleased(int key) override;


   void LoadLayout(const ofxJSONElement& moduleInfo) override;
   void SetUpFromSaveData() override;
   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, int rev) override;
   int GetModuleSaveStateRev() const override { return 1; }

   bool IsEnabled() const override { return mEnabled; }

private:
   //IDrawableModule
   void DrawModule() override;
   void GetModuleDimensions(float& width, float& height) override
   {
      width = mWidth;
      height = mHeight;
   }
   void OnClicked(float x, float y, bool right) override;
   bool IsResizable() const override { return true; }
   void Resize(float w, float h) override;
   void RefreshOctaveCount();

   void DrawKeyboard(int x, int y, int w, int h);
   void SetPitchColor(int pitch);
   ofRectangle GetKeyboardKeyRect(int pitch, int w, int h, bool& isBlackKey) const;

   int RootKey() const;
   int NumKeys() const;

   float mWidth{ 500 };
   float mHeight{ 110 };
   int mRootOctave{ 3 };
   int mNumOctaves{ 3 };
   int mForceNumOctaves{ 0 };
   int mPlayingMousePitch{ -1 };
   bool mTypingInput{ false };
   bool mLatch{ false };
   bool mShowScale{ false };
   bool mGetVelocityFromClickHeight{ false };
   bool mHideLabels{ false };
   int mMidPress = 0;
   std::array<float, 128> mLastOnTime{};
   std::array<float, 128> mLastOffTime{};
   std::unordered_map<int, int> mKeyPressRegister{};
};

#endif /* defined(__Bespoke__KeyboardDisplay__) */
