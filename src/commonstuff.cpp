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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ocrtidy.h"
#include "option.h"
#include "charconv.h"
#include "commonstuff.h"
#if FILESTREAM
#include "abbrev.h"
#include "mwu.h"
#endif

STROEM * sourceFile,* outputtext;
static wchar_t Line[256];
static size_t Index = 0;

#if 0
const bool flatspace[256] = // true if \t ' ' or non breakable space
    {
      false, false, false, false, false, false, false, false, false, true , false, false, false, false, false, false, /*   0 -  15 */ /* 00 - 0F */
      false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /*  16 -  31 */ /* 10 - 1F */
      true , false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /*  31 -  47 */ /* 20 - 2F */
      false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /*  48 -  63 */ /* 30 - 3F */
                                                                                                                     
      false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /*  64 -  79 */ /* 40 - 4F */
      false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /*  80 -  95 */ /* 50 - 5F */
      false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /*  96 - 111 */ /* 60 - 6F */
      false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /* 112 - 127 */ /* 70 - 7F */
                                                                                                                     
      false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /* 128 - 143 */ /* 80 - 8F */
      false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /* 144 - 159 */ /* 90 - 9F */
      true , false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /* 160 - 175 */ /* A0 - AF */
      false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /* 176 - 191 */ /* B0 - BF */
                                                                                                                     
      false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /* 192 - 207 */ /* C0 - CF */
      false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /* 208 - 223 */ /* D0 - DF */
      false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /* 224 - 239 */ /* E0 - EF */
      false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /* 240 - 255 */ /* F0 - FF */
    };

#define isFlatSpace(s) (0 <= s && s < 256 && flatspace[s])
#endif

// 20071112 isspace ->isSpace for handling 0xA0 (nbsp) as white space.
const bool space[256] =
    {
    false, false, false, false, false, false, false, false, false, true , true , true , true , true , false, false, /*   0 -  15 */ /* 00 - 0F */
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /*  16 -  31 */ /* 10 - 1F */
    true , false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /*  31 -  47 */ /* 20 - 2F */
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /*  48 -  63 */ /* 30 - 3F */

    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /*  64 -  79 */ /* 40 - 4F */
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /*  80 -  95 */ /* 50 - 5F */
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /*  96 - 111 */ /* 60 - 6F */
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /* 112 - 127 */ /* 70 - 7F */

    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /* 128 - 143 */ /* 80 - 8F */
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /* 144 - 159 */ /* 90 - 9F */
    true , false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /* 160 - 175 */ /* A0 - AF */
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /* 176 - 191 */ /* B0 - BF */

    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /* 192 - 207 */ /* C0 - CF */
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /* 208 - 223 */ /* D0 - DF */
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /* 224 - 239 */ /* E0 - EF */
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, /* 240 - 255 */ /* F0 - FF */
    };

static wchar_t SEP[] = L"!\"\'(),./:;?[]`{}\x82\x83\x84\x85\x86\x87\x88\x89\x91\x92\x93\x94\x95\x96\x97\x98\xab\xbb";// Bart 20051207
static wchar_t QUOTE[] = L"\'`\x91\x92"; // Bart 20090522. Was: L"\'`\x92"
static wchar_t QUANTITY[] = L"$%\x80\x83\x89\xA2\xA3\xA5";//dollar, percent, euro, florin, promille, cent, pound, yen 
static wchar_t RBRACKET[] = L")}]";
static wchar_t LBRACKET[] = L"({[";


static bool isSep(wchar_t a)
    {
    if(0xA0 <= a && a < 0xC0)
        return true;
    return  (0x2012 <= a && a <= 0x2027)  // dashes, bar, vertical line, double low line, quotation marks, daggers, bullet, leaders, ellipsis, hyphenation point
        // 20090528
        || wcschr(SEP,a) != 0;
    }

static bool isQuantity(wchar_t a)
    {
    return wcschr(QUANTITY,a) != 0;
    }

void Put(wchar_t ch,flags & flgs); // Called from GetPutBullet and doTheSegmentation



static void Put4(wint_t c,flags & flgs) // Called from Put3
    {
    static int last = 0;
    if(!isFlatSpace(c))// c != ' ' && c != 0xA0)
        {
        if(c == '\n')
            {
            if(Option.spaceAtEndOfLine)
                Fputc(' ',outputtext);// Add extra space before new line (compatibility mode).
            flgs.writtentoline = false;
            if(Option.keepEOLsequence)
                {
                if(flgs.firstEOLchar != 0)
                    {
                    Fputc(flgs.firstEOLchar,outputtext);
                    if(flgs.secondEOLchar != 0)
                        Fputc(flgs.secondEOLchar,outputtext);
                    }
                else
                    Fputc(c,outputtext);
                }
            else
                {
                Fputc(c,outputtext);
                }
            }
        else
            {
            flgs.writtentoline = true;
            if(isFlatSpace(last))// last == ' ' ||  last == 0xA0)
                Fputc(' ',outputtext);
            Fputc(c,outputtext);
            }
        }
    last = c;
    }

void Put4A(wint_t c,flags & flgs)// Called from Put3
    {
    if(0 <= c && c < 256)
        {
        wint_t k = AsciiEquivalent[c];
        if(k == 0) // 20090528 -1 --> 0
            {
            switch(c & 0xFF)
                {
                case 128:
                    Put4('E',flgs);Put4('U',flgs);Put4('R',flgs);
                    break;
                case 133:
                    Put4('.',flgs);Put4('.',flgs);Put4('.',flgs);
                    break;
                case 140:
                    Put4('O',flgs);Put4('E',flgs);
                    break;
                case 153:
                    Put4('T',flgs);Put4('M',flgs);
                    break;
                case 156:
                    Put4('o',flgs);Put4('e',flgs);
                    break;
                case 159:
                    Put4('I',flgs);Put4('J',flgs);
                    break;
                default:
                    Put4(c,flgs);
                }
            flgs.writtentoline = true;
            }
        else
            {
            Put4(k,flgs);
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
                Put4('-',flgs);
                break;
            case 0x2016:
                Put4('|',flgs);
                break;
            case 0x2017:
                Put4('_',flgs);
                break;
            case 0x2018:
            case 0x2019:
            case 0x201a:
            case 0x201b:
                Put4('\'',flgs);
                break;
            case 0x201c:
            case 0x201d:
            case 0x201e:
            case 0x201f:
                Put4('\"',flgs);
                break;
            // bullet added 20100108
            case 0x2022:
                Put4('*',flgs);
                break;

            default:
                Put4(c,flgs);
            }
        }
    }

void (*pPut4)(wint_t c,flags & flgs) = Put4;


static void Put3(wint_t ch,flags & flgs) // called from PutN, Put2 and GetPut
    {
    /* Put3 generally causes a newline (ch=='\n') to be written.
    Exception: inside htmltags.
    */

    static int last = 0;
    //static wint_t trailingDotFollowingNumber = 0;

    if(flgs.inhtmltag)
        {
        if(flgs.trailingDotFollowingNumber)
            {
            if(Option.tokenize)
                pPut4(' ',flgs); // insert blank before dot if number followed by dot is at the end of the line
            pPut4('.',flgs);
            flgs.trailingDotFollowingNumber = false;
            }
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
                    if(isFlatSpace(last))// last == ' ' ||  last == 0xA0)
                        {
                        pPut4(' ',flgs);
                        }
                    pPut4(ch,flgs);
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
            if(flgs.trailingDotFollowingNumber)
                {
                if(Option.tokenize)
                    pPut4(' ',flgs); // insert blank before dot if number followed by dot is at the end of the line
                pPut4('.',flgs);
                flgs.trailingDotFollowingNumber = false;
                }
            }
        else if(isFlatSpace(last))// last == ' ' || last == 0xA0)
            {
            wint_t lastToWrite = ' ';
            if(/*isUpper*/!isLower(ch)) // Might be an indication that a new sentence starts here. 
                // Check preceding token for trailing dot that might be a
                // sentence delimiter after all.
                {
                if(flgs.trailingDotFollowingNumber) // ... in 1999. Next month ...   ch=='N', last is ' '
                    //            ^ Not written from here
                    {// Regard dot as sentence delimiter after all
                    if(Option.tokenize)
                        pPut4(' ',flgs); // Insert blank before dot if number followed by dot is followed by capitalised word.
                    pPut4('.',flgs);
                    flgs.trailingDotFollowingNumber = false;
                    lastToWrite = '\n'; // Number seems to be the last word of the previous sentence. Fake history.
                    // ... in 1999.
                    // Next month ...
                    }
                else if(flgs.in_abbreviation /*&& flgs.newSegment*/)
                    {
                    switch(flgs.person_name)
                        {
                        case initial: 
                            flgs.person_name = not_a_name;
                            break;
                            /*
                            case family_name:
                            person_name = not_a_name;
                            // drop through
                            */
                        case not_a_name:
                        default: // Skema 1. Affald fra husholdninger --> Skema 1. | Affald fra husholdninger       20040420
                            if(!flgs.expectCapitalizedWord)
                                lastToWrite = '\n'; // Abbreviation seems to be the last word of the previous sentence
                            break;
                        }
                    }
                }
            else if(flgs.trailingDotFollowingNumber)
                { // Now we suppose that the dot trailing the number is part of that number.
                pPut4('.',flgs);
                flgs.trailingDotFollowingNumber = false;
                }
            if((lastToWrite != ' ' && lastToWrite != 0xA0) || flgs.writtentoline)
                pPut4(lastToWrite,flgs);
            flgs.writtentoline = (lastToWrite == ' ' || lastToWrite == 0xA0);
            flgs.in_abbreviation = false;
            }
        if(flgs.number_final_dot == 1) // This can only be the case if ch == separating character
            {
            flgs.trailingDotFollowingNumber = true;
            }
        else
            {
            pPut4(ch,flgs);
            flgs.writtentoline = ch != '\n';
            }
        }
    last = ch;
    //    flgs.newSegment = false;
    }

static void PutN(wchar_t * buf,size_t len,flags & flgs) // called from Put2
    {
    for(size_t i = 0;i < len;++i)
        {
        Put3(buf[i],flgs);
        }
    }


void Shift(wchar_t *end,int save,wchar_t buf2[256],int offset)
    {
    if(save)
        *end = (wchar_t)save;
    wchar_t * e = buf2+wcslen(buf2);
    *(e+1) = '\0';
    for(;e > buf2 + offset;--e)
        *e = *(e - 1);
    *e = ' ';
    }



// see http://www.cis.upenn.edu/~treebank/tokenizer.sed
void PennTreebankTokenize( wchar_t buf[],size_t lastNonSeparatingCharacter,bool & abbreviation,flags & flgs ,bool & number)
    {    
    size_t aa = 0;
    for(;aa <= lastNonSeparatingCharacter;++aa)
        {
        if(buf[aa] == '.')
            {
            //                                printf("{ABBR . inside}");
            abbreviation = true; // . detected inside word
            }
        else if(isDigit(buf[aa])) // 20040120
            {
            number = true;
            }
        else if(aa > 0 && aa < lastNonSeparatingCharacter &&  wcschr(QUOTE,buf[aa]))
            {
            buf[aa] = '\'';
            }
        }
    aa = 0;
    wchar_t save = buf[lastNonSeparatingCharacter+1];
    buf[lastNonSeparatingCharacter+1] = '\0';
    wchar_t buf2[256];
    wcscpy (buf2,buf+aa);
    buf[lastNonSeparatingCharacter+1] = save;
    save = '\0';
    size_t len = wcslen(buf2);
    if(buf2[len - 1] == '\'')
        {
        buf2[len - 1] = ' ';
        buf2[len] = '\'';
        buf2[len + 1] = '\0';
        len--;
        }
    int shift = 0;
    if(len > 3)
        {
        wchar_t * last3 = buf2 + len - 3;
        if(*last3 == '\'')
            {
            wchar_t * last2 = last3 + 1;
            if(  !strCaseCmp(last2,"ll")
              || !strCaseCmp(last2,"ve")
              || !strCaseCmp(last2,"re")
              )
                shift = 3;
            }
        else if(!strCaseCmp(last3,"n't"))
            shift = 3;
        }
    if(!shift && len > 2)
        {
        wchar_t * last2 = buf2 + len - 2;
        if(*last2 == '\'')
            {
            wchar_t * last1 = buf2 + len - 1;
            if(  !strCaseCmp(last1,"s")
              || !strCaseCmp(last1,"m")
              || !strCaseCmp(last1,"d")
              )
            shift = 2;
            }
        }
    if(shift)
        {
        wchar_t * e = buf2+wcslen(buf2);
        *(e+1) = '\0';
        for(;e > buf2 + len - shift;--e)
            *e = *(e - 1);
        *e = ' ';
        }
    wchar_t * end = wcschr(buf2,' ');
    if(end)
        {
        save = *end;
        *end = '\0';
        }
/*
cannot = can not
d'ye = d' ye
gimme = gim me
gonna = gon na
gotta = got ta
lemme = lem me
more'n = more 'n
'tis = 't is            20091113: added the apostrophe
'twas = 't was          20091113: added the apostrophe
wanna = wan na
whaddya = wha dd ya
whatcha = wha t cha
*/

    if(  !strCaseCmp(buf2,"tis")
      || !strCaseCmp(buf2,"twas")
      )
        {
        Shift(end,save,buf2,1);
        }
    else if(  !strCaseCmp(buf2,"'tis") // 20091113 Does not work as intended. The leading apostrophe is already separated.
      || !strCaseCmp(buf2,"'twas")
      )
        {
        Shift(end,save,buf2,2);
        }
    else if(!strCaseCmp(buf2,"d'ye"))
        {
        Shift(end,save,buf2,2);
        }
    else if(  !strCaseCmp(buf2,"cannot")
           || !strCaseCmp(buf2,"gimme")
           || !strCaseCmp(buf2,"gonna")
           || !strCaseCmp(buf2,"gotta")
           || !strCaseCmp(buf2,"lemme")
           || !strCaseCmp(buf2,"wanna")
           )
        {
        Shift(end,save,buf2,3);
        }
    else if(!strCaseCmp(buf2,"more'n"))
        {
        Shift(end,save,buf2,4);
        }
    else if(!strCaseCmp(buf2,"whatcha"))
        {
        Shift(end,save,buf2,4);
        Shift(end,save,buf2,3);
        }
    else if(!strCaseCmp(buf2,"whaddya"))
        {
        Shift(end,save,buf2,5);
        Shift(end,save,buf2,3);
        }
    else
        {
        if(save)
            *end = save;
        }
    PutN(buf2,/*strlen*/ wcslen(buf2),flgs);
    }


bool closeStreams()
    {
    Fclose(sourceFile);
    if(outputtext )
        {
        flags flgs;
        Put4('\n',flgs);
        Fclose(outputtext );
        return true;
        }
    return false;
    }


void Put2(const wchar_t ch,flags & flgs) // Called from PutHandlingWordWrap
    {
    // Check for brackets, quotes, valuta and other quantities,
    // abbreviations and numbers.

    // Put2 generally writes a newline for each received newline. Exceptions
    // are newlines emitted after numbers and abbreviations with trailing
    // dots, in which case the newline is replaced by a white space.
    // However, this whitespace is possibly changed back to a newline.
    // This happens if a trailing dot after a number is followed by an
    // upper case letter, indicating the start of a sentence.
    static wchar_t bufr[256]; // Buffer to keep token that must be analysed for 
                          // brackets, dots, etc.
                          // The analysis is done when ch is whitespace and
                          // also when an html-tag is going to begin.
                          // It is necessary to call Put2 with 
                          // flgs.htmltagcoming=true only for the character
                          // immediately preceding the opening brace.
    static size_t pos = 0;
    wint_t ch_loc = ch; // If ch is newline and the preceding token is an 
                     // abbreviation or number with a dot in the end, the dot 
                     // is regarded as part of that token and the newline is
                     // replaced with a simple blank.

    if(!flgs.fileStarted)
        {
        pos = 0;
        bufr[0] = '\0';
        flgs.fileStarted = true;
        }
    if(pos >= (sizeof(bufr)/sizeof(bufr[0])) - 1)
        {
        PutN(bufr,(sizeof(bufr)/sizeof(bufr[0])) - 1,flgs);
        pos = 0;
        bufr[0] = '\0';
        }

    if(flgs.inhtmltag)
        {
        if(flgs.firstofhtmltag)
            {
            if(Option.tokenize && !Option.suppressHTML && flgs.notstartofline)
                {
                bufr[pos++] = ' ';
                }
            flgs.firstofhtmltag = false;
            }
        bufr[pos++] = ch;
        bufr[pos] = '\0';
        PutN(bufr,pos,flgs);
        pos = 0;
        bufr[0] = '\0';
        }
    else if(flgs.htmltagcoming || wcschr(L" \t\n\r",ch) || ch == 0)
        {
        if(pos)
            {
            bufr[pos] = '\0';
            wchar_t * pbuf = bufr;

            for(;;)
                {// Check for sequences of hyphens and put them in separate tokens (if necessary)
                wchar_t * h = wcsstr(pbuf,L"--");
                wchar_t * nullbyte = bufr+pos;
                if(h)
                    {
                    *h = '\0';
                    nullbyte = h;
                    }
                else
                    nullbyte = bufr+pos;

                if(nullbyte > pbuf)
                    {
                    //size_t  firstNonSeparatingCharacter,lastNonSeparatingCharacter,a2,b2;
                    wchar_t * firstNonSeparatingCharacter;
                    wchar_t * lastNonSeparatingCharacter;
                    wchar_t * a2;
                    wchar_t * b2;
                    bool lquote = false;
                    bool rquote = false;
                    wchar_t * rbracket = NULL;
                    for(firstNonSeparatingCharacter = pbuf;*firstNonSeparatingCharacter && isSep(*firstNonSeparatingCharacter);++firstNonSeparatingCharacter)
                        {
                        if(wcschr(QUOTE,*firstNonSeparatingCharacter))
                            lquote = true;
                        }
                    for(lastNonSeparatingCharacter = nullbyte - 1;lastNonSeparatingCharacter >= firstNonSeparatingCharacter && isSep(*lastNonSeparatingCharacter);--lastNonSeparatingCharacter)
                        {
                        if(!rbracket && wcschr(RBRACKET,*lastNonSeparatingCharacter))
                            rbracket = lastNonSeparatingCharacter;

                        if(wcschr(QUOTE,*lastNonSeparatingCharacter))
                            rquote = true;
                        else
                            rquote = false;
                        }

                    if(rquote && !lquote)
                        ++lastNonSeparatingCharacter; // A right quote without corresponding left quote is regarded part of the word, ikk'
                    else if(rbracket)
                        {
                        wchar_t * L = wcspbrk(firstNonSeparatingCharacter,LBRACKET);
                        if(L && L < rbracket)
                            lastNonSeparatingCharacter = rbracket; // found something like opfinder(e)
                        }
                    if(isDigit(*firstNonSeparatingCharacter))
                        for(;lastNonSeparatingCharacter >= firstNonSeparatingCharacter && isQuantity(*lastNonSeparatingCharacter);--lastNonSeparatingCharacter)
                            {//trailing dollar, percent, euro, florin, promille, cent, pound, yen are put in separate tokens
                            //TODO preceding valuta.
                            }
                    bool URL = false;
                    // If a separating character (not a dot) occurs inside a word, then it is also accepted as part of the word if it also is at the end of the word.
                    // TODO: same for pairs such as brackets
                    bool extended = false;
                    for(b2 = nullbyte; !extended && --b2 > lastNonSeparatingCharacter && *b2 != '.';)
                        {
                        for(a2 = firstNonSeparatingCharacter;a2 < lastNonSeparatingCharacter;++a2)
                            if(*a2 == *b2)
                                {
                                URL = true;
                                lastNonSeparatingCharacter = b2;     // Not a separator after all.
                                extended = true;
                                break;
                                }           // e.g. http://www.postgirotbank.com/ 
                            // The last / is part of the string because there are / inside the string
                        }

                    extended = false;
                    for(a2 = pbuf ;!extended && a2 < firstNonSeparatingCharacter;++a2)
                        {
                        for(b2 = firstNonSeparatingCharacter;b2 <= lastNonSeparatingCharacter;++b2)
                            if(*a2 == *b2)
                                {
                                URL = true;
                                firstNonSeparatingCharacter = a2;     // Not a separator after all.
                                extended = true;
                                break;
                                }           // e.g. ..[tmp/drive.txt/
                            // The first .. is part of the text because there is a . inside the string
                        }

                    if(firstNonSeparatingCharacter <= lastNonSeparatingCharacter)
                        {
                        if(*(lastNonSeparatingCharacter+1) == ';')
                            {
                            wchar_t * p;
                            if(  pbuf < (p=wcschr(firstNonSeparatingCharacter,'&'))
                                && p <= lastNonSeparatingCharacter // b&amp;
                                )
                                ++lastNonSeparatingCharacter;// html character entity, for example &amp;
                            else if(  pbuf < firstNonSeparatingCharacter
                                && *(firstNonSeparatingCharacter-1) == '&' // If it is decided that '&' is a separator
                                )
                                {                               // &amp;
                                --firstNonSeparatingCharacter;
                                ++lastNonSeparatingCharacter;
                                }
                            }
                        else if(  firstNonSeparatingCharacter > pbuf 
                            && *(firstNonSeparatingCharacter-1) == '&' // If it is decided that '&' is a separator
                            )
                            {
                            wchar_t * p;
                            if(  pbuf < (p= wcschr(firstNonSeparatingCharacter,';'))
                                && p <= lastNonSeparatingCharacter
                                )
                                --firstNonSeparatingCharacter;// html character entity, for example &amp;cetera
                            }

                         wchar_t * aa;
                        // Write the separating characters at the beginning
                        for(aa = pbuf;aa < firstNonSeparatingCharacter;++aa)
                            {
                            Put3(*aa,flgs);
                            if(Option.tokenize)
                                Put3(' ',flgs);
                            }
                        bool abbreviation = false;
                        bool expectCapitalizedWord = false;
                        bool possibly_initial = false;
                        bool number = false;
                        // Write the non-separating characters.
                        // Check also whether there is reason to think that the string is an abbreviation or(/and) a number.
                        if(URL)
                            {
                            for(;aa <= lastNonSeparatingCharacter;++aa)
                                {
                                Put3(*aa,flgs);
                                }
                            }
                        else
                            {
                            if(Option.tokenize && Option.PennTreebankTokenization)
                                {
                                PennTreebankTokenize(aa,lastNonSeparatingCharacter - aa,abbreviation,flgs,number);
                                }
                            else
                                {
                                for(;aa <= lastNonSeparatingCharacter;++aa)
                                    {
                                    if(*aa == '.')
                                        {
                                        //                                printf("{ABBR . inside}");
                                        abbreviation = true; // . detected inside word
                                        }
                                    else if(isDigit(*aa)) // 20040120
                                        {
                                        number = true;
                                        }
                                    Put3(*aa,flgs); // BBB
                                    }
                                }
                            }
                         wchar_t * SeparatingCharacter = lastNonSeparatingCharacter + 1;
                        if(*SeparatingCharacter == '.') // This is the 
                            // dot immediately following a word or number 
                            // (sequence of one or more non-separating characters)
                            {
                            if(!abbreviation)
                                {
                                if(SeparatingCharacter == firstNonSeparatingCharacter+1) // 20040120 Single characters followed by period are always abbreviations
                                    {
                                    abbreviation = true;
                                    possibly_initial = /*isUpper*/!isLower(*firstNonSeparatingCharacter);
                                    }
                                else if(firstNonSeparatingCharacter > pbuf) // 20040120 word starts with separator, e.g. "(Ref."
                                    {
                                    abbreviation = true;
                                    }
                                else
                                    {
                                    // Check whether the word (including the final dot) is in the list of abbreviations.
                                    wchar_t save = *(SeparatingCharacter+1);
                                    *(SeparatingCharacter+1) = '\0';
#if FILESTREAM
                                    if(Abbreviation(firstNonSeparatingCharacter,expectCapitalizedWord))
                                        {
                                        abbreviation = true;
                                        }
#endif
                                    *(SeparatingCharacter+1) = save;
                                    }
                                }
                            if(abbreviation)
                                {
                                if(possibly_initial)
                                    {
                                    switch(flgs.person_name)
                                        {
                                        case not_a_name:
                                            flgs.person_name = initial;
                                            break;
                                        case initial:
                                            break;
                                            /*case family_name:
                                            person_name = not_a_name;
                                            break;*/
                                        }
                                    }
                                else
                                    flgs.person_name = not_a_name;
                                if(number) // single digit followed by dot.
                                    {
                                    if(flgs.number_final_dot < 3) // 3 is the maximum that can be held in 2 bits.
                                        ++flgs.number_final_dot;
                                    }
                                else
                                    flgs.number_final_dot = 0;
                                Put3(*SeparatingCharacter++,flgs);
                                ch_loc = ' '; // change new line to blank
                                flgs.in_abbreviation = true;
                                flgs.expectCapitalizedWord = expectCapitalizedWord;
                                }
                            else if(number) // string containing digit(s) followed by dot
                                {
                                if(flgs.number_final_dot < 3)
                                    ++flgs.number_final_dot;
                                Put3(*SeparatingCharacter++,flgs);
                                ch_loc = ' '; // change new line to blank
                                flgs.in_abbreviation = true;
                                }
                            }

                        flgs.number_final_dot = 0;
                        for(;SeparatingCharacter < nullbyte;++SeparatingCharacter)
                            {
                            if(Option.tokenize)
                                Put3(' ',flgs);
                            Put3(*SeparatingCharacter,flgs);
                            }
                        }
                    else
                        {
                        PutN(pbuf,nullbyte-pbuf,flgs);
                        }
                    }

                if(h)
                    {
                    *h = '-';
                    Put3(' ',flgs);
                    while(*h == '-')
                        {
                        Put3('-',flgs);
                        ++h;
                        }
                    Put3(' ',flgs);
                    pbuf = h;
                    }
                else
                    break;
                }

            pos = 0;
            }
        if(ch)
            {
            Put3(ch_loc,flgs); // CCC
            }
        }
    else
        {
        if(flgs.firstafterhtmltag)
            {
            flgs.firstafterhtmltag = false;
            if(Option.tokenize && !Option.suppressHTML)
                {
                bufr[pos++] = ' ';
                }
            }
        bufr[pos++] = ch;
        }

    }




void Put(wchar_t ch,flags & flgs) // Called from GetPutBullet and doTheSegmentation
    {
    if(!outputtext)
        return;
    static int last = '\n'; // last character that was not space or tab (we ignore trailing spaces and tabs)
    static int neededSegmentDelimiters = 0; // The number of delimiters needed to finish the current segment.
    // Normally, this number is 1: a single newline '\n'.
    // If Option.emptyline is set, the number is 2: either one sentence delimiter and one newline or
    // two newlines.
    // If a newline is received and the last character wasn't a newline or sentence delimiter,
    // a simple blank is sent to Put2.

    if(flgs.inhtmltag)
        {
        Put2(ch,flgs);
        }
    else
        {
        if(ch == '\n' || ch == '\r')
            {
            switch(last)
                {
                case ';':
                case '?':
                case '.':
                case '!':
#if COLONISSENTENCEDELIMITER
                case ':':
#endif
                    if(Option.emptyline)
                        {
                        --neededSegmentDelimiters; // Do not create empty line if last line ended with sentence delimiter.
                        }
                    if(neededSegmentDelimiters > 0)
                        {
                        Put2('\n',flgs);
                        }
                    --neededSegmentDelimiters;
                    break;
                case '\n':
                    {
                    if(neededSegmentDelimiters > 0)
                        {
                        Put2('\n',flgs);
                        }
                    --neededSegmentDelimiters;
                    }
                    break;
                default:
                    {
                    if(Option.emptyline)
                        {
                        neededSegmentDelimiters = 2; // Insert empty line after line not ending with punctuation. (two \n or sentence delimiter + \n)
                        }
                    else
                        {
                        neededSegmentDelimiters = 1;
                        }
                    Put2(' ',flgs); // Treat newline as a blank
                    }
                }
            }
        else
            {
            if(!isFlatSpace(ch))// ch != ' ' && ch != '\t' && ch != 0xA0)
                {
                if(Option.emptyline)
                    neededSegmentDelimiters = 2;
                else
                    neededSegmentDelimiters = 1;
                }
            Put2(ch,flgs);// AAA
            }
        }
    if(!isFlatSpace(ch))// ch != ' ' && ch != '\t' && ch != 0xA0)
        {
        switch(ch)
            {
            case '\n':
                last = '\n';
                flgs.firstafterhtmltag = false; // Suppress space after tag
                flgs.notstartofline = false;
                break;
            default:
                flgs.notstartofline = true;
                last = ch;
            }
        }
    }

const bool vowels[256] =
    {
        false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  00 -  16 */ /* 00 - 10 */
        false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  16 -  32 */ /* 10 - 20 */
        false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  32 -  48 */ /* 20 - 30 */
        false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  48 -  64 */ /* 30 - 40 */
        false       , true /* A */, false       , false       , false       , true /* E */, false       , false       , false       , true /* I */, false       , false       , false       , false       , false       , true /* O */, /*  64 -  80 */ /* 40 - 50 */
        false       , false       , false       , false       , false       , true /* U */, false       , false       , false       , true /* Y */, false       , false       , false       , false       , false       , false       , /*  80 -  96 */ /* 50 - 60 */
        false       , true /* a */, false       , false       , false       , true /* e */, false       , false       , false       , true /* i */, false       , false       , false       , false       , false       , true /* o */, /*  96 -  112 */ /* 60 - 70 */
        false       , false       , false       , false       , false       , true /* u */, false       , false       , false       , true /* y */, false       , false       , false       , false       , false       , false       , /*  112 -  128 */ /* 70 - 80 */
        false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , true /* Œ */, false       , false       , false       , /*  128 -  144 */ /* 80 - 90 */
        false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , true /* œ */, false       , false       , true /* Ÿ */, /*  144 -  160 */ /* 90 - a0 */
        false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  160 -  176 */ /* a0 - b0 */
        false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  176 -  192 */ /* b0 - c0 */
        true /* À */, true /* Á */, true /* Â */, true /* Ã */, true /* Ä */, true /* Å */, true /* Æ */, false       , true /* È */, true /* É */, true /* Ê */, true /* Ë */, true /* Ì */, true /* Í */, true /* Î */, true /* Ï */, /*  192 -  208 */ /* c0 - d0 */
        false       , false       , true /* Ò */, true /* Ó */, true /* Ô */, true /* Õ */, true /* Ö */, false       , true /* Ø */, true /* Ù */, true /* Ú */, true /* Û */, true /* Ü */, true /* Ý */, false       , false       , /*  208 -  224 */ /* d0 - e0 */
        true /* à */, true /* á */, true /* â */, true /* ã */, true /* ä */, true /* å */, true /* æ */, false       , true /* è */, true /* é */, true /* ê */, true /* ë */, true /* ì */, true /* í */, true /* î */, true /* ï */, /*  224 -  240 */ /* e0 - f0 */
        false       , false       , true /* ò */, true /* ó */, true /* ô */, true /* õ */, true /* ö */, false       , true /* ø */, true /* ù */, true /* ú */, true /* û */, true /* ü */, true /* ý */, false       , true /* ÿ */  /*  240 -  256 */ /* f0 - 100 */
    };


#if COLONISSENTENCEDELIMITER
#define isPunct(s) (s == ';' || s == '?' || s == '.' || s == '!' || s == ':')
#define isSemiPunct(s) (s == ',')
#else
#define isPunct(s) (s == ';' || s == '?' || s == '.' || s == '!')
#define isSemiPunct(s) (s == ',' || s == ':')
#endif

void hyphenate(wchar_t * lastWord,int waited,flags & flgs)
    {
    Put('-',flgs);
    Put('\n',flgs);
    for(int j = 0;j < waited;++j)
        Put(lastWord[j],flgs);
    }

static void considerHyphenation(wchar_t * lastWord,int waited,bool dropHyphen,bool allLowerP,flags & flgs)
    {
    bool vowel = false;
    bool nonAlphaFound = false;
    int cnt = 0;
    int j = 0;
    int punkpos = -1;
    bool allLower = true;
    bool allUpper = true;
    for(j = 0;j < waited && isFlatSpace(lastWord[j]);++j)
        ;
    for(;j < waited;++j)
        {
        ++cnt;
        int k = lastWord[j];
        if(isAlpha(k))
            {
            if(!isLower(k))
                allLower = false;
            if(!isUpper(k))
                allUpper = false;
            }
        else if(k != '-')
            {
            if(punkpos < 0 && isPunct(k))
                punkpos = cnt;
            else if(!isSemiPunct(k))
                nonAlphaFound = true;
            break;
            }
        if(isVowel(k))
            vowel = true;
        }
    if(dropHyphen)
        { // Require agreement of case
        if(allLowerP)
            {
            if(!allLower)
                dropHyphen = false;
            }
        else
            {
            if(!allUpper)
                dropHyphen = false;
            }
        }
    if((!nonAlphaFound && cnt >= 2 && punkpos < 0) || punkpos == cnt)
        {
        if(!dropHyphen || !vowel)
            Put('-',flgs);
        for(j = 0;j < waited;++j)
            Put(lastWord[j],flgs);
        }
    else
        hyphenate(lastWord,waited,flgs);
    }


static void PutHandlingWordWrap(wint_t ch,flags & flgs) // Called from GetPut, GetPutBullet and doTheSegmentation
    {
    static int last = '\n'; // last character that was not space or tab (we ignore trailing spaces and tabs)
    static wchar_t lastWord[256];
    static int lastWordIndex = 0;
    static int wait = 0;
    static int waited = 0;
    static bool dropHyphen = false;
    //static int hyphenFound = 0;
    static bool spaceAfterHyphen = false;
    static bool allLower = false;
    static bool allUpper = false;

    if(flgs.inhtmltag || !Option.wordUnwrap)
        {
        Put(ch,flgs);
        }
    else
        {
        if(ch == '-')
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
                        hyphenate(lastWord,waited,flgs);
                        wait = 0;
                        flgs.hyphenFound = 0;
                        spaceAfterHyphen = false;
                        }
                    Put('\n',flgs);
                    break;
                case '-':
                    {
                    int k;
                    int cnt = 0;
                    bool nonAlphaFound = false;
                    dropHyphen = false;
                    int i;
                    for(i = lastWordIndex-1
                        ;    i != lastWordIndex 
                        && isSpace(lastWord[(i+(sizeof(lastWord)/sizeof(lastWord[0])))%(sizeof(lastWord)/sizeof(lastWord[0]))])
                        ; i = (i+((sizeof(lastWord)/sizeof(lastWord[0]))-1))%(sizeof(lastWord)/sizeof(lastWord[0]))
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

                    for(
                        ;    i != lastWordIndex 
                          && ( k = lastWord[(i+(sizeof(lastWord)/sizeof(lastWord[0])))%(sizeof(lastWord)/sizeof(lastWord[0]))]
                             , !isSpace(k)
                             )
                        ; i = (i+((sizeof(lastWord)/sizeof(lastWord[0]))-1))%(sizeof(lastWord)/sizeof(lastWord[0]))
                        )  // Look at casing of last word in retrograde fashion.
                        {
                        ++cnt;
                        if(Upper)
                            allLower = false;
                        if(isAlpha(k) && k != '-')
                            {
                            if(/*isUpper*/!isLower(k))
                                Upper = true;
                            if(/*isLower*/!isUpper(k))
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
                    if(!nonAlphaFound && cnt >= 2)
                        {
                        /**/
                        lastWordIndex = 0;
                        wait = (sizeof(lastWord)/sizeof(lastWord[0]));
                        waited = 0;
                        }
                    break;
                    }
                default:
                    {
                    if(wait)
                        {
                        considerHyphenation(lastWord,waited,dropHyphen,allLower,flgs);
                        wait = 0;
                        flgs.hyphenFound = 0;
                        spaceAfterHyphen = false;
                        }
                    Put('\n',flgs); // Treat newline as a blank
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
                        hyphenate(lastWord,waited,flgs);
                        wait = 0;
                        flgs.hyphenFound = 0;
                        spaceAfterHyphen = false;
                        Put(ch,flgs);
                        }
                    else
                        {
                        lastWord[waited++] = (wchar_t)ch;
                        }
                    }
                else if(waited > 0)
                    {
                    considerHyphenation(lastWord,waited,dropHyphen,allLower,flgs);
                    wait = 0;
                    flgs.hyphenFound = 0;
                    spaceAfterHyphen = false;
                    Put(ch,flgs);
                    }
                }
            else
                {
                if(!flgs.hyphenFound)
                    Put(ch,flgs);
                }
            }
        }
    if(!isFlatSpace(ch))
        {
        if(ch != '\n' && ch != '-' && flgs.hyphenFound && !wait) // A-bomb
            {
            int k;
            for(k = 0;k < flgs.hyphenFound;++k)
                Put('-',flgs);
            if(spaceAfterHyphen)
                Put(' ',flgs);
            spaceAfterHyphen = false;
            flgs.hyphenFound = 0;
            Put(ch,flgs);
            }
        last = ch;
        }
    else if(flgs.hyphenFound && last != '\n')
        {
        spaceAfterHyphen = true;
        }

    if(!wait)
        {
        if(  !isFlatSpace(ch) 
          || !isFlatSpace(lastWord[(lastWordIndex + ((sizeof(lastWord)/sizeof(lastWord[0]))-1)) % (sizeof(lastWord)/sizeof(lastWord[0]))])
          )
            {
            lastWord[lastWordIndex++] = (wchar_t)ch;
            }
        if(lastWordIndex == (sizeof(lastWord)/sizeof(lastWord[0])))
            lastWordIndex = 0;
        }
    }

static void flushLine(wint_t ch,flags & flgs)
    {
    if(  flgs.inhtmltag
      || !Option.suppressNoise 
      || textBadness(Line,Index) < 0.44
      )
        {
        if(flgs.MindTheSpace != 0 /*&& ch != flgs.MindTheSpace*/)
            {
            assert(Index < sizeof(Line)/sizeof(Line[0]));
            Line[Index++] = flgs.MindTheSpace;
            flgs.MindTheSpace = 0;
            }
        for(size_t i = 0;i < Index;++i)
            {
            PutHandlingWordWrap(Line[i],flgs);        
            }
        PutHandlingWordWrap(ch,flgs);        
        }
    else // skip line, too noisy
        {
        PutHandlingWordWrap('\n',flgs);        
        PutHandlingWordWrap(ch,flgs);        
        }
    Index = 0;
    }

void PutHandlingLine(wint_t ch,flags & flgs) // Called from GetPut, GetPutBullet and doTheSegmentation
    {
    if(!outputtext)
        return;
    if(flgs.inhtmltag)
        {
        flushLine(ch,flgs);
        }
    else
        {
        if(ch == '\n' || ch == '\r')
            {
            if(!flgs.lastEOLchar || flgs.lastEOLchar == ch)
                {
                flushLine('\n',flgs);
                flgs.lastEOLchar = ch;
                }
            else
                {
                // Ignore ch: CRLF or LFCR sequence equals '\n'
                flgs.lastEOLchar = '\0'; 
                }
            }
        else
            {
            if(  ch == WEOF 
              || Index >= sizeof(Line)/sizeof(Line[0]) - 1 // overflow
              )
                {
                flushLine(ch,flgs);
                }
            else
                {
                if(ch == ' ' || ch == '\t') // reduces "     \t\t   \t\t\t" to " \t \t"
                                            // why keep spaces AND tabs, why not reduce to a single space?
                    {
                    if(flgs.MindTheSpace != 0 /*&& ch != flgs.MindTheSpace*/)
                        {
                        assert(Index < sizeof(Line)/sizeof(Line[0]));
                        Line[Index++] = flgs.MindTheSpace;
                        }
                    flgs.MindTheSpace = ch;
                    //Line[Index++] = ch;
                    }
                else if(flgs.MindTheSpace != 0)
                    {
                    assert(Index < sizeof(Line)/sizeof(Line[0]) - 1);
                    Line[Index++] = flgs.MindTheSpace;
                    flgs.MindTheSpace = 0;

                    //assert(Index < sizeof(Line)/sizeof(Line[0]));
                    Line[Index++] = ch;
                    }
                else
                    {
                    assert(Index < sizeof(Line)/sizeof(Line[0]));
                    Line[Index++] = ch;
                    }
                // 20100106 The following does not reduce spaces, but increases their numbers!
                //assert(Index < sizeof(Line)/sizeof(Line[0]));
                //Line[Index++] = ch;
                }
            }
        flgs.lastEOLchar = '\0'; 
        }
    }


wint_t GetPutBullet(const long end_offset,flags & flgs)
    {
    wint_t ch = 0;
    while(Ftell(sourceFile) < end_offset)
        {
        ch = Getc(sourceFile);
        }
    PutHandlingLine(Option.bullet,flgs);
    PutHandlingLine(' ',flgs);
    return ch;
    }

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

bool checkSentenceEnd(int ch,long & begin_offset,lookahead_fnc lookahead,const startLine firsttext,const long curr_pos,flags & flgs)
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
    static bool in_parentheses = false;
    static bool in_number = false;
    static bool in_word = false;
    static bool character_entity = false;

    if(isSpace(ch))
        {
        if(firsttext.EOL)
            {
            begin_offset = curr_pos; //Ftell(sourceFile) - 1;
            }
        if(flgs.bbullet)
            {
            sentenceEnd = true; // force writing of segment
            }
        }
    else if(flgs.bbullet)
        {
        flgs.bbullet = isBullet(ch);// The first bullet-like character is immediately followed by more bullet-like characters.
        }
    else 
        {
        if(firsttext.EOL)
            {
            begin_offset = curr_pos; //Ftell(sourceFile) - 1;
            }
        if(firsttext.b.LS)
            flgs.bbullet = isBullet(ch); // First character on line seems to indicate a bullet.

        }
    static int ellipsis = 0;
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
                if(strchr(" \\/:*?\"<>|",next))
                    flgs.in_fileName = false;
                if(!flgs.in_fileName)
                    {
                    if(next == '.')
                        {
                        ellipsis++;
                        }
                    else if(  !ellipsis
                           && !(  (in_word && isAlpha(next))
                               || (  in_number 
                                  && (  isFlatSpace(next) // next == ' ' || next == 0xA0
                                     || isDigit(next)
                                     )
                                  )
                               )
                           )
                        {
                        sentenceEnd = true;
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
            case '?':
                flgs.in_fileName = false;
                // drop through
            case ';':
                if(character_entity)
                    {
                    character_entity = false;
                    break;
                    }
            case '!':
                sentenceEnd = true;
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
