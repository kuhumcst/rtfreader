/*
CSTRTFREADER - read flat text or rtf and output flat text, 
               one line per sentence, optionally tokenised

Copyright (C) 2012  Center for Sprogteknologi, University of Copenhagen

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
/*

segment[Text]                                                       Recursively traverse RTF segment tree.
  |                                                                 Keep track of character properties (bold, italic,
  |                                                                 small caps, font size)
  |                                                                 Returns true if segment is a field. False otherwise.
  |                                                                 Sets begin and end offsets for candidate segments, 
  |                                                                 based on the interpretation of RTF tags such as \par
  |
  |                                                                 In case of flat text input: detect well-formed HTML-tags.
  |
  +--segmentdefault[Text]                                           Tries to spot sentence endings by looking at interpunction.
       |                                                                Knows of filenames, ellipsis ...
       +--doTheSegmentation[Text]
            | |
            | +--GetPutBullet                                       Skip rest of segment. Print bullet character and a blank.
            |               |
            +--GetPut[Text] |                                       Read segment. Convert rtf-tokens to their character equivalents.
            |     |         |                                       Suppress optional hyphens.
            |     |         |
            +-----+---------+--PutHandlingLine                      Handle a line, detecting noise
                                 |
                                 +--PutHandlingWordWrap             Handle hyphens at end of line
                                 |    |
                                 +----+--Put                        Handle newlines
                                           |
                                           +--Put2                  Handle quotes, brackets and abbreviations
                                                |
                                                +--PutN             Repeated call of Put3 on whole buffer
                                                |    |
                                                +----+--Put3        Start new sentence if an abbreviation is followed by a capital 
                                                          |         letter in a new segment
                                                          |
                                                          +--Put4   Optionally translate 8-bit set characters to combinations 
                                                                    of 7-bit characters
*/
/*This source is also used in OCRtidy*/

#include "commonstuff.h"
#include <string.h>
#include <assert.h>
#include "option.h"
#include "charconv.h"
#include "parsesgml.h"
#include "txtstuff.h"

static wint_t GetPutText(const long end_offset,flags & flgs)
    {
//    static bool skipSpace = false;
    wint_t ch = 0;
    while(Ftell(sourceFile) < end_offset - 1)
        {
        ch = Getc(sourceFile);
        if((ch != '\n' && ch != '\r') || flgs.inhtmltag)
            {
#if TEST
//            putchar('_');
#endif
            PutHandlingLine(ch,flgs);
            }
        }
    if(flgs.htmltagcoming)
        {
        if(Option.tokenize)
            PutHandlingLine(' ',flgs);
        flgs.inhtmltag = true;
        flgs.firstofhtmltag = true;
        flgs.htmltagcoming = false;
        }
    if(Ftell(sourceFile) < end_offset)
        {
        ch = Getc(sourceFile);
        if((ch != '\n' && ch != '\r') || flgs.inhtmltag)
            {
#if TEST
//            putchar('!');
#endif
            PutHandlingLine(ch,flgs);
            }
        }
    return ch;
    }

static int doTheSegmentationText(const bool nl,bool & oldnl,const long end_offset,long & begin_offset,flags & flgs,bool & WritePar,long & targetFilePos)
    {
    int ret = 0; // 0 if space
    if(-1L < begin_offset && begin_offset < end_offset)
        {
//        flgs.newSegment = true; // Signal to Put3 to let it handle sentence-ending abbreviations
        long cur = Ftell(sourceFile);
        
        static bool myoldnl = 0;

        bool writepar = false;
        wint_t ch = 0;
        if(!oldnl)
            oldnl = myoldnl;
        if(oldnl)
            {
            PutHandlingLine(' ',flgs);
            }
        Fseek(sourceFile,begin_offset,_SEEK_SET);

        if(writepar || WritePar)
            {
//            if(oldnl)
                {
                if(Option.emptyline)
                    {
                    PutHandlingLine('\n',flgs);
                    }
                PutHandlingLine('\n',flgs);
                PutHandlingLine('\n',flgs);
                }
            WritePar = false;
            }
        myoldnl = nl;
        static int nbullets = 0;
        if(flgs.bbullet)
            {
            if(!Option.B)
                {
                if(Option.separateBullets)
                    {
                    
                    if(nbullets < 5)
                        {
                        PutHandlingLine('\n',flgs);
                        PutHandlingLine('\n',flgs);
                        }
                    ret = ch = GetPutText(end_offset,flgs);
                    if(Option.emptyline)
                        {
                        PutHandlingLine('\n',flgs);
                        ret = 0;
                        }
                    
                    if(nbullets < 5)
                        {
                        PutHandlingLine('\n',flgs);
                        PutHandlingLine('\n',flgs);
                        ret = 0;
                        }
                    flgs.bbullet = false;
                    ++nbullets;
                    }
                else
                    {
                    nbullets = 0;
                    ch = GetPutText(end_offset,flgs);
                    ret = 1;
                    }
                }
            else if(Option.bullet)
                {
                if(Option.separateBullets)
                    {
                    if(nbullets < 5)
                        {
                        PutHandlingLine('\n',flgs);
                        PutHandlingLine('\n',flgs);
                        }
                    ret = ch = GetPutBullet(end_offset,flgs);
                    if(Option.emptyline)
                        {
                        PutHandlingLine('\n',flgs);
                        ret = 0;
                        }
                    if(nbullets < 5)
                        {
                        PutHandlingLine('\n',flgs);
                        PutHandlingLine('\n',flgs);
                        ret = 0;
                        }
                    flgs.bbullet = false;
                    ++nbullets;
                    }
                else
                    {
                    nbullets = 0;
                    ret = ch = GetPutBullet(end_offset,flgs);
                    }
                }
            else
                {
                if(Option.separateBullets)
                    {
                    if(nbullets < 5)
                        {
                        PutHandlingLine('\n',flgs);
                        PutHandlingLine('\n',flgs);
                        }
                    if(Option.emptyline)
                        {
                        PutHandlingLine('\n',flgs);
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
            /*
            if(flgs.htmltagcoming)
                {
                ret = ch = GetPutText(end_offset,flgs);
                }
            else
                {
                ret = ch = GetPutText(end_offset,flgs);
                }*/
            ret = ch = GetPutText(end_offset,flgs);
            }

        if(nl)
            {
            if(ch == '\n' || ch == '\r')
                {
                PutHandlingLine(ch,flgs);
                }
            else
                {
                PutHandlingLine('\n',flgs);
                }
            ret = 0;
            }
        else
            {
            switch(ch)
                {
                case ';':
                case '?':
                case '.':
                case '!':
#if COLONISSENTENCEDELIMITER
                case ':':
#endif
                case '\n':
                    PutHandlingLine('\n',flgs);
                    ret = 0;
                    break;
                default:
                    {
                    }
                }
            }
        targetFilePos = end_offset;
        Fseek(sourceFile,cur,_SEEK_SET);
        begin_offset = end_offset;//-1L; 20040120 There are no characters to be excluded between end_offset and begin_offset
        oldnl = false;
        flgs.bbullet = false;
        }
    return ret && !isSpace(ret);
    }

static wint_t lookaheadText(STROEM * fp,const startLine firsttext,int f)
    {
    wint_t next = Getc(fp);
    Ungetc(next,fp);
    return next;
    }

void copyEOLsequence(flags flgs)
/*
Record the way lines are separated in the input. With this recording,
the same pattern can be applied to the output.
*/
    {
    wint_t ch;
    while((ch = Getc(sourceFile)) != '\n' && ch != '\r' && ch != WEOF && ch != 26)
        {
        }
    if(ch == '\n')
        {
        flgs.firstEOLchar = '\n';
        if(Getc(sourceFile) == '\r')
            flgs.secondEOLchar = '\r';
        }
    else if(ch == '\r')
        {
        flgs.firstEOLchar = '\r';
        if(Getc(sourceFile) == '\n')
            flgs.secondEOLchar = '\n';
        }
    else // Assume DOS-format
        {
        flgs.firstEOLchar = '\r';
        flgs.secondEOLchar = '\n';
        }
    Frewind(sourceFile);
    }

bool isHTMLtagComing(wint_t ch,html_tag_class & html_tag, long & tagendpos)
    {
    bool ret = false;
    if(ch != WEOF && ch != 26 && (html_tag.*tagState)(ch) == tag) 
        {

        long curr_pos = Ftell(sourceFile);
        //assert(html_tag.*tagState == &html_tag_class::lt);
        estate Seq = notag;
        do
            {
            ch = Getc(sourceFile);
            if(ch == WEOF || ch == 26)
                {
                Seq = notag;
                break;
                }
            Seq = (html_tag.*tagState)(ch);
            }
        while(Seq == tag || Seq == endoftag_startoftag);
        /*notag,tag,endoftag,endoftag_startoftag*/
        if(Seq == notag)
            { // Not an HTML tag. Backtrack.
            }
        else // endoftag
            { // ch == '>'
            assert(ch == '>');
            tagendpos = Ftell(sourceFile);// position of first char after '>'
            //ch = Getc(sourceFile);
            ret = true;
            }
        Fseek(sourceFile,curr_pos,_SEEK_SET);
        }
    return ret;
    }

bool isHeading(startLine & firsttext,wint_t ch,bool & WritePar, flags & flgs)//,bool heading)
    {
    if(  (firsttext.b.CR && ch == '\r')
      || (firsttext.b.LF && ch == '\n')
      ) // two times LF or two times CR implicates empty line!
        WritePar = true; // Bart 20050714. headings weren't set 

    if(flgs.nrNonSpaceBytes > 0)
        {
        if(flgs.hyphens == 0)
            {
            if(flgs.allcaps)  // CONSISTENCY
                {
                flgs.heading = true;
                WritePar = true;
                }
            if(flgs.allNumber)  // 10   10.     10.2    xi
                {
                flgs.heading = true;
                WritePar = true;
                }

            /*
            */
            if(flgs.startcaps)  // How I Did Feed My Pet
                {
                flgs.heading = true;
                WritePar = true;
                }
            if(flgs.nrStartCaps > flgs.nrNoStartCaps) // Discussion of Experimental Proof for the Paradox of Einstein, Rosen, and Podolsky
                {
                flgs.heading = true;
                WritePar = true;
                }
            /*
            */
            /*
            if(flgs.nrStartCaps + flgs.nrNoStartCaps < 2) // Foundations of physics
                {
                heading = true;
                WritePar = true;
                }
            */
            /*
            if(flgs.nrNonSpaceBytes < 15) // kartofler
                {
                heading = true;
                WritePar = true;
                }
            */
            }
        else
            flgs.hyphens = 0;
        }
    /*
    */
    /*        flgs.allcaps = true;
    flgs.startcaps = true;*/
    /*
    if(!WritePar && heading)
        {// A normal line after a heading has WritePar==false and heading==true
        WritePar = heading;
        heading = false;
        }
    */
    flgs.nrNonSpaceBytes = 0;
    return flgs.heading;
    }

bool skipSegmentation(startLine & firsttext,wint_t ch)
    {
    bool skipSeg = false;
    if(ch == '\n')
        {
        if(!firsttext.b.LF)
            {
            firsttext.b.LF = 1;
            if(firsttext.b.CR)
                skipSeg = true;
            }
        }
    else if(ch == '\r')
        {
        if(!firsttext.b.CR)
            {
            firsttext.b.CR = 1;
            if(firsttext.b.LF)
                skipSeg = true;
            }
        }
    else
        {
        firsttext.b.CR = 1;
        firsttext.b.LF = 1;
        }
    return skipSeg;
    }

void updateFlags(wint_t ch,flags & flgs)
    {
    if(ch == '-')
        flgs.hyphens++;
    else
        flgs.hyphens = 0;
    if(!flgs.nrNonSpaceBytes)
        {
        flgs.wordComing = true;
       //evidently trivial: flgs.nrNonSpaceBytes = 0;
        flgs.nrNoStartCaps = 0;
        if(flgs.hyphenFound)
            {
            /*
            h har netop været vært for en institut-
            dag på Institut for Medier, Erkendelse
            */
            flgs.nrStartCaps = -10;
            flgs.allcaps = false;
            flgs.startcaps = false;
            }
        else
            {
            flgs.nrStartCaps = 0;
            flgs.allcaps = true;
            flgs.startcaps = true;
            }
        flgs.allNumber = true;
        flgs.lowerRoman = false;
        flgs.upperRoman = false;
        flgs.arabic = false;
        }
    if(isFlatSpace(ch))
        {
        flgs.wordComing = true;
        }
    else 
        {
        if(/*isUpper*/!isLower(ch))
            {
            if(flgs.wordComing)
                ++flgs.nrStartCaps;
            if(flgs.allNumber)
                {
                if(!flgs.lowerRoman && !flgs.arabic && strchr("IVXLCDM",ch))
                    {
                    flgs.upperRoman = true;
                    }
                else
                    {
                    flgs.allNumber = false;
                    }
                }
            }
        else
            {
            if(!strchr("ivxlcdm-/().:0123456789",ch))
                flgs.allNumber = false;
            if(flgs.allNumber)
                {
                if(strchr("-/().:",ch))
                    {
                    flgs.lowerRoman = false;
                    flgs.upperRoman = false;
                    flgs.arabic = false;
                    }
                else
                    {
                    if(flgs.upperRoman)
                        {
                        flgs.allNumber = false;
                        }
                    else if(!flgs.arabic && strchr("ivxlcdm",ch))
                        {
                        flgs.lowerRoman = true;
                        }
                    else if(!flgs.lowerRoman && strchr("0123456789",ch))
                        {
                        flgs.arabic = true;
                        }
                    else
                        {
                        flgs.allNumber = false;
                        }
                    }
                }
            if(flgs.wordComing && !flgs.allNumber) // 'iv. The Big Dipper' should be o.k.
                {
                flgs.startcaps = false;
                ++flgs.nrNoStartCaps;
                }
            if(!flgs.allNumber)
                flgs.allcaps = false; // 'iv. THE BIG DIPPER' should be o.k.
            }
        flgs.nrNonSpaceBytes++;
        flgs.wordComing = false;
        }
    }


bool TextSegmentation(STROEM * sourceFile,STROEM * targetFile)
    {
    if(Option.parseAsXml)
        parseAsXml();
    if(Option.parseAsHtml)
        parseAsHtml();
    ::sourceFile = sourceFile;
    ::outputtext = targetFile;
    flags flgs;
    startLine firsttext = {{1,0,0,1}}; // SD, LF, CR, WS
    long targetFilePos = 0L;
    bool oldnl = false;
    bool WritePar = false;
    wint_t ch;
    long begin_offset = 0; // The position that indicates the first character that has not been written to output.
    //long end_offset = 0;   // When calling doTheSegmentationText, this indicates the last position to be written.
    long tagendpos = 0; // position of first char after '>'
    long curr_pos = Ftell(sourceFile);// After parsing a html-tag, seeking to curr_pos brings you back to the position where the parsed sequence started.                   
    html_tag_class html_tag;

    if(Option.keepEOLsequence)
        {
        copyEOLsequence(flgs);
        }

    do
        {
        ch = Getc(sourceFile);
        long new_pos = Ftell(sourceFile);
        /** /
        if(ch > 256)
            printf("%C %x %d (ftell %ld)\n",ch,ch,ch,new_pos);
            //*/
        if(curr_pos >= tagendpos)
            {
            // We are not inside an HTML-tag.
            if(flgs.inhtmltag)
                {
                flgs.firstafterhtmltag = true;
                flgs.inhtmltag = false;
                }
            // Check whether a well-formed HTML tag is ahead.
            flgs.htmltagcoming = isHTMLtagComing(ch,html_tag,tagendpos);
            }
        else if(flgs.htmltagcoming)
            {
            // We are leaving an HTML-tag and entering a new one.
            flgs.inhtmltag = true;
            flgs.htmltagcoming = false;
            }
        /* Scan in advance, checking whether the line to come is a heading and 
        therefore must be preceded with a newline (WritePar will then be set to 
        true.)
        */
        //long end_offset = curr_pos;
        //static bool in_fileName = false;
        //static bool wordComing = true;
        //flgs.wordComing = true;
        //static bool heading = false; 
        // Used to indicate that a new line must be put in the stream because we've just seen a heading
        //putchar(ch);

        if(  ch ==  '\n'
          || ch == '\r'
          || ch == WEOF
          || ch == 26
          )
            {
            flgs.in_fileName = false;
            flgs.heading = isHeading(firsttext,ch,WritePar, flgs);//,heading);
            if(!skipSegmentation(firsttext,ch))
                {
                doTheSegmentationText(true,oldnl,new_pos,begin_offset,flgs,WritePar,targetFilePos); // Bart 20040120. true because: Suppose that end of line is end of segment
                if(!WritePar && flgs.heading)
                    {// A normal line after a heading has WritePar==false and heading==true
                    WritePar = flgs.heading;
                    flgs.heading = false;
                    }
                }
            if(firsttext.EOL) // Bart 20061214
                firsttext.b.LS = 1;
            }
        else
            {
            updateFlags(ch,flgs);
            int EOL = firsttext.EOL;
            bool sentenceEnd = checkSentenceEnd(ch,begin_offset,lookaheadText,firsttext,curr_pos,flgs);
            if(  sentenceEnd 
              || flgs.htmltagcoming
              || flgs.inhtmltag
              || (new_pos - begin_offset > MAXSEGMENTLENGTH && isSpace(ch)) // check for buffer overflow
              )
                {
                doTheSegmentationText(false,oldnl,new_pos,begin_offset,flgs,WritePar,targetFilePos);
                firsttext.b.SD = 1;
                firsttext.b.LS = 0;
                }
            if(isSpace(ch))
                {
                if(EOL)
                    firsttext.b.LS = 1;
                }
            else
                {
                firsttext.b.LS = 0;
                }
            firsttext.EOL = 0; // resets SD, CR and LF
            }
        curr_pos = new_pos;
        }
    while(ch != WEOF && ch != 26);
    PutHandlingLine('\n',flgs); // 20100106 Flush last line
//    flushLine('\n',flgs); // 20080104

    Put2(0,flgs);
    return closeStreams();
    }
