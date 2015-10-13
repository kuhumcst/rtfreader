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
#include <string.h>
#include "abbrev.h"

#include "readlist.h"
#include "letterfunc.h"


static char * abbrX = NULL;
static char ** abbreviations = NULL; 
static int abbrcnt = 0;
static size_t * abblen;
static bool * Xtra = 0;


bool Abbreviation(const wchar_t * abbr, bool & expectCapitalizedWord)
    {
    int f;
    for(f = 0;f < abbrcnt;++f)
        {
        if(!strCaseCmp(abbr,abbreviations[f]))
            {
            if(Xtra)
                expectCapitalizedWord = Xtra[f];
            return true;
            }
        }
    return false;
    }

bool readAbbreviations(FILE * fpx)
    {
    return readlist(fpx,abbrX,abbrcnt,abbreviations,abblen,Xtra);
    }

