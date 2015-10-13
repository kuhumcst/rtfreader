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
#ifndef RTFSTUFF_H
#define RTFSTUFF_H
#include "data.h"
#include "paragraph.h"
#include "charsource.h"
//#include "commonstuff.h"

struct MSfont
    {
    int f;
    int * pos; // points at pos[224] array
    };

class rtfSource: public charSource
    {
    private:
        MSfont * fonttable;
        int lastfont;
        int Uchar(int f,int c);
        int TranslateToken(const char * token,int f);
        virtual bool writeparAfterCharacterPropertyChange(charprops oldprops,charprops newprops);
        int staticEat;
        int staticUc; // number of characters to "eat" after a \uNNNNN character has been read. (indicated by \ucN)
//Any RTF control word or symbol is considered a single character for the purposes of counting skippable characters.
    public:
        rtfSource(STROEM * sourceFile,paragraph * outputtext);
        ~rtfSource()
            {
            delete [] fonttable;
            }
        virtual bool eating()
            {
            if(staticEat > 0)
                {
                --staticEat;
                return true;
                }
            else
                return false;
            } 
        virtual bool escapeSequence(int ch,int f,bool & okToWrite);
        char * parseRTFtoken(int level);
        void resetCharProp(); // called after \pard
        int interpretToken(char * tok,int f);
        virtual void doTheSegmentation(charprops CharProps,bool newlineAtEndOffset,bool forceEndOfSegmentAfter);
        bool segment(int level
                    ,int sstatus
                    ,bool PrevIsField  // True if previous sibling block contains a \field
                    ,charprops CharProps
                    );
        void segmentdefault(int ch,int sstatus,charprops CharProps);
    };

#endif
