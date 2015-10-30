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
            | 
            +--GetPut[Text]                                         Read segment. Convert rtf-tokens to their character equivalents.
            |     |                                                 Suppress optional hyphens.
            |     |          
            +-----+------------PutHandlingLine                      Handle a line, detecting noise
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
                                                          +--regularizeWhiteSpace   Optionally translate 8-bit set characters to combinations 
                                                                    of 7-bit characters
*/
/*This source is also used in OCRtidy*/
#include "data.h"

#include "commonstuff.h"
#include <string.h>
#include <assert.h>
#include "option.h"
#include "letterfunc.h"
#include "parsesgml.h"
#include "txtstuff.h"
#include "paragraph.h"
#include "flags.h"
#include "segment.h"
#include "charsource.h"


static bool wordComing = true;
static bool allcaps = false;//true;
static bool allNumber = false;//true;
static bool lowerRoman = false; // Roman number
static bool upperRoman = false; // Roman number
static bool arabic = false; // True if number is represented with arabic ciffres (0..9)
static bool heading = false;
static bool lastCharIsSemipunct = false;
static int nrStartCaps = 0;
static int nrNoStartCaps = 0;
static int nrNonSpaceBytes = 0;
static int hyphens = 0; // If a line in all other respects seems a header, if line ends with a single hyphen, it is no header after all.




textSource::textSource(STROEM * sourceFile,paragraph * outputtext):charSource(sourceFile,outputtext),tagendpos(0)
    {
    pASCIIfyFunction = ASCIIfy;
    }


void textSource::doTheSegmentation(charprops CharProps,bool newlineAtEndOffset,bool forceEndOfSegmentAfter)


    {
    if(-1L < begin_offset && begin_offset < end_offset)
        {








        static bool myoldnl = 0;
        if(!oldnl)
            oldnl = myoldnl;
        if(oldnl)
            {


            outputtext->PutHandlingLine(' ',flgs);
            }


        if(WriteParAfterHeadingOrField)
            {

                {


                if(Option.emptyline)
                    {
                    outputtext->PutHandlingLine('\n',flgs);
                    }
                outputtext->PutHandlingLine('\n',flgs);
                outputtext->PutHandlingLine('\n',flgs);
                }
            WriteParAfterHeadingOrField = false;
            }
        myoldnl = newlineAtEndOffset;

        wint_t ch = bulletStuff(Off);
        begin_offset = end_offset;//-1L; 20040120 There are no characters to be excluded between end_offset and begin_offset

        if(forceEndOfSegmentAfter)
            {
            outputtext->PutHandlingLine('\n',flgs);
            outputtext->PutHandlingLine('\n',flgs);
            }
        else if(newlineAtEndOffset)
            {
            if(ch == '\n' || ch == '\r')
                outputtext->PutHandlingLine(ch,flgs);
            else
                outputtext->PutHandlingLine('\n',flgs);
            }
        else
            {
            switch(ch)
                {
                case ';':
                case '?':

                case '!':
#if COLONISSENTENCEDELIMITER
                case ':':
#endif

                    outputtext->PutHandlingLine('\n',flgs);
                    break;
                case '\n':
                    outputtext->PutHandlingLine('\n',flgs);
                    break;
                default:
                    {
                    }
                }
            }
        oldnl = false;
        flgs.bbullet = false;


        }
    }


void textSource::copyEOLsequence()
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


bool textSource::isHTMLtagComing(wint_t ch)
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
            ret = true;
            }
        Fseek(sourceFile,curr_pos,_SEEK_SET);
        }
    return ret;
    }

static bool isHeading(startLine & firsttext,wint_t ch,bool & WritePar)
    {
    if(  (firsttext.b.CR && ch == '\r')
      || (firsttext.b.LF && ch == '\n')
      ) // two times LF or two times CR implicates empty line!
        WritePar = true;

    if(nrNonSpaceBytes > 0)
        {
        if(hyphens == 0)
            {
            if(!lastCharIsSemipunct)
                {
                if(allcaps)  // CONSISTENCY
                    {
                    heading = true;
                    WritePar = true;
                    }
                if(allNumber)  // 10   10.     10.2    xi
                    {
                    heading = true;
                    WritePar = true;
                    }
                if(/*nrStartCaps > 1 &&*/ nrStartCaps > nrNoStartCaps) // Discussion of Experimental Proof for the Paradox of Einstein, Rosen, and Podolsky
                    {
                    heading = true;
                    WritePar = true;
                    }
                }
            }
        else
            hyphens = 0;
        }
    nrNonSpaceBytes = 0;
    return heading;
    }

static bool skipSegmentation(startLine & firsttext,wint_t ch)
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
    else // WEOF or 26
        {
        firsttext.b.CR = 1;
        firsttext.b.LF = 1;
        }
    return skipSeg;
    }

typedef enum 
    {  I_
    ,  I 
    , II_
    , II 
    ,III 
    , IV 
    ,  V_
    ,  V 
    , VI_
    , VI 
    ,VII_
    , IX 

    ,  X_
    ,  X 
    , XX_
    , XX 
    ,XXX 
    , XL 
    ,  L_
    ,  L 
    , LX_
    , LX 
    ,LXX_
    , XC_
    ,ONE 

    ,  C_
    ,  C 
    , CC_
    , CC 
    ,CCC 
    , CD 
    ,  D_
    ,  D 
    , DC_
    , DC 
    ,DCC_
    , CM_
    ,TEN 

    ,  M_
    ,  M 
    , MM_
    , MM 
    ,MMM 
    ,CEN 
    ,END 
    ,  F
    ,  T
    } R;

static unsigned int parseRoman(unsigned int k)
    {
    static unsigned int Romans[] =
        {
/*   I_*/ 'i',I  ,F   ,     // i..
/*   I */  0 ,T  ,II_ ,     // i
/*  II_*/ 'i',II ,IV  ,     // ii..
/*  II */  0 ,T  ,III ,     // ii
/* III */ 'i',END,F   ,     // iii
/*  IV */ 'v',END,IX  ,     // iv
/*   V_*/ 'v',V  ,I_  ,     // v..
/*   V */  0 ,T  ,VI_ ,     // v
/*  VI_*/ 'i',VI ,F   ,     // vi..
/*  VI */  0 ,T  ,VII_,     // vi
/* VII_*/ 'i',II ,F   ,     // vii..
/*  IX */ 'x',END,F   ,
/*   X_*/ 'x',X  ,V_  ,     // x..
/*   X */  0 ,T  ,XX_ ,     // x
/*  XX_*/ 'x',XX ,XL  ,     // xx..
/*  XX */  0 ,T  ,XXX ,     // xx
/* XXX */ 'x',ONE,ONE ,     // xxx..
/*  XL */ 'l',ONE,XC_ ,     // xl..
/*   L_*/ 'l',L  ,X_  ,     // l..
/*   L */  0 ,T  ,LX_ ,     // l
/*  LX_*/ 'x',LX ,ONE ,     // lx..
/*  LX */  0 ,T  ,LXX_,     // lx
/* LXX_*/ 'x',XX ,ONE ,     // lxx.. 
/*  XC_*/ 'c',ONE,ONE ,     // XC..
/* ONE */  0 ,T  ,V_  ,     // 1-9
/*   C_*/ 'c',C  ,L_  ,     // c..
/*   C */  0 ,T  ,CC_ ,     // c
/*  CC_*/ 'c',CC ,CD  ,     // cc..
/*  CC */  0 ,T  ,CCC ,     // cc
/* CCC */ 'c',TEN,TEN ,     // ccc..
/*  CD */ 'd',TEN,CM_ ,     // cd..
/*   D_*/ 'd',D  ,C_  ,     // d..
/*   D */  0 ,T  ,DC_ ,     // d
/*  DC_*/ 'c',DC ,TEN ,     // dc..
/*  DC */  0 ,T  ,DCC_,     // dc
/* DCC_*/ 'c',CC ,TEN ,     // dcc.. 
/*  CM_*/ 'M',TEN,TEN ,     // cm..
/* TEN */  0 ,T  ,L_  ,     // 10-99
/*   M_*/ 'm',M  ,D_  ,     // m..
/*   M */  0 ,T  ,MM_ ,     // m
/*  MM_*/ 'm',MM ,D_   ,     // mm..
/*  MM */  0 ,T  ,MMM ,     // mm
/* MMM */ 'm',CEN,CEN ,     // mmm..
/* CEN */  0 ,T  ,D_  ,     // 1-9
/* END */  0 ,T ,F
        };
    static unsigned int index = M_;
    static bool Upper = false;

    if(k == 128)
        {
        index = M_;
        return M_;
        }

    if(index == T || index == F)
        {
        index = F;
        return F;
        }

    if(k <= ' ')
        k = 0;

    if(index == M_)
        {
        if(strchr("IVXLCDM",k))
            {
            Upper = true;
            k = lowerEquivalent(k);
            }
        else if(strchr("ivxlcdm",k))
            Upper = false;
        else
            {
            index = F;
            return F;
            }
        }
    else if(Upper)
        {
        if(!strchr("IVXLCDM",k))
            {
            index = F;
            return F;
            }
        k = lowerEquivalent(k);
        }
    else
        {
        if(!strchr("ivxlcdm",k))
            {
            index = F;
            return F;
            }
        }

    for(;index != T && index != F;)
        {
        if(k == Romans[3*index])
            {
            index = Romans[3*index+1];
            break;
            }
        else
            {
            index = Romans[3*index+2];
            }
        }

    return index;
    }
/*
static bool testRoman(const char * rom)
    {
    int result;
    parseRoman(128);
    for(const char * p = rom;*p;++p)
        {
        result = parseRoman(*p);
        if(result == T)
            return true;
        else if(result == F)
            return false;
        }
    result = parseRoman(0);
    if(result == T)
        return true;
    else
        return false;
    }

static void testRomans()
    {
    const char * numbers[] =
        {""
        ,"i"
        ,"ii"
        ,"iii"
        ,"iv"
        ,"v"
        ,"vi"
        ,"vii"
        ,"viii"
        ,"ix"
        ,"x"
        ,"xi"
        ,"xiv"
        ,"xix"
        ,"mmcdxcviii"
        ,"MMCDXCVIII"
        ,"ci"
        ,"mi"
        ,"viv"
        ,"viiii"
        ,"xxc"
        ,"ic"
        ,"lil"
        ,"mil"
        ,"MMMCDXLIV"
        ,"MMMCDxliv"
        ,0
        };
    for(const char ** q = numbers;*q;++q)
        {
        printf("%s\t:",*q);
        if(testRoman(*q))
            printf("OK\n");
        else
            printf("..\n");
        }
    }
*/
void textSource::updateFlags(wint_t ch,flags & flgs)
    {

    if(ch == '-')
        hyphens++;
    else
        hyphens = 0;
    if(!nrNonSpaceBytes)
        {
        if(!isUpper(ch) && !allNumber)
/*
1. 
omejitev  odvisno  od.

==>

1 .
omejitev odvisno od .

because the 'header' 1. seems to be a list (or section) number. 
It is not likely starting a sentence, even though the first
character is lower case.
*/
            WriteParAfterHeadingOrField = false;
        wordComing = true;
        lastCharIsSemipunct = false;
       //evidently trivial: nrNonSpaceBytes = 0;
        nrNoStartCaps = 0;
        if(  flgs.hyphenFound
          || flgs.semiPunctuationFound
          || (  (  (  !flgs.punctuationFound
                   && !isUpper(ch)
                   )
                || flgs.in_abbreviation
                || flgs.person_name == initial
                ) 
             && flgs.writtentoline
             )
          )
            {
            /*
            h har netop været vært for en institut-
            dag på Institut for Medier, Erkendelse
            */
            nrStartCaps = -10; // Smaller chance that line starting here is a headline.
            allcaps = false;
            }
        else
            {
            nrStartCaps = 0;
            allcaps = true;
            }

        allNumber = !isFlatSpace(ch);
        lowerRoman = false;
        upperRoman = false;
        arabic = false;
        parseRoman(128);
        }
    if(isFlatSpace(ch))
        {
        wordComing = true;
        int result = parseRoman(0);
        if(result == F)
            {
            upperRoman = false;
            lowerRoman = false;
            }
        parseRoman(128);
        }
    else 
        {
        lastCharIsSemipunct = isSemiPunct(ch);
        if(!isLower(ch))
            {
            if(wordComing)
                {
                if(isUpper(ch))
                    ++nrStartCaps;
                }
            if(allNumber)
                {
                if(!lowerRoman && !arabic && strchr("IVXLCDM",ch))
                    {
                    int result = parseRoman(ch);
                    allNumber = upperRoman = (result != F);
                    }
                else
                    {
                    allNumber = false;
                    }
                }
            }
        else
            {
            if(!strchr("ivxlcdm-/().:0123456789",ch))
                allNumber = false;
            if(allNumber)
                {
                if(strchr("-/().:",ch))
                    {
                    lowerRoman = false;
                    upperRoman = false;
                    arabic = false;
                    }
                else
                    {
                    if(upperRoman)
                        {
                        allNumber = false;
                        }
                    else if(!arabic && strchr("ivxlcdm",ch))
                        {
                        int result = parseRoman(ch);
                        allNumber = lowerRoman = (result != F);                            
                        }
                    else if(!lowerRoman && strchr("0123456789",ch))
                        {
                        arabic = true;
                        }
                    else
                        {
                        allNumber = false;
                        }
                    }
                }
            if(wordComing && !allNumber) // 'iv. The Big Dipper' should be o.k.
                {
                if(!isUpper(ch))
                    ++nrNoStartCaps;
                }
            if(!allNumber)
                allcaps = false; // 'iv. THE BIG DIPPER' should be o.k.
            }
        nrNonSpaceBytes++;
        wordComing = false;
        }
    }

bool textSource::segment(int level
                        ,int sstatus
                        ,bool PrevIsField  // True if previous sibling block contains a \field
                        ,charprops CharProps
                        )
    {

    wint_t ch;
    curr_pos = Ftell(sourceFile);// After parsing a html-tag, seeking to curr_pos brings you back to the position where the parsed sequence started.                   

    if(Option.keepEOLsequence)
        {
        copyEOLsequence();
        // SourceFile is rewinded
        }

    do
        {
        ch = Getc(sourceFile);
        end_offset = Ftell(sourceFile);
        if(curr_pos >= tagendpos)
            {
            // We are not inside an HTML-tag.
            if(flgs.inhtmltag)
                {
                flgs.firstafterhtmltag = true;
                flgs.inhtmltag = false;
                }
            // Check whether a well-formed HTML tag is ahead. Returns sourceFile
            // in same file position.
            flgs.htmltagcoming = isHTMLtagComing(ch);
//            assert(new_pos == Ftell(sourceFile));
            assert(end_offset == Ftell(sourceFile));
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

        if(  ch ==  '\n'
          || ch == '\r'
          || ch == WEOF
          || ch == 26
          )
            {
            flgs.in_fileName = false;
            heading = isHeading(firsttext,ch,WriteParAfterHeadingOrField);
            if(!skipSegmentation(firsttext,ch))
                {
                doTheSegmentation(CharProps,true,false); // Bart 20040120. true because: Suppose that end of line is end of segment
                if(!WriteParAfterHeadingOrField && heading)
                    {// A normal line after a heading has WritePar==false and heading==true
                    WriteParAfterHeadingOrField = true;
                    heading = false;
                    }
                }
            if(firsttext.EOL)
                firsttext.b.LS = 1;
            }
        else
            {
            updateFlags(ch,flgs);
            int EOL = firsttext.EOL;
            bool sentenceEnd = checkSentenceStartDueToBullet(ch);
            if(  sentenceEnd 
              || flgs.htmltagcoming
              || flgs.inhtmltag
              || (end_offset - begin_offset > MAXSEGMENTLENGTH && isSpace(ch)) // check for buffer overflow
              )
                {
                doTheSegmentation(CharProps,false,false); 
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
                firsttext.EOL = 0; // resets SD, CR and LF
                }
            }
        curr_pos = end_offset;
        }
    while(ch != WEOF && ch != 26);
    outputtext->PutHandlingLine('\n',flgs); // 20100106 Flush last line
    return false;
    }

