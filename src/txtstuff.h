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
#ifndef TXTSTUFF_H
#define TXTSTUFF_H
#include "data.h"

#include "charsource.h"
#include "parsesgml.h"

class paragraph;

class textSource: public charSource
    {
    private:
        html_tag_class html_tag;
        long tagendpos;
        bool isHTMLtagComing(wint_t ch);
        void copyEOLsequence();
    public:
        textSource(STROEM * sourceFile,paragraph * outputtext);
        virtual void doTheSegmentation(charprops CharProps,bool newlineAtEndOffset,bool forceEndOfSegmentAfter);
        virtual bool segment(int level
                    ,int sstatus
                    ,bool PrevIsField  // True if previous sibling block contains a \field
                    ,charprops CharProps
                    );
        void updateFlags(wint_t ch,flags & flgs);
    };

#endif
