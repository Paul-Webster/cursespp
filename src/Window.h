//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2007-2016 casey langen
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of the author nor the names of other contributors may
//      be used to endorse or promote products derived from this software
//      without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "config.h"
#include "IWindow.h"

#ifdef WIN32
#define IDLE_TIMEOUT_MS 0
#define REDRAW_DEBOUNCE_MS 100
#else
#define IDLE_TIMEOUT_MS 75
#define REDRAW_DEBOUNCE_MS 100
#endif

namespace cursespp {
    class IInput;

    class Window : public IWindow, public std::enable_shared_from_this<IWindow> {
        public:
            Window(IWindow* parent = NULL);
            virtual ~Window();

            virtual void SetParent(IWindow* parent);

            virtual void Show();
            virtual void Hide();

            virtual void Repaint();

            void SetFrameVisible(bool enabled);
            bool IsFrameVisible();

            virtual void Focus();
            virtual void Blur();

            virtual void SetContentColor(cursespp_int64 color);
            virtual void SetFrameColor(cursespp_int64 color);
            virtual void SetSize(int width, int height);
            virtual void SetPosition(int x, int y);
            virtual void MoveAndResize(int x, int y, int width, int height);

            virtual int GetWidth() const;
            virtual int GetHeight() const;
            virtual int GetContentHeight() const;
            virtual int GetContentWidth() const;
            virtual int GetX() const;
            virtual int GetY() const;
            virtual int GetId() const;

            virtual void BringToTop();
            virtual void SendToBottom();

            virtual void ProcessMessage(IMessage &message);

            virtual WINDOW* GetFrame() const;
            virtual WINDOW* GetContent() const;

            virtual int GetFocusOrder();
            virtual void SetFocusOrder(int order = -1);

            virtual bool IsVisible();
            virtual bool IsFocused();

            bool HasBadBounds() { return this->badBounds; }

            static void WriteToScreen(IInput* input);
            static void Invalidate();
            static void Freeze();
            static void Unfreeze();

        protected:
            IWindow* GetParent() const;

            void PostMessage(int messageType, cursespp_int64 user1 = 0, cursespp_int64 user2 = 0, cursespp_int64 delay = 0);
            void DebounceMessage(int messageType, cursespp_int64 user1 = 0, cursespp_int64 user2 = 0, cursespp_int64 delay = 0);
            void RemoveMessage(int messageType);

            void Create();
            void Destroy();
            void Recreate();
            void Clear();

            bool CheckForBoundsError();

            virtual void OnDimensionsChanged();
            virtual void OnVisibilityChanged(bool visible);
            virtual void OnFocusChanged(bool focused);

        private:
            IWindow* parent;
            PANEL* framePanel;
            WINDOW* frame;
            PANEL* contentPanel;
            WINDOW* content;
            bool badBounds;
            bool drawFrame;
            bool isVisible, isFocused;
            int focusOrder;
            int id;
            cursespp_int64 contentColor, frameColor;
            int width, height, x, y;
    };
}
