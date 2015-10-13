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
#include "argopt.h"
#include <string.h>

char *optarg;
int optind = 0;


int getopt(int argc,char *argv[],const char *opts)
    {
    static char emptystr[] = "";
    char *index/* = NULL*/;
    int optc/* = -1*/;
    
    if (!optind)    /* argv[0] points to the command verb */
        ++optind;
    if (optind >= argc)
        {
        optarg = NULL;
        return -1;
        }
    
    if ((index = argv[optind]) != NULL)
        {
        const char * optpos;
        if (*index != '-' && *index != '/')
            {
            /* no option, perhaps something else ? */
            optarg = NULL;
            return -1;
            }
        if (*(++index) == '-')
            {
            ++optind;            /* double --,  end of options */
            optarg = NULL;
            return -1;
            }
        if (!*index)
            {
                                /* single -, probably not an option */
            optarg = NULL;
            return -1;
            }
        optc = *index;       /* option letter */
        optpos = strchr(opts,optc);
        if(optpos)
            {
            if(optpos[1] == ':')
                {
                /* this option has always data */
                if(!*++index)
                    {
                    /* try next argument */
                    for (;++optind < argc && !*argv[optind];);
                    if(  optind == argc
                      || argv[optind] == NULL
                      || (  *argv[optind] == '-'
                         && *(argv[optind]+1) != '\0'
                         )
                      )
                        {
                        optarg = emptystr;
                        return optc;  /* no data after all */
                        }
                    else
                        {
                        optarg = argv[optind++]; 
                        return optc;
                        }
                    }
                else
                    {
                    optind++;
                    optarg = index; 
                    return optc;
                    }
                }
            else
                {
                optind++;
                optarg = NULL; 
                return optc;
                }
            }
        else
            {
            optind++;
            optarg = NULL; 
            return -1;
            }
        }
    else
        {
        optind++;
        optarg = NULL;
        return -1;
        }
    }
