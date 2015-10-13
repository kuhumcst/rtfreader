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
#ifndef COMMONSTUFF_H
#define COMMONSTUFF_H

#include "data.h"

#define MAXSEGMENTLENGTH 10000



//typedef wint_t (*lookahead_fnc)(STROEM * fp,const startLine firsttext,int f);


extern void regularizeWhiteSpace(STROEM * file,wint_t c,flags & flgs);
extern void ASCIIfy(STROEM * file,wint_t c,flags & flgs);
extern void ASCIIfyRTF(STROEM * file,wint_t c,flags & flgs);
extern void (*pRegularizationFnc)(STROEM * file,wint_t c,flags & flgs);
extern void (*pASCIIfyFunction)(STROEM * file,wint_t c,flags & flgs);

#endif
