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
#include "dots.h"
#include "flags.h"
#include "option.h"
#include "letterfunc.h"
#include "commonstuff.h"

void dots::Put3(STROEM * file,wint_t ch,flags & flgs) // called from PutN, Put2 and GetPut
    {
    /* Put3 generally causes a newline (ch=='\n') to be written.
    Exception: inside htmltags.
    */

    if(flgs.inhtmltag)
        {
        flgs.in_abbreviation = false;
        flgs.person_name = not_a_name;
        flgs.number_final_dot = false;
        // This code writes the complete HTML-tag
        if(Option.suppressHTML)
            {
            ch = ' ';
            }
        else
            {
            if(ch)
                {
                if(isSpace(ch))
                    {
                    last = ' ';
                    //return; // 20100107
                    }
                else 
                    {
                    if(isFlatSpace(last))// last == ' ' ||  last == 0xA0 || last == 0x3000)
                        {
                        pRegularizationFnc(file,' ',flgs);
                        }
                    pRegularizationFnc(file,ch,flgs);
                    }
                }
            }
        }
    else
        {
        if(isFlatSpace(ch))//20100106 // ch == 0xA0) // 20071112
            ch = ' ';
        else if((unsigned int)ch < ' ') // replace tabs by spaces and all other non-white space by an asterisk
            {
            if(ch != '\n')
                ch = '*';
            }

        if(ch == ' ')
            {
            if(last != '\n') // Spaces at the beginning of a line are ignored. Only spaces after words are recorded in 'last'.
                {
                last = ' ';
                }
            return;
            }

        if(ch == '\n')
            {
            if(trailingDotFollowingNumber)
                {
                if(Option.tokenize)
                    pRegularizationFnc(file,' ',flgs); // insert blank before dot if number followed by dot is at the end of the line
                pRegularizationFnc(file,'.',flgs);
                trailingDotFollowingNumber = false;
                flgs.in_abbreviation = false;
                }
            }
        else if(isFlatSpace(last))// last == ' ' || last == 0xA0)
            {
            wint_t lastToWrite = ' ';
            if(!isLower(ch)) // Might be an indication that a new sentence starts here. 
                // Check preceding token for trailing dot that might be a
                // sentence delimiter after all.
                {
                if(trailingDotFollowingNumber) // ... in 1999. Next month ...   ch=='N', last is ' '
                    //            ^ Not written from here
                    {// Regard dot as sentence delimiter after all
                    if(Option.tokenize)
                        pRegularizationFnc(file,' ',flgs); // Insert blank before dot if number followed by dot is followed by capitalised word.
                    pRegularizationFnc(file,'.',flgs);
                    trailingDotFollowingNumber = false;
                    lastToWrite = '\n'; // Number seems to be the last word of the previous sentence. Fake history.
                    // ... in 1999.
                    // Next month ...
                    flgs.in_abbreviation = false;
                    }
                else if(flgs.in_abbreviation /*&& flgs.newSegment*/)
                    {
                    switch(flgs.person_name)
                        {
                        case initial: 
                            flgs.person_name = not_a_name;
                            break;
                        case not_a_name:
                        default: // Skema 1. Affald fra husholdninger --> Skema 1. | Affald fra husholdninger       20040420
                            if(!flgs.expectCapitalizedWord)
                                lastToWrite = '\n'; // Abbreviation seems to be the last word of the previous sentence
                            break;
                        }
                    }
                }
            else if(trailingDotFollowingNumber)
                { // Now we suppose that the dot trailing the number is part of that number.
                pRegularizationFnc(file,'.',flgs);
                trailingDotFollowingNumber = false;
                }
            if((lastToWrite != ' ' && lastToWrite != 0xA0) || flgs.writtentoline)
                pRegularizationFnc(file,lastToWrite,flgs);
            flgs.writtentoline = (lastToWrite == ' ' || lastToWrite == 0xA0);
            if(Option.emptyline && flgs.in_abbreviation && !flgs.writtentoline)
                { // Make sure to send \n next time
                ensureEmptyLine = true;
                }
            flgs.in_abbreviation = false;
            }

        if(flgs.number_final_dot == 1 && !(flgs.person_name == initial)) // This can only be the case if ch == separating character
            {
            trailingDotFollowingNumber = true;
            }
        else
            {
            if(ch != '\n' && ensureEmptyLine)
                {
                pRegularizationFnc(file,'\n',flgs);
                }
            ensureEmptyLine = false;
            pRegularizationFnc(file,ch,flgs);
            flgs.writtentoline = ch != '\n';
            if(Option.emptyline && flgs.in_abbreviation && !flgs.writtentoline)
                { // Make sure to send \n next time
                ensureEmptyLine = true;
                }
            }
        }

    last = ch;
    //    flgs.newSegment = false;
    }

void dots::PutN(STROEM * file,wchar_t * buf,size_t len,flags & flgs) // called from Put2
    {
    for(size_t i = 0;i < len;++i)
        {
        Put3(file,buf[i],flgs);
        }
    }
