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
#include "paragraph.h"
#include "flags.h"
#include "option.h"
//#include "commonstuff.h"
#include "segment.h"
#include "letterfunc.h"
#include "ocrtidy.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>

void paragraph::hyphenate(flags & flgs)
    {
    Segment.Put(file,'-',flgs);
    Segment.Put(file,'\n',flgs);
    for(int j = 0;j != waited;++j)
        Segment.Put(file,BufferForHandlingEndOfLineHyphens[j],flgs);
    }

void paragraph::considerHyphenation(flags & flgs)
/*
Requires option -w-

Drop hyphen:
ab-
ces
-> abces    Same case before and after hyphen

AB-
CES
-> ABCES    Same case before and after hyphen

Don't drop hyphen
ab-
Ces
-> ab-Ces  Not all lowercase after hyphen

AB-
Ces
-> AB-Ces  Not all uppercase after hyphen

b-
ces
-> b-ces   No vowel before hyphen

ab-ces
-> ab-ces  Hyphen not at end of line

ab- ces
-> ab- ces  Hyphen not at end of line

*/
    {
    bool vowel = false;
    bool nonAlphaFound = false;
    int cnt = 0;
    int j = 0;
    int punkpos = -1;
    bool locAllLower = true;
    bool locAllUpper = true;
    for(j = 0;j != waited && isFlatSpace(BufferForHandlingEndOfLineHyphens[j]);++j)
        BufferForHandlingEndOfLineHyphens[j] = ' ';
    for(;j != waited;++j)
        {
        ++cnt;
        int k = BufferForHandlingEndOfLineHyphens[j];
        if(isAlpha(k))
            {
            if(!isLower(k))
                locAllLower = false;
            if(!isUpper(k))
                locAllUpper = false;
            }
        else if(k != '-')
            {
            if(punkpos < 0 && isPunct(k))
                punkpos = cnt;
            else if(!isCommaOrColon(k))
                nonAlphaFound = true;
            break;
            }
        if(isVowel(k))
            vowel = true;
        }
    if(dropHyphen)
        { // Require agreement of case
        if(allLower)
            {
            if(!locAllLower)
                dropHyphen = false;
            }
        else
            {
            if(!locAllUpper)
                dropHyphen = false;
            }
        }
    if((!nonAlphaFound && cnt >= 2 && punkpos < 0) || punkpos == cnt)
        {
        if(!dropHyphen || !vowel)
            Segment.Put(file,'-',flgs);
        for(j = 0;j != waited;++j)
            Segment.Put(file,BufferForHandlingEndOfLineHyphens[j],flgs);
        }
    else
        hyphenate(flgs);
    }

void paragraph::append(wchar_t ch)
    {
    assert(Index + 1 < sizeof(Line)/sizeof(Line[0]));
    if (Index + 1 < sizeof(Line) / sizeof(Line[0]))
        {
        Line[Index++] = ch;
        Line[Index] = 0;
        }
    else
        ;// We lose characters!
    }

void paragraph::PutHandlingWordWrap(const wint_t ch,flags & flgs) // Called from GetPut, GetPutBullet and doTheSegmentation
    {
    if(flgs.inhtmltag || !Option.wordUnwrap)
        {
        Segment.Put(file,ch,flgs); // //
        }
    else
        {
        if(isSentencePunct(ch))
            {
            ++flgs.punctuationFound;
            }
        else if(isCommaOrColon(ch))
            {
            ++flgs.semiPunctuationFound;
            }
        else if(ch == '-')
            {
            ++flgs.hyphenFound;
            }

        if(ch == '\n' || ch == '\r')
            {
            switch(last)
                {
                case '\n':
                    if(wait)
                        {
                        hyphenate(flgs);
                        wait = 0;
                        flgs.punctuationFound = 0;
                        flgs.hyphenFound = 0;
                        spaceAfterHyphen = false;
                        }
                    flgs.semiPunctuationFound = 0;
                    Segment.Put(file,'\n',flgs);
                    break;
                case '-':
                    {
                    int k;
                    int nonSpaceCount = 0;
                    bool nonAlphaFound = false;
                    dropHyphen = false;
                    int i;
                    // skip previous spaces
                    for( i = ind(lastWordIndex-1)
                       ;    i != lastWordIndex 
                         && isSpace(BufferForHandlingEndOfLineHyphens[ind(i)])
                       ; i = dec(i)
                       )
                        ;
                    allLower = allUpper = true;
                    bool Upper = false; 
                    /* This variable introduces "lag" of one iteration in 
                    changing the value of allLower. In that way, the case of 
                    first character in the string (the one last checked)
                    doesn't matter.
                    Example:
                    blah blah. Da-
                    vid did it.
                    */

                    for (
                        ;    i != lastWordIndex 
                          && ( k = BufferForHandlingEndOfLineHyphens[ind(i)]
                             , !isSpace(k)
                             )
                        ; i = dec(i)
                        )  // Look at casing of last word in retrograde fashion.
                        {
                        ++nonSpaceCount;
                        if(Upper)
                            allLower = false;
                        if(isAlpha(k) && k != '-')
                            {
                            if(!isLower(k))
                                Upper = true;
                            if(!isUpper(k))
                                allUpper = false;
                            }
                        else if(k != '-')
                            {
                            nonAlphaFound = true;
                            break;
                            }
                        if(isVowel(k))  // We need at least one vowel to admit
                            //interpretation of hyphen as the 
                            // effect of word wrap.
                            dropHyphen = true;
                        }
                    if(dropHyphen && !allLower && !allUpper)
                        dropHyphen = false; // Mixed case -> keep hyphen
                    if(!nonAlphaFound && nonSpaceCount >= 2)
                        {
                        /**/
                        lastWordIndex = 0;
                        wait = (sizeof(BufferForHandlingEndOfLineHyphens)/sizeof(BufferForHandlingEndOfLineHyphens[0]));
                        waited = 0;
                        }
                    break;
                    }
                default:
                    {
                    if(wait)
                        {
                        considerHyphenation(flgs);
                        wait = 0;
                        flgs.punctuationFound = 0;
                        flgs.semiPunctuationFound = 0;
                        flgs.hyphenFound = 0;
                        spaceAfterHyphen = false;
                        }
                    Segment.Put(file,'\n',flgs); // Treat newline as a blank
                    }
                }
            }
        else
            {
            if(wait)
                {
                if(!isFlatSpace(ch))
                    {
                    if(waited == wait)
                        {
                        hyphenate(flgs);
                        wait = 0;
                        flgs.punctuationFound = 0;
                        flgs.semiPunctuationFound = 0;
                        flgs.hyphenFound = 0;
                        spaceAfterHyphen = false;
                        Segment.Put(file,ch,flgs);
                        }
                    else
                        {
                        BufferForHandlingEndOfLineHyphens[waited++] = (wchar_t)ch;
                        }
                    }
                else if(waited > 0)
                    {
                    considerHyphenation(flgs);
                    wait = 0;
                    flgs.punctuationFound = 0;
                    flgs.semiPunctuationFound = 0;
                    flgs.hyphenFound = 0;
                    spaceAfterHyphen = false;
                    Segment.Put(file,' ',flgs);
                    }
                }
            else
                {
                if(!flgs.hyphenFound)
                    Segment.Put(file,ch,flgs);
                }
            }
        }
    if(!isFlatSpace(ch))
        {
        if(ch != '\n' && !isSentencePunct(ch) && !wait)
            flgs.punctuationFound = 0;

        if(ch != '\n' && !isCommaOrColon(ch) && !wait)
            flgs.semiPunctuationFound = 0;

        if(ch != '\n' && ch != '-' && flgs.hyphenFound && !wait) // A-bomb
            {
            int k;
            for(k = 0;k < flgs.hyphenFound;++k)
                Segment.Put(file,'-',flgs);
            if(spaceAfterHyphen)
                Segment.Put(file,' ',flgs);
            spaceAfterHyphen = false;
            flgs.hyphenFound = 0;
            Segment.Put(file,ch,flgs);
            }
        last = ch; ///
        }
    else if(flgs.hyphenFound && last != '\n')
        {
        spaceAfterHyphen = true;
        }

    if(!wait)
        {
        if(  !isFlatSpace(ch) 
          || !isFlatSpace(BufferForHandlingEndOfLineHyphens[ind(lastWordIndex-1)])
          )
            {
            BufferForHandlingEndOfLineHyphens[lastWordIndex] = (wchar_t)ch;
            lastWordIndex = inc(lastWordIndex);
            }
        }
    }

void paragraph::flushLine(wint_t ch,flags & flgs)
    {
    size_t lastNonSpace = SIZE_MAX;
    if(Index > 0)
        for (lastNonSpace = Index - 1; lastNonSpace != 0 && isFlatSpace(Line[lastNonSpace]); --lastNonSpace)
            {        
            }
    if(  flgs.inhtmltag
      || !Option.suppressNoise 
      || textBadness(Line,Index) < 0.44
      )
        {
        if(MindTheSpace != 0 /*&& ch != flgs.MindTheSpace*/)
            {
            append(MindTheSpace); // This increments Index
            if (lastNonSpace > Index) // SIZE_MAX
                lastNonSpace = Index - 1;
            MindTheSpace = 0;
            }
        /* Process all characters in Line[] */
        for(size_t i = 0;i < Index;++i)
            {
            flgs.nonWhiteSpaceAhead = (i < lastNonSpace);
            PutHandlingWordWrap(Line[i],flgs);        
            }
        PutHandlingWordWrap(ch,flgs); // //
        }
    else // skip line, too noisy
        {
        PutHandlingWordWrap('\n',flgs);        
        PutHandlingWordWrap(ch,flgs);        
        }
    Index = 0;
    }

void paragraph::PutHandlingLine(wint_t ch,flags & flgs) // Called from GetPut, GetPutBullet and doTheSegmentation
    {
    if(flgs.inhtmltag)
        {
        flushLine(ch,flgs);
        }
    else
        {
        if(ch == '\n' || ch == '\r')
            {
            if(!lastEOLchar || lastEOLchar == ch)
                {
                flushLine('\n',flgs);///
                lastEOLchar = ch;
                }
            else
                {
                // Ignore ch: CRLF or LFCR sequence equals '\n'
                lastEOLchar = '\0'; 
                }
            }
        else
            {
            if(  ch == WEOF 
              || ch == 26 /*20150916*/
              || overflowing()
              )
                {
                flushLine(ch,flgs);
                }
            else
                {
                if(isFlatSpace(ch))
//                if(ch == ' ' || ch == '\t') // reduces "     \t\t   \t\t\t" to " \t \t"
                                            // why keep spaces AND tabs, why not reduce to a single space?
                    {
                    if(MindTheSpace != 0 /*&& ch != flgs.MindTheSpace*/)
                        {
                        append(MindTheSpace);
                        }
                    if(ch == '\f' || ch == '\v')
                        MindTheSpace = ch;
                    else
                        MindTheSpace = ' ';
                    }
                else if(MindTheSpace != 0)
                    {
                    append(MindTheSpace);
                    MindTheSpace = 0;

                    //assert(Index < sizeof(Line)/sizeof(Line[0]));
                    append(ch);
                    }
                else
                    {
                    append(ch);
                    }
                // 20100106 The following does not reduce spaces, but increases their numbers!
                //assert(Index < sizeof(Line)/sizeof(Line[0]));
                //Line[Index++] = ch;
                }
            }
        lastEOLchar = '\0'; 
        }
    }
