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
#include "readlist.h"

bool readlist(FILE * fpx,char *& X,int & cnt, char ** & elems, size_t * & len, bool * & Xtra)
    {
    fseek(fpx,0,SEEK_END);

    long lcnt = ftell(fpx);
    rewind(fpx);
    X = new char[lcnt+1];
    if(fread(X,lcnt,1,fpx) == 1)
        {
        cnt = 0;
        char * nxt;
        char * xx;
        for(xx = X;(nxt = strchr(xx,'\n')) != NULL;xx = nxt + 1)
            {
            ++cnt;
            }
        elems = new char*[cnt];
        len = new size_t[cnt];
        Xtra = new bool[cnt];
        cnt = 0;
        for(xx = X;(nxt = strchr(xx,'\n')) != NULL;xx = nxt + 1)
            {
            *nxt = '\0';
            if(nxt > xx && nxt[-1] == '\r')
                nxt[-1] = '\0';
            bool xtra = false;
            while(*xx == ' '|| *xx == '\t')
                {
                xtra = true;
                ++xx;
                }
            len[cnt] = strlen(xx);
            Xtra[cnt] = xtra;
            elems[cnt++] = xx;
            }
        return true;
        }
    elems = 0;
    len = 0;
    Xtra = 0;
    return false;
    }


