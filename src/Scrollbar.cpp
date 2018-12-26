//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2007-2017 musikcube team
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

#include <algorithm>
#include <cursespp/Scrollbar.h>

using namespace cursespp;

void Scrollbar::Draw(ListWindow* list, Window* target) {
    int height = list->GetHeight();
    auto *adapter = &list->GetScrollAdapter();
    if (adapter && height > 2) {
        auto& pos = list->GetScrollPosition();

        /* these are defaults for drawing to an external view, that
        is, when target != list. */
        int xOffset = 0;
        int from = 0, to = height;
        WINDOW* frame = nullptr;
        float range = (float) height;
        size_t minY = 0;

        if (!target || target == list) {
            /* if we're drawing on top of the ListWindow's frame,
            we need to account for the padding it provides. */
            frame = list->GetFrame();
            xOffset = list->GetWidth() - 1;
            ++from; --to;
            range -= 2.0f;
            minY = 1;
        }
        else {
            frame = target->GetFrame();
        }

        float total = (float) std::max(minY, adapter->GetEntryCount());

        int yOffset;
        if (range >= total) {
            yOffset = -1;
        }
        else {
            float percent = (float)pos.logicalIndex / total;
            yOffset = (int)(range * percent) + minY;
        }

        for (int i = from; i < to; i++) {
            wmove(frame, i, xOffset);
            if (i == yOffset) wattron(frame, A_REVERSE);
            waddch(frame, (i == yOffset) ? ' ' : ACS_VLINE);
            if (i == yOffset) wattroff(frame, A_REVERSE);
        }
    }
}
