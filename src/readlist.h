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
#ifndef READLIST_H
#define READLIST_H

#include <stdio.h>

/*
20110224 Xtra: if line n in fpx starts with white-space (blank or tab), the
corresponding Xtra[n] is set to true. 
Used in abbreviations file to indicate that the next word is expected to start
with a capital letter.
*/
bool readlist(FILE * fpx,char *& X,int & cnt, char ** & elems, size_t * & len, bool * & Xtra);


#endif

