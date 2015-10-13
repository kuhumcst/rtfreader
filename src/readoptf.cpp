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
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "readoptf.h"

static int optionSets = 0;
static char *** Ppoptions = NULL;
static char ** Poptions = NULL;


void readOptsFromFile(char * locoptarg,char * progname,optionStruct & Option,optswitch doSwitch,const char opts[])
    {
    char ** poptions;
    char * options;
    FILE * fpopt = fopen(locoptarg,"r");
    if(fpopt)
        {
        char line[1000];
        int lineno = 0;
        size_t bufsize = 0;
        while(fgets(line,sizeof(line) - 1,fpopt))
            {
            lineno++;
            size_t off = strspn(line," \t");
            if(line[off] == ';')
                continue; // comment line
            if(line[off] == '-')
                {
                off++;
                if(line[off])
                    {
                    char * optarg2 = line + off + 1;
                    size_t off2 = strspn(optarg2," \t");
                    if(!optarg2[off2])
                        optarg2 = NULL;
                    else
                        optarg2 += off2;
                    if(optarg2)
                        {
                        for(char * p = optarg2 + strlen(optarg2) - 1;p >= optarg2;--p)
                            {
                            if(!isspace(*p))
                                break;
                            *p = '\0';
                            }
                        if(*optarg2 == '\'' || *optarg2 == '"')
                            {

                            // -x 'jhgfjhagj asdfj\' hsdjfk' ; dfaasdhfg
                            // -x 'jhgfjhagj asdfj\' hsdjfk' ; dfa ' asdhfg
                            // -x "jhgfjhagj \"asdfj hsdjfk" ; dfaasdhfg
                            // -x "jhgfjhagj \"asdfj hsdjfk" ; dfa " asdhfg
                            for(char * p = optarg2 + strlen(optarg2) - 1;p > optarg2;--p)
                                {
                                if(*p == *optarg2)
                                    {
                                    bool string = true;
                                    for(char * q = p + 1;*q;++q)
                                        {
                                        if(*q == ';')
                                            break;
                                        if(!isspace(*q))
                                            {
                                            string = false;
                                            }
                                        }
                                    if(string)
                                        {
                                        *p = '\0';
                                        ++optarg2;
                                        }
                                    break;
                                    }
                                }
                            }
                        if(!*optarg2)
                            optarg2 = NULL;
                        }
                    if(optarg2)
                        {
                        bufsize += strlen(optarg2) + 1;
                        }
                    const char * optpos = strchr(opts,line[off]);
                    if(optpos)
                        {
                        if(optpos[1] != ':')
                            {
                            if(optarg2)
                                {
                                printf("Option argument %s provided for option letter %c that doesn't use it on line %d in option file \"%s\"\n",optarg2,line[off],lineno,locoptarg);
                                exit(0);
                                }
                            }
                        }
                    }
                else
                    {
                    printf("Missing option letter on line %d in option file \"%s\"\n",lineno,locoptarg);
                    exit(0);
                    }
                }
            }
        rewind(fpopt);

        poptions = new char * [lineno];
        options = new char[bufsize];
        // update stacks that keep pointers to the allocated arrays.
        optionSets++;
        char *** tmpPpoptions = new char **[optionSets];
        char ** tmpPoptions = new char *[optionSets];
        int g;
        for(g = 0;g < optionSets - 1;++g)
            {
            tmpPpoptions[g] = Ppoptions[g];
            tmpPoptions[g] = Poptions[g];
            }
        tmpPpoptions[g] = poptions;
        tmpPoptions[g] = options;
        delete [] Ppoptions;
        Ppoptions = tmpPpoptions;
        delete [] Poptions;
        Poptions = tmpPoptions;

        lineno = 0;
        bufsize = 0;
        while(fgets(line,sizeof(line) - 1,fpopt))
            {
            poptions[lineno] = options+bufsize;
            size_t off = strspn(line," \t");
            if(line[off] == ';')
                continue; // comment line
            if(line[off] == '-')
                {
                off++;
                if(line[off])
                    {
                    char * optarg2 = line + off + 1;
                    size_t off2 = strspn(optarg2," \t");
                    if(!optarg2[off2])
                        optarg2 = NULL;
                    else
                        optarg2 += off2;
                    if(optarg2)
                        {
                        for(char * p = optarg2 + strlen(optarg2) - 1;p >= optarg2;--p)
                            {
                            if(!isspace(*p))
                                break;
                            *p = '\0';
                            }
                        if(*optarg2 == '\'' || *optarg2 == '"')
                            {

                            // -x 'jhgfjhagj asdfj\' hsdjfk' ; dfaasdhfg
                            // -x 'jhgfjhagj asdfj\' hsdjfk' ; dfa ' asdhfg
                            // -x "jhgfjhagj \"asdfj hsdjfk" ; dfaasdhfg
                            // -x "jhgfjhagj \"asdfj hsdjfk" ; dfa " asdhfg
                            for(char * p = optarg2 + strlen(optarg2) - 1;p > optarg2;--p)
                                {
                                if(*p == *optarg2)
                                    {
                                    bool string = true;
                                    for(char * q = p + 1;*q;++q)
                                        {
                                        if(*q == ';')
                                            break;
                                        if(!isspace(*q))
                                            {
                                            string = false;
                                            }
                                        }
                                    if(string)
                                        {
                                        *p = '\0';
                                        ++optarg2;
                                        }
                                    break;
                                    }
                                }
                            }
                        if(!*optarg2)
                            optarg2 = NULL;
                        }
                    if(optarg2)
                        {
                        strcpy(poptions[lineno],optarg2);
                        bufsize += strlen(optarg2) + 1;
                        }
                    /*else
                        optarg2 = "";
                    char * optpos = strchr(opts,line[off]);*/
                    doSwitch(line[off],poptions[lineno],progname,Option);
                    }
                }
            lineno++;
            }
        fclose(fpopt);
        }
    else
        {
        printf("Cannot open option file %s\n",locoptarg);
        }
    }
