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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "option.h"
#include "commonstuff.h"
#include "flags.h"
#if FILESTREAM
#include "mwu.h"
#endif

void Put(STROEM * file,wchar_t ch,flags & flgs); // Called from GetPutBullet and doTheSegmentation

void regularizeWhiteSpace(STROEM * file,wint_t c,flags & flgs) // Called from Put3
    {
    static wint_t previousCharacter = 0;
    static bool previousWasNewLine = false;
    static bool previousWasFlatSpace = false;
    if(isFlatSpace(c))// c == ' ' || c == 0xA0)
        previousWasFlatSpace = true;
    else
        {
        if(c == '\n')
            {
            if(!previousWasNewLine)
                {
                if(Option.spaceAtEndOfLine)
                    Fputc(' ',file);// Add extra space before new line (compatibility mode).
                flgs.writtentoline = false;
                int reps = 1;
                if(Option.Emptyline && !isPunct(previousCharacter))
                    reps = 2;
                for(int i = 0;i < reps;++i)
                    {
                    if(Option.keepEOLsequence)
                        {
                        if(flgs.firstEOLchar != 0)
                            {
                            Fputc(flgs.firstEOLchar,file);
                            if(flgs.secondEOLchar != 0)
                                Fputc(flgs.secondEOLchar,file);
                            }
                        else
                            Fputc(c,file);
                        }
                    else
                        {
                        Fputc(c,file);
                        }
                    }
                previousWasNewLine = true;
                }
            }
        else
            {
            previousWasNewLine = false;
            previousCharacter = c;
            if(  flgs.writtentoline
              && previousWasFlatSpace
              )// last == ' ' ||  last == 0xA0)
                Fputc(' ',file);
            Fputc(c,file);
            flgs.writtentoline = true;
            }
        previousWasFlatSpace = false;
        }
    }

void ASCIIfy(STROEM * file,wint_t c,flags & flgs)// Called from Put3
    {
    switch(c)
        {// cases added 20090528
        case 0x2012:
        case 0x2013:
        case 0x2014:
        case 0x2015:
            regularizeWhiteSpace(file,'-',flgs);
            break;
        case 0x2016:
            regularizeWhiteSpace(file,'|',flgs);
            break;
        case 0x2017:
            regularizeWhiteSpace(file,'_',flgs);
            break;
        case 0x2018:
        case 0x2019:
        case 0x201a:
        case 0x201b:
            regularizeWhiteSpace(file,'\'',flgs);
            break;
        case 0x201c:
        case 0x201d:
        case 0x201e:
        case 0x201f:
            regularizeWhiteSpace(file,'\"',flgs);
            break;
            // bullet added 20100108
        case 0x2022:
            regularizeWhiteSpace(file,'*',flgs);
            break;
            // 2015 ->
        case 0x20AC:
            regularizeWhiteSpace(file,'E',flgs);regularizeWhiteSpace(file,'U',flgs);regularizeWhiteSpace(file,'R',flgs);
            break;
        case 0x2026:
            regularizeWhiteSpace(file,'.',flgs);regularizeWhiteSpace(file,'.',flgs);regularizeWhiteSpace(file,'.',flgs);
            break;
        case 0x0152:
            regularizeWhiteSpace(file,'O',flgs);regularizeWhiteSpace(file,'E',flgs);
            break;
        case 0x2122:
            regularizeWhiteSpace(file,'T',flgs);regularizeWhiteSpace(file,'M',flgs);
            break;
        case 0x0153:
            regularizeWhiteSpace(file,'o',flgs);regularizeWhiteSpace(file,'e',flgs);
            break;
        case 0x0132:
            regularizeWhiteSpace(file,'I',flgs);regularizeWhiteSpace(file,'J',flgs);
            break;
        case 0x0133:
            regularizeWhiteSpace(file,'i',flgs);regularizeWhiteSpace(file,'j',flgs);
            break;

        default:
            regularizeWhiteSpace(file,c,flgs);
        }
    }



void ASCIIfyRTF(STROEM * file,wint_t c,flags & flgs)// Called from Put3
    {
    static const wint_t AsciiEquivalent[256] =
    {
          0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
         16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
        ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
        '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
        'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
        '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
        'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~', 127,
        0,  129, ',', '#', '"',  0, '#', '#', '^', '#', 'S', '<',  0, 141, 142, 143, // EUR comma florin ldblquote ellipsis footnote footnote circonflex promille S < OE
        144,'\'','\'', '"', '"', '*', '-', '-', '~',  0, 's', '>',  0, 157, 158,  0,
        160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
        176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
        192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
        208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
        224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
        240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
    };


    if(0 <= c && c < 256)
        {
        wint_t k = AsciiEquivalent[c];
        if(k == 0) // 20090528 -1 --> 0
            {
            switch(c & 0xFF)
                {
                case 128:
                    regularizeWhiteSpace(file,'E',flgs);regularizeWhiteSpace(file,'U',flgs);regularizeWhiteSpace(file,'R',flgs);
                    break;
                case 133:
                    regularizeWhiteSpace(file,'.',flgs);regularizeWhiteSpace(file,'.',flgs);regularizeWhiteSpace(file,'.',flgs);
                    break;
                case 140:
                    regularizeWhiteSpace(file,'O',flgs);regularizeWhiteSpace(file,'E',flgs);
                    break;
                case 153:
                    regularizeWhiteSpace(file,'T',flgs);regularizeWhiteSpace(file,'M',flgs);
                    break;
                case 156:
                    regularizeWhiteSpace(file,'o',flgs);regularizeWhiteSpace(file,'e',flgs);
                    break;
                case 159:
                    regularizeWhiteSpace(file,'I',flgs);regularizeWhiteSpace(file,'J',flgs);
                    break;
                default:
                    regularizeWhiteSpace(file,c,flgs);
                }
            flgs.writtentoline = true;
            }
        else
            {
            regularizeWhiteSpace(file,k,flgs);
            }
        }
    else
        {
        switch(c)
            {// cases added 20090528
            case 0x2012:
            case 0x2013:
            case 0x2014:
            case 0x2015:
                regularizeWhiteSpace(file,'-',flgs);
                break;
            case 0x2016:
                regularizeWhiteSpace(file,'|',flgs);
                break;
            case 0x2017:
                regularizeWhiteSpace(file,'_',flgs);
                break;
            case 0x2018:
            case 0x2019:
            case 0x201a:
            case 0x201b:
                regularizeWhiteSpace(file,'\'',flgs);
                break;
            case 0x201c:
            case 0x201d:
            case 0x201e:
            case 0x201f:
                regularizeWhiteSpace(file,'\"',flgs);
                break;
            // bullet added 20100108
            case 0x2022:
                regularizeWhiteSpace(file,'*',flgs);
                break;

            default:
                regularizeWhiteSpace(file,c,flgs);
            }
        }
    }


void (*pRegularizationFnc)(STROEM * file,wint_t c,flags & flgs) = regularizeWhiteSpace;
void (*pASCIIfyFunction)(STROEM * file,wint_t c,flags & flgs) = ASCIIfy;



#if 0
bool checkSentenceEnd(STROEM * sourceFile,int ch,long & begin_offset,lookahead_fnc lookahead,const startLine firsttext,const long curr_pos,flags & flgs)
    {
    /* Look for:
            bullets
            .
            ?
            !
            ;
            :
       Check for exceptions: ellipsis, filenames, abbreviations, numbers, "inline bullets"
    */

    bool sentenceEnd = false;
    //flgs.certainSentenceEndMarker = false;

    static bool in_parentheses = false;
    static bool in_number = false;
    static bool in_word = false;
    static bool character_entity = false;
    static int ellipsis = 0;

    if(isSpace(ch))
        {
        if(firsttext.EOL)
            {
            begin_offset = curr_pos;
            }
        if(flgs.bbullet)
            {
            sentenceEnd = true; // force writing of segment
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
            flgs.bbullet = isBullet(ch); // First character on line seems to indicate a bullet.
        }

    if(ch != '.')
        ellipsis = 0;

    if(!sentenceEnd)
        switch(ch)
            {
            case '<':
            case '>':
            case '|':
            case '*':
            case '"':
                flgs.in_fileName = false;
                break;
            case ')':
                /* Closing parenthesis is NOT regarded as sentence delimiter if
                        it is part of a file name               c:\file(a).txt
                        it has no opening parenthesis           So we say A) this and b) that.
                */
                if(!flgs.in_fileName && in_parentheses)
                    {
                    in_parentheses = false;
                    sentenceEnd = true;
                    //flgs.certainSentenceEndMarker = true;
                    }
                break;
            case '(':
                {
                /* Opening parenthesis is NOT regarded as sentence delimiter if
                        it is part of a file name               c:\file(a).txt
                        it is followed by a lower case letter   So we say (a) this and (b that.
                */
                if(!flgs.in_fileName)
                    {
                    int next = lookahead(sourceFile,firsttext,flgs.f);
                    if(!isLower(next))
                        {
                        in_parentheses = true;
                        sentenceEnd = true;
                        //flgs.certainSentenceEndMarker = true;
                        }
                    }
                break;
                }
            case '.':
                {
                /* Period is NOT regarded as sentence delimiter if
                        it is part of a filename                c:\file(a).txt
                        it is part of an ellipsis               ..
                        it stands between letters               F.eks.
                        it stands between digits                3.4
                        it follows after a digit and it is followed by a space  4.
                */
                wint_t next = lookahead(sourceFile,firsttext,flgs.f);
                if(strchr(" \\:*?\"<>|",next)) // Note: ./program is often seen, .\program not 
                    flgs.in_fileName = false;

                if(!flgs.in_fileName)
                    {
                    if(next == '.')
                        {
                        ellipsis++;
                        }
                    else if(  !ellipsis
                           && !(in_word && isAlpha(next))
                           && !(  in_number 
                               && (  isFlatSpace(next) // next == ' ' || next == 0xA0
                                  || isDigit(next)
                                  )
                               )
                           )
                        {
                        sentenceEnd = true;
                        //flgs.certainSentenceEndMarker = false;// We have not yet checked with abbreviation gazetteer.
                        }
                    }
                break;
                }
            case ':':
                /* Colon is NOT regarded as sentence delimiter if
                        it stands between digits                                2:3
                        it has a letter to the left and a slash to the right    c:/
                */
                flgs.in_fileName = false;
                if(in_number)
                    {
#if COLONISSENTENCEDELIMITER
                    int next = lookahead(sourceFile,firsttext);
                    if(!isDigit(next))
                        sentenceEnd = true;
#endif
                    }
                else if(in_word)
                    {
                    int next = lookahead(sourceFile,firsttext,flgs.f);
                    if(next == '\\' || next == '/')
                        flgs.in_fileName = true;
#if COLONISSENTENCEDELIMITER
                    else
                        sentenceEnd = true; // not in a DOS file name
#endif
                    }
#if COLONISSENTENCEDELIMITER
                else
                    sentenceEnd = true;
#endif
                break;
            case ';':
                if(character_entity)
                    {
                    character_entity = false;
                    break;
                    }
                // drop through
            case '?':
            case '!':
                flgs.in_fileName = false;
                sentenceEnd = true;
                //flgs.certainSentenceEndMarker = true;
                break;
            case '&':
                character_entity = true;
                break;
            default:
                if(isAlpha(ch))
                    {
                    in_word = true;
                    }
                else
                    {
                    in_word = false;
                    if(isDigit(ch))
                        {
                        in_number = true;
                        }
                    else
                        {
                        character_entity = false;
                        in_number = false;
                        }
                    }
            }

    return sentenceEnd;
    }
#endif
