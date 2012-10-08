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
#ifndef COMMONSTUFF_H
#define COMMONSTUFF_H

#include "data.h"

#define MAXSEGMENTLENGTH 10000


union startLine
    {
    struct
        {
        unsigned int SD:1; // set at start of text and upon seeing segment delimiter (bullet, full stop, colon, ...)
        unsigned int LF:1;
        unsigned int CR:1;
        unsigned int LS:1; // Leading space (Bart 20061214)
        } b;
    unsigned int EOL:3; // SD, LF and CR together
    //unsigned int i;     // all five together
    };

enum PersonName {not_a_name,initial/*,family_name*/};

struct flags
    {
//    bool newSegment:1;
    bool in_abbreviation:1;
    bool expectCapitalizedWord:1; // 20110224
    bool htmltagcoming:1;
    bool firstofhtmltag:1;
    bool notstartofline:1;
    bool writtentoline:1;
    bool inhtmltag:1;
    bool firstafterhtmltag:1;
    bool bbullet:1;
    PersonName person_name:2;
    int number_final_dot:2;
    bool allcaps:1;
    bool startcaps:1;
    bool trailingDotFollowingNumber:1;
    bool fileStarted;
    bool allNumber:1;
    bool lowerRoman:1; // Roman number
    bool upperRoman:1; // Roman number
    bool arabic:1; // True if number is represented with arabic ciffres (0..9)
    bool wordComing:1;
    bool heading:1;
    bool in_fileName:1;
    int hyphens; // If a line in all other respects seems a header, if line ends with a single hyphen, it is no header after all.
    int hyphenFound;
    int nrStartCaps;
    int nrNoStartCaps;
    int nrNonSpaceBytes;
    int f; // font
    int uc; // \ucN	This keyword represents the number (count) of bytes that
    // follow a \uN Unicode character to give the codepage code that best 
    // corresponds to the Unicode character. This keyword may be used at any 
    // time, and values are scoped like character properties. That is, a \ucN 
    // keyword applies only to text following the keyword, and within the same
    // (or deeper) nested braces. On exiting the group, the previous \ucN
    // value is restored. The reader must keep a stack of counts seen and use
    // the most recent one to skip the appropriate number of characters when
    // it encounters a \uN keyword. When leaving an RTF group that specified a
    // \ucN value, the reader must revert to the previous value. A default of
    // 1 should be assumed if no \ucN keyword has been seen in the current or
    // outer scopes.
    // A common practice is to emit no ANSI representation for Unicode 
    // characters within a Unicode destination context (that is, inside a \ud
    // destination). Typically, the destination will contain a \uc0 control
    // sequence. There is no need to reset the count on leaving the \ud 
    // destination, because the scoping rules will ensure the previous value 
    // is restored.

    wint_t lastEOLchar;  // can be '\n' or '\r' or '\0'
    wint_t firstEOLchar; // Used if Option.keepEOLsequence == true
    wint_t secondEOLchar;// Used if Option.keepEOLsequence == true
    wint_t MindTheSpace;
    flags()
        {
        in_abbreviation = false;
        expectCapitalizedWord = false;
        htmltagcoming = false;
        firstofhtmltag = false;
        notstartofline = false;
        writtentoline = false;
        inhtmltag = false;
        firstafterhtmltag = false;
        bbullet = false;
        person_name = not_a_name;
        number_final_dot = 0;
        allcaps = true;
        startcaps = true;
        trailingDotFollowingNumber = false;
        fileStarted = false;
        allNumber = true;
        lowerRoman = false;
        upperRoman = false;
        arabic = false;
        wordComing = true;
        heading = false;
        in_fileName = false;
        hyphens = 0;
        hyphenFound = 0;
        nrStartCaps = 0;
        nrNoStartCaps = 0;
        nrNonSpaceBytes = 0;
        f = 0;
        uc = 2;
        lastEOLchar = 0;
        firstEOLchar = 0;
        secondEOLchar = 0;
        MindTheSpace = 0;
        }
    };

typedef wint_t (*lookahead_fnc)(STROEM * fp,const startLine firsttext,int f);


extern const bool space[256];
extern STROEM * sourceFile,* outputtext;
extern void PutHandlingLine(wint_t ch,flags & flgs); // Called from GetPut, GetPutBullet and doTheSegmentation
extern wint_t GetPutBullet(const long end_offset,flags & flgs);
extern bool checkSentenceEnd(int ch,long & begin_offset,lookahead_fnc lookahead,const startLine firsttext,const long curr_pos,flags & flgs);
extern void Put2(const wchar_t ch,flags & flgs); // Called from PutHandlingWordWrap
extern bool closeStreams();

// rtfreader.cpp only:
extern void Put4A(wint_t c,flags & flgs);
extern void (*pPut4)(wint_t c,flags & flgs);

// ocrtidy.cpp only:
extern const bool vowels[256];


#define isSpace(s) (0 <= s && s < 256 && space[s])
#define isFlatSpace(s) (s == '\t' || s == ' ' || s == 0xA0)
#define isVowel(s) (0 <= s && s < 256 && vowels[s])

#endif
