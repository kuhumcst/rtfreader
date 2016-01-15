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
#ifndef CHARSOURCE_H
#define CHARSOURCE_H

#include <wchar.h>
#include "data.h"
#include "flags.h"

class paragraph;
enum eStatus { uninitialised = -3, notSpecified = -2, mixed = -1, Off = 0, On = 1 };

union startLine
    {
    struct
        {
        unsigned int SD:1; // set at start of text and upon seeing segment delimiter (bullet, full stop, colon, ...)
        unsigned int LF:1;
        unsigned int CR:1;
        unsigned int LS:1; // Leading space
        } b;
    unsigned int EOL:3; // SD, LF and CR together
    };


class charprops
    {
    public:
        eStatus i;
        eStatus b;
        eStatus scaps;
        int fs;
        int f;
        int uc;
        charprops():i(uninitialised),b(uninitialised),scaps(uninitialised),fs(uninitialised),f(uninitialised),uc(1){}
    };

class charSource
    {
    protected:
        STROEM * sourceFile;
        paragraph * outputtext;
        flags flgs;
        long begin_offset;
        long curr_pos;
        long end_offset;
        startLine firsttext;
        bool oldnl;
        bool WriteParAfterHeadingOrField;
        bool checkSentenceStartDueToBullet(int ch);
    public:
        charSource(STROEM * sourceFile,paragraph * outputtext)
            :sourceFile(sourceFile)
            ,outputtext(outputtext)
            ,begin_offset(0)
            ,curr_pos(0)
            ,end_offset(0)
            ,oldnl(false)
            ,WriteParAfterHeadingOrField(false)
            {
            firsttext.b.SD = 1; // set at start of text and upon seeing segment delimiter (bullet, full stop, colon, ...)
            firsttext.b.LF = 0;
            firsttext.b.CR = 0;
            firsttext.b.LS = 1; // Leading space
            }
        wint_t getput(int f);
        virtual int Uchar(int f,int c){return c;}
        virtual bool eating(){return false;} 
        virtual bool escapeSequence(int ch,int f,bool & okToWrite){return false;}
        wint_t bulletStuff(int f);
        virtual bool writeparAfterCharacterPropertyChange(charprops oldprops,charprops newprops){return false;}
        virtual void doTheSegmentation(charprops CharProps,bool newlineAtEndOffset,bool forceEndOfSegmentAfter) = 0;
        void Segmentation();
        virtual bool segment(int level
                    ,int sstatus
                    ,bool PrevIsField  // True if previous sibling block contains a \field
                    ,charprops CharProps
                    ) = 0;
    };
#endif