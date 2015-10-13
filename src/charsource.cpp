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
#include "charsource.h"
#include "option.h"
#include "paragraph.h"
#include "commonstuff.h"
#include "parsesgml.h"
#include <assert.h>

bool isBullet(int ch)
    {
    return  (/*  (ch & 0x80) // ??????
            ||*/ (ch == '-') 
            || (ch == '*') 
            || (ch == 0x2022) // unicode BULLET
            || (ch == 0x2023) // unicode triangular bullet
            || (ch == 0x25E6) // unicode white bullet
            ||  ( (unsigned int )ch < ' '
                && ch != '\n'
                && ch != '\r'
                && ch != '\t'
                && ch != '\v'
                && ch != '\b'
                )
            );
    }

bool charSource::checkSentenceStartDueToBullet(int ch)
    {
    /* Look for bullets, can set begin_offset. */

    bool sentenceStart = false;

    if(isSpace(ch))
        {
        if(firsttext.EOL)
            {
            begin_offset = curr_pos;
            }
        if(flgs.bbullet)
            {
            sentenceStart = true; // force writing of segment
            //flgs.certainSentenceEndMarker = true;
            }
        }
    else if(flgs.bbullet)
        {
        flgs.bbullet = isBullet(ch);// The first bullet-like character is immediately followed by more bullet-like characters.
        }
    else 
        {
        if(firsttext.EOL)
            begin_offset = curr_pos;

        if(firsttext.b.LS)
            flgs.bbullet = isBullet(ch); // First character on line seems to indicate a bullet. Now see whether there comes a space next.
        }

    return sentenceStart;
    }


void charSource::Segmentation()
    {
//    testRomans();

    if(Option.A)
        pRegularizationFnc = pASCIIfyFunction;
    if(Option.ParseAsXml)
        parseAsXml();
    if(Option.ParseAsHtml)
        parseAsHtml();
    charprops CharProps;
    segment(0,0,false,CharProps);
    outputtext->Segment.Put(outputtext->file,0,flgs);
    outputtext->Segment.Put(outputtext->file,0,flgs);
    }

wint_t charSource::getput(int f)
    {
    bool okToWrite = true;
    wint_t ch = 0;
    while(Ftell(sourceFile) < end_offset - 1)
        {
        ch = Getc(sourceFile);
        if(!escapeSequence(ch,f,okToWrite))
            {
            if((ch != '\n' && ch != '\r') || flgs.inhtmltag)
                {
                if(!eating())
                    {
                    if(okToWrite)
                        outputtext->PutHandlingLine((wint_t)Uchar(f,ch),flgs);
                    }
                }
            }
        }
    if(flgs.htmltagcoming)
        {
        if(Option.tokenize)
            outputtext->PutHandlingLine(' ',flgs);
        flgs.inhtmltag = true;
        flgs.firstofhtmltag = true;
        flgs.htmltagcoming = false;
        }
    if(Ftell(sourceFile) < end_offset)
        {
        ch = Getc(sourceFile);
        if((ch != '\n' && ch != '\r') || flgs.inhtmltag)
            {
            if(okToWrite)
                outputtext->PutHandlingLine(ch,flgs);
            }
        }
    return ch;
    }


wint_t charSource::bulletStuff(int f)
    {
    static int nbullets = 0;
    wint_t ch = 0;
    long cur = Ftell(sourceFile);
    Fseek(sourceFile,begin_offset,_SEEK_SET);
    if(flgs.bbullet)
        {
        if(!Option.B)
            {
            if(Option.separateBullets)
                {
                if(nbullets < 5)
                    {
                    outputtext->PutHandlingLine('\n',flgs);
                    outputtext->PutHandlingLine('\n',flgs);
                    }
                ch = getput(f);
                if(Option.emptyline)
                    {
                    outputtext->PutHandlingLine('\n',flgs);
                    }

                if(nbullets < 5)
                    {
                    outputtext->PutHandlingLine('\n',flgs);
                    outputtext->PutHandlingLine('\n',flgs);
                    }
                flgs.bbullet = false;
                ++nbullets;
                }
            else
                {
                nbullets = 0;
                ch = getput(f);
                }
            }
        else if(Option.bullet)
            {
            if(Option.separateBullets)
                {
                if(nbullets < 5)
                    {
                    outputtext->PutHandlingLine('\n',flgs);
                    outputtext->PutHandlingLine('\n',flgs);
                    }

                while(Ftell(sourceFile) < end_offset)
                    {
                    ch = Getc(sourceFile);
                    }
                outputtext->PutHandlingLine(Option.bullet,flgs);
                outputtext->PutHandlingLine(' ',flgs);
                if(Option.emptyline)
                    {
                    outputtext->PutHandlingLine('\n',flgs);
                    }
                if(nbullets < 5)
                    {
                    outputtext->PutHandlingLine('\n',flgs);
                    outputtext->PutHandlingLine('\n',flgs);
                    }
                flgs.bbullet = false;
                ++nbullets;
                }
            else
                {
                nbullets = 0;
                while(Ftell(sourceFile) < end_offset)
                    {
                    ch = Getc(sourceFile);
                    }
                outputtext->PutHandlingLine(Option.bullet,flgs);
                outputtext->PutHandlingLine(' ',flgs);
                }
            }
        else
            {
            if(Option.separateBullets)
                {
                if(nbullets < 5)
                    {
                    outputtext->PutHandlingLine('\n',flgs);
                    outputtext->PutHandlingLine('\n',flgs);
                    }
                if(Option.emptyline)
                    {
                    outputtext->PutHandlingLine('\n',flgs);
                    }
                flgs.bbullet = false;
                ++nbullets;
                }
            else
                {
                nbullets = 0;
                }
            }
        }
    else
        {
        nbullets = 0;
        ch = getput(f);
        }
    Fseek(sourceFile,cur,_SEEK_SET);
    return ch;
    }
