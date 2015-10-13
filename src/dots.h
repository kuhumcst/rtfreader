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
#ifndef ABBREVIATION_H
#define ABBREVIATION_H

#include "data.h"

struct flags;

class dots
    {
    private:
        int last;
        bool trailingDotFollowingNumber;
        bool ensureEmptyLine;
    public:
        dots():last(0),trailingDotFollowingNumber(false),ensureEmptyLine(false){}
        void Put3(STROEM * file,wint_t ch,flags & flgs); // called from PutN, Put2 and GetPut
        void PutN(STROEM * file,wchar_t * buf,size_t len,flags & flgs); // called from Put2
        const int getTrailingDotsFollowingNumber() const { return trailingDotFollowingNumber ? 1 : 0; }
    };

#endif