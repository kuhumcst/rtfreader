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
#if !defined HYPHENATION_H
#define HYPHENATION_H

#include "data.h"
#include "dots.h"

struct flags;


enum EndOfSegmentIndication {no_indication,period_seen,other_punctuation_seen,newline_seen};

class segment
    {
    public:
        dots Dots;
    private:
        bool notstartofline;
        bool fileStarted;

        enum EndOfSegmentIndication SegmentEndIndication;//{no_indication,period_seen,other_punctuation_seen,newline_seen};
        int neededSegmentDelimiters; // The number of delimiters needed to finish the current segment.
        // Normally, this number is 1: a single newline '\n'.
        // If Option.emptyline is set, the number is 2: either one sentence delimiter and one newline or
        // two newlines.
        // If a newline is received and the last character wasn't a newline or sentence delimiter,
        // a simple blank is sent to Put2.

        wchar_t bufr[256]; // Buffer to keep token that must be analysed for 
        // brackets, dots, etc.
        // The analysis is done when ch is whitespace and
        // also when an html-tag is going to begin.
        // It is necessary to call Put2 with 
        // flgs.htmltagcoming=true only for the character
        // immediately preceding the opening brace.
        size_t pos;

        void perhapsWriteSegmentDelimiter(STROEM * file,wint_t ch,flags & flgs,wint_t ch_loc);
        wint_t bracketsDotsEntitiesInitialsAbbreviations(STROEM * file,flags & flgs,wchar_t * pbuf,wchar_t * nullbyte,wint_t ch_loc);
        void lookWhatsInTheBuffer(STROEM * file,wint_t ch,flags & flgs);
        void PennTreebankTokenize(STROEM * file, wchar_t buf[],size_t lastNonSeparatingCharacter,bool & abbreviation,flags & flgs ,bool & number);
    public:
        segment():notstartofline(false),fileStarted(false),SegmentEndIndication(newline_seen),neededSegmentDelimiters(0),pos(0){}
        void Put(STROEM * file,const wchar_t ch,flags & flgs); // Called from PutHandlingWordWrap
    };

#endif