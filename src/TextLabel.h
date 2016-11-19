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
#include "Window.h"
#include "IInput.h"
#include "Text.h"
#include "utfutil.h"
#include "sigslot.h"

namespace cursespp {
    class TextLabel :
#if (__clang_major__ == 7 && __clang_minor__ == 3)
        public cursespp::Window,
        public std::enable_shared_from_this<TextLabel> {
#else
        public cursespp::Window {
#endif
    public:
        TextLabel();
        virtual ~TextLabel();

        virtual void SetText(
            const std::string& value,
            const text::TextAlign alignment = text::AlignLeft);

        virtual std::string GetText() { return this->buffer; }
        virtual size_t Length() { return u8cols(this->buffer); }

        virtual void Show();

    private:
        std::string buffer;
        text::TextAlign alignment;
    };
}
