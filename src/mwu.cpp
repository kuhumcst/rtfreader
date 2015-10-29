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
#include "mwu.h"
#include "option.h"
#include "readlist.h"
#include <string.h>

static char * mwuX = NULL;
static char ** mwu = NULL; 
static int mwucnt = 0;
static size_t * mwulen;

static size_t MWU(char * p)
    {
    size_t ret = 0;
    for(int j = 0;j < mwucnt;++j)
        {
        if(  !strncmp(mwu[j],p,mwulen[j]) 
          && (  p[mwulen[j]] == ' ' 
             || p[mwulen[j]] == '\0'
             )
          )
            {
            if(ret < mwulen[j])
                ret = mwulen[j];
            for(char * t = p + mwulen[j] - 2;t > p;--t)
                {
                if(*t == ' ')
                    *t = '_';
                }
            }
        }
    return ret;
    }

static bool readMWU(FILE * a)
    {
    bool * dummy = NULL;
    bool res = readlist(a,mwuX,mwucnt,mwu,mwulen,dummy);
    delete [] dummy;
    return res;
    }

bool DoMultiWords(const char * argt)
    {
    if(Option.argm)
        {
        FILE * a = fopen(Option.argm,"rt");
        if(a)
            {
            readMWU(a);
            FILE * fpt = fopen(argt,"rb+");
            if(!fpt)
                {
                fprintf(stderr,"cstrtfreader: Error in opening output file %s\n",argt);
                return false;
                }
            int c/* = EOF*/;
            long pos = ftell(fpt);
                {
                do
                    {
                    char buf[1024];
                    size_t i = 0;
                    while((c = fgetc(fpt)) != EOF && c != '\n' && i + 1 < ((sizeof(buf)/sizeof(buf[0]))))
                        {
                        buf[i++] = (char)c;
                        }
                    buf[i] = '\0';
                    char * p = buf;
                    bool rewrite = false;
                        {
                        for(;;)
                            {
                            while(*p == ' ' || *p == (char)(unsigned char)0xa0)
                                ++p;
                            if(*p)
                                {
                                size_t len = MWU(p);
                                if(len)
                                    {
                                    rewrite = true;
                                    p += len;
                                    }
                                while(*p && *p != ' ' && *p != (char)(unsigned char)0xa0)
                                    ++p;
                                }
                            else
                                break;
                            }
                        }
                        long npos = ftell(fpt);
                        if(rewrite)
                            {
                            fseek(fpt,pos,SEEK_SET);
                            fwrite(buf,i,1,fpt);
                            fseek(fpt,npos,SEEK_SET);
                            }
                        pos = npos;
                    }
                    while(c != EOF);
                }
                fclose(fpt);
            }
        else
            {
            fprintf(stderr,"cstrtfreader: Error in opening multi-word-unit file %s\n",Option.arga);
            return false;
            }
        }
    return true;
    }

