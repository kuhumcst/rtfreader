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
#if !defined FLAGS_H
#define FLAGS_H

#include "data.h"

enum PersonName {not_a_name,initial/*,family_name*/};

struct flags
    {
    bool in_abbreviation:1;
    bool expectCapitalizedWord:1; // 20110224
    bool htmltagcoming:1;
    bool firstofhtmltag:1;
    bool writtentoline:1;
    bool inhtmltag:1;
    bool firstafterhtmltag:1;
    bool bbullet:1;
    PersonName person_name:2;
    int number_final_dot:2;
    bool in_fileName:1;
    //bool certainSentenceEndMarker:1; // Set to false if period found and it might be part of abbreviation in gazetteer
    //bool previousWasFlatSpace:1; // regularizeWhiteSpace
    int punctuationFound;
    int semiPunctuationFound;
    int hyphenFound;
//    int f; // font
    //int uc; // \ucN	This keyword represents the number (count) of bytes that
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

    
    wint_t firstEOLchar; // Used if Option.keepEOLsequence == true
    wint_t secondEOLchar;// Used if Option.keepEOLsequence == true
    flags()
        {
        in_abbreviation = false;
        expectCapitalizedWord = false;
        htmltagcoming = false;
        firstofhtmltag = false;
        writtentoline = false;
        inhtmltag = false;
        firstafterhtmltag = false;
        bbullet = false;
        person_name = not_a_name;
        number_final_dot = 0;
        in_fileName = false;
        //hyphens = 0;
        punctuationFound = 0;
        semiPunctuationFound = 0;
        hyphenFound = 0;
        //f = 0;
        //uc = 2;
        firstEOLchar = 0;
        secondEOLchar = 0;
       // MindTheSpace = 0;
        }
    };
#endif