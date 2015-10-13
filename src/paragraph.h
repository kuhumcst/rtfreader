/*
CSTRTFREADER - read flat text or rtf and output flat text, 
               one line per sentence, optionally tokenised

Copyright (C) 2015  Center for Sprogteknologi, University of Copenhagen

This file is part of CSTRTFREADER.

CSTRTFREADER is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

CSTRTFREADER is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CSTRTFREADER; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#if !defined PARAGRAPH_H
#define PARAGRAPH_H

#include "data.h"
#include "segment.h"

class paragraph
    {
    public:
        segment Segment;
    private:
        wint_t MindTheSpace;
        wint_t lastEOLchar;

        int last; // last character that was not space or tab (we ignore trailing spaces and tabs)
        int wait;
        bool spaceAfterHyphen;
        bool dropHyphen;
        bool allLower;
        bool allUpper;
        int lastWordIndex;
        int waited;
        size_t Index;
        wchar_t Line[256];
        wchar_t circularBuffer[256];
        const size_t ind(int i) const
            {
            i = i % (sizeof(circularBuffer)/sizeof(circularBuffer[0]));
            if(i < 0)
                i += (sizeof(circularBuffer)/sizeof(circularBuffer[0]));
            return (size_t)i;
            }
        const size_t inc(size_t i) const
            {
            if(i == (sizeof(circularBuffer)/sizeof(circularBuffer[0])) - 1)
                return 0;
            else
                return i + 1;
            }
        const size_t dec(size_t i) const
            {
            if(i == 0)
                return (sizeof(circularBuffer)/sizeof(circularBuffer[0])) - 1;
            else
                return i - 1;
            }
        const size_t index() const{return Index;}
        const wchar_t * line() const{return Line;}
        void hyphenate(flags & flgs);
        void considerHyphenation(flags & flgs);
        void PutHandlingWordWrap(const wint_t ch,flags & flgs);
    public:
        STROEM * file;
        void flushLine(wint_t ch,flags & flgs);
        bool overflowing(){return Index >= sizeof(Line)/sizeof(Line[0]) - 1;}
        void append(wchar_t ch);
        void PutHandlingLine(wint_t ch,flags & flgs); // Called from GetPut, GetPutBullet and doTheSegmentation
        paragraph(STROEM * target):MindTheSpace(0),lastEOLchar(0),last('\n'),
            wait(0),spaceAfterHyphen(false),dropHyphen(false),allLower(false),
            allUpper(false),lastWordIndex(0),waited(0),Index(0),file(target){}
    };

#endif