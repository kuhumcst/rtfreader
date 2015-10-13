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
#include "segment.h"
#include "letterfunc.h"
#include "flags.h"
//#include "commonstuff.h"
#include "option.h"
#include "abbrev.h"
#include <assert.h>

static void Shift(wchar_t *end,int save,wchar_t buf2[256],int offset)
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
void segment::PennTreebankTokenize(STROEM * file, wchar_t buf[],size_t lastNonSeparatingCharacter,bool & abbreviation,flags & flgs ,bool & number)
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
    Dots.PutN(file,buf2,/*strlen*/ wcslen(buf2),flgs);
    }


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

static EndOfSegmentIndication checkWhetherCharIsPunctuation(wint_t ch)
    {
    switch(ch)
        {
        case '.':
        case ';':
            return period_seen;
        case '?':
        case '!':
#if COLONISSENTENCEDELIMITER
        case ':':
#endif
            return other_punctuation_seen;
//            return period_seen;
        default:
            return no_indication;
        }
    }

static size_t addToBuffer(wint_t ch,flags & flgs,wchar_t * bufr,size_t pos)
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
    return pos;
    }

void segment::perhapsWriteSegmentDelimiter(STROEM * file,wint_t ch,flags & flgs,wint_t ch_loc)
    {
    if(neededSegmentDelimiters > 0)
        {
        if(  Option.emptyline 
          && (  SegmentEndIndication == period_seen
             || SegmentEndIndication == other_punctuation_seen
             )
          )
            {
            --neededSegmentDelimiters; // Do not create empty line if last line ended with sentence delimiter.
            }

        if(ch && neededSegmentDelimiters > 0)
            {
            neededSegmentDelimiters -= Dots.getTrailingDotsFollowingNumber();
            if((SegmentEndIndication == period_seen || SegmentEndIndication == other_punctuation_seen) && ch != '\n')
                Dots.Put3(file,'\n',flgs);
            else
                Dots.Put3(file,ch_loc,flgs);
            neededSegmentDelimiters += Dots.getTrailingDotsFollowingNumber();
            }
        }
    }

wint_t segment::bracketsDotsEntitiesInitialsAbbreviations(STROEM * file,flags & flgs,wchar_t * pbuf,wchar_t * nullbyte,wint_t ch_loc)
    {
    //size_t  firstNonSeparatingCharacter,lastNonSeparatingCharacter,a2,b2;
    wchar_t * firstNonSeparatingCharacter;
    wchar_t * lastNonSeparatingCharacter;
    wchar_t * a2;
    wchar_t * b2;
    bool lquote = false;
    bool rquote = false;
    wchar_t * rbracket = NULL;

    // Find left quote while scanning separating characters from the beginning of the string.
    for(firstNonSeparatingCharacter = pbuf;*firstNonSeparatingCharacter && isSep(*firstNonSeparatingCharacter);++firstNonSeparatingCharacter)
        {
        if(wcschr(QUOTE,*firstNonSeparatingCharacter))
            {
            if(flgs.person_name == initial)
                flgs.person_name = not_a_name;
            lquote = true;
            }
        }

    // Find right quote and right bracket while scanning separating characters from the end of the string.
    for(lastNonSeparatingCharacter = nullbyte - 1;lastNonSeparatingCharacter >= firstNonSeparatingCharacter && isSep(*lastNonSeparatingCharacter);--lastNonSeparatingCharacter)
        {
        if(!rbracket && wcschr(RBRACKET,*lastNonSeparatingCharacter))
            rbracket = lastNonSeparatingCharacter;

        if(wcschr(QUOTE,*lastNonSeparatingCharacter))
            rquote = true;
        else
            rquote = false;
        }

    // Left bracket can be inside the word. So we could not be sure to find it by only scanning separating characters from the left.
    // If there is a right bracket, we look for the left bracket. If it is found in the word, both brackets are considered part of the word.
    // Otherwise right and left brackets are separated from the word.
    if(rquote && !lquote)
        ++lastNonSeparatingCharacter; // A right quote without corresponding left quote is regarded part of the word, ikk'
    else if(rbracket)
        {
        wchar_t * L = wcspbrk(firstNonSeparatingCharacter,LBRACKET);
        if(L && L < rbracket)
            lastNonSeparatingCharacter = rbracket; // found something like opfinder(e)
        }

    if(isDigit(*firstNonSeparatingCharacter))
        {
        if(flgs.person_name == initial)
            flgs.person_name = not_a_name;
        for(;lastNonSeparatingCharacter >= firstNonSeparatingCharacter && isQuantity(*lastNonSeparatingCharacter);--lastNonSeparatingCharacter)
            {//trailing dollar, percent, euro, florin, promille, cent, pound, yen are put in separate tokens
            //TODO preceding valuta.
            }
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
                if(flgs.person_name == initial)
                    flgs.person_name = not_a_name;
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
                if(flgs.person_name == initial)
                    flgs.person_name = not_a_name;
                URL = true;
                firstNonSeparatingCharacter = a2;     // Not a separator after all.
                extended = true;
                break;
                }           // e.g. ..[tmp/drive.txt/
            // The first .. is part of the text because there is a . inside the string
        }

    // See whether there are XML or HTML entities that need to be kept together internally and with a word.
    if(firstNonSeparatingCharacter <= lastNonSeparatingCharacter)
        {
        if(*(lastNonSeparatingCharacter+1) == ';')
            {
            wchar_t * p;
            if(  pbuf <= (p=wcschr(firstNonSeparatingCharacter,'&'))
                && p <= lastNonSeparatingCharacter // b&amp;
                )
                {
                ++lastNonSeparatingCharacter;// html character entity, for example &amp;
                neededSegmentDelimiters = Option.emptyline ? 2 : 1; // Insert empty line after line not ending with punctuation. (two \n or sentence delimiter + \n)
                SegmentEndIndication = no_indication;
                }
            else if(  pbuf < firstNonSeparatingCharacter
                   && *(firstNonSeparatingCharacter-1) == '&' // If it is decided that '&' is a separator
                   )
                {                               // &amp;
                --firstNonSeparatingCharacter;
                ++lastNonSeparatingCharacter;
                neededSegmentDelimiters = Option.emptyline ? 2 : 1; // Insert empty line after line not ending with punctuation. (two \n or sentence delimiter + \n)
                SegmentEndIndication = no_indication;
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
                {
                --firstNonSeparatingCharacter;// html character entity, for example &amp;cetera
                neededSegmentDelimiters = Option.emptyline ? 2 : 1; // Insert empty line after line not ending with punctuation. (two \n or sentence delimiter + \n)
                SegmentEndIndication = no_indication;
                }
            }

        wchar_t * aa;
        // Write the separating characters at the beginning, all separated if tokenization is wanted.
        for(aa = pbuf;aa < firstNonSeparatingCharacter;++aa)
            {
            Dots.Put3(file,*aa,flgs);
            if(Option.tokenize)
                Dots.Put3(file,' ',flgs);
            }

        bool abbreviation = false;
        bool possibly_initial = false;
        bool number = false;
        bool singleCharacter = false;
        // Write the non-separating characters.
        // Check also whether there is reason to think that the string is an abbreviation or(/and) a number.
        if(URL)
            {
            for(;aa <= lastNonSeparatingCharacter;++aa)
                {
                Dots.Put3(file,*aa,flgs);
                }
            }
        else if(Option.tokenize && Option.PennTreebankTokenization)
            {
            PennTreebankTokenize(file,aa,lastNonSeparatingCharacter - aa,abbreviation,flgs,number);
            }
        else
            {
            for(;aa <= lastNonSeparatingCharacter;++aa)
                {
                if(*aa == '.')
                    {
                    abbreviation = true; // . detected inside word
                    }
                else if(isDigit(*aa))
                    {
                    number = true;
                    }
                Dots.Put3(file,*aa,flgs);
                }
            }
            
        wchar_t * SeparatingCharacter = lastNonSeparatingCharacter + 1;
        if(*SeparatingCharacter == '.') // This is the 
            // dot immediately following a word or number 
            // (sequence of one or more non-separating characters)
            {
            bool expectCapitalizedWord = false;
            if(!abbreviation)
                {
                if(lastNonSeparatingCharacter == firstNonSeparatingCharacter) // 20040120 Single characters followed by period are always abbreviations
                    {
                    abbreviation = true;
                    singleCharacter = true;
                    possibly_initial = isUpper(*firstNonSeparatingCharacter) && !isLower(*firstNonSeparatingCharacter);
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
                neededSegmentDelimiters = Option.emptyline ? 2 : 1; // Insert empty line after line not ending with punctuation. (two \n or sentence delimiter + \n)
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
                if(number || singleCharacter) // single digit or character followed by dot.
                    {
                    if(flgs.number_final_dot < 3) // 3 is the maximum that can be held in 2 bits.
                        ++flgs.number_final_dot;
                    }
                else
                    flgs.number_final_dot = 0;
                ch_loc = ' '; // change new line to blank
             //   ch_loc = ch;
                SegmentEndIndication = no_indication;
                flgs.in_abbreviation = true;
                flgs.expectCapitalizedWord = expectCapitalizedWord;
                Dots.Put3(file,*SeparatingCharacter++,flgs);
                }
            else if(number) // string containing digit(s) followed by dot
                {
                neededSegmentDelimiters = Option.emptyline ? 2 : 1; // Insert empty line after line not ending with punctuation. (two \n or sentence delimiter + \n)
                if(flgs.number_final_dot < 3)
                    ++flgs.number_final_dot;
                ch_loc = ' '; // change new line to blank
                //ch_loc = ch;
                SegmentEndIndication = no_indication;
                flgs.in_abbreviation = true;
                Dots.Put3(file,*SeparatingCharacter++,flgs);
                }
            }

        flgs.number_final_dot = 0;
        for(;SeparatingCharacter < nullbyte;++SeparatingCharacter)
            {
            if(Option.tokenize)
                Dots.Put3(file,' ',flgs);
            wchar_t kar = *SeparatingCharacter;
            for(;;)
                {
                Dots.Put3(file,kar,flgs);
                if(SeparatingCharacter+1 < nullbyte && *(SeparatingCharacter+1) == kar)
                    ++SeparatingCharacter;
                else
                    break;
                }
            flgs.in_abbreviation = false;
            abbreviation = false;
            SegmentEndIndication = checkWhetherCharIsPunctuation(*SeparatingCharacter);
            if(neededSegmentDelimiters > 0)
                {
                if(  Option.emptyline 
                  && (  SegmentEndIndication == period_seen
                     || SegmentEndIndication == other_punctuation_seen
                     )
                  )
                    {
                    --neededSegmentDelimiters; // Do not create empty line if last line ended with sentence delimiter.
                    }
                }
            }
        if(neededSegmentDelimiters > 0)
            {
            if((SegmentEndIndication == period_seen || SegmentEndIndication == other_punctuation_seen))
                {
                neededSegmentDelimiters -= Dots.getTrailingDotsFollowingNumber();
                Dots.Put3(file,'\n',flgs);
                neededSegmentDelimiters += Dots.getTrailingDotsFollowingNumber();
                }
            }
        }
    else
        {
        Dots.PutN(file,pbuf,nullbyte-pbuf,flgs);
        }
    return ch_loc;
    }

void segment::lookWhatsInTheBuffer(STROEM * file,wint_t ch,flags & flgs)
    {
    // Check for brackets, quotes, valuta and other quantities,
    // abbreviations and numbers.

    // lookWhatsInTheBuffer generally writes a newline for each received
    // newline. Exceptions are newlines emitted after numbers and abbreviations
    // with trailing dots, in which case the newline is replaced by a white
    // space. However, this whitespace is possibly changed back to a newline.
    // This happens if a trailing dot after a number is followed by an
    // upper case letter, indicating the start of a sentence.
    /*
    if(Option.emptyline && ch == '\n' && (SegmentEndIndication == other_punctuation_seen))
        {
        --neededSegmentDelimiters; // Do not create empty line if last line ended with sentence delimiter.
        }*/
    if(neededSegmentDelimiters <= 0)
        return;

    wint_t ch_loc = ch;
    if(pos)
        {
        bufr[pos] = '\0';
        wchar_t * pbuf = bufr;
        wchar_t * h = 0;

        for ( pbuf = bufr
            ; 0 != (h = wcsstr(pbuf,L"--"))
            ; pbuf = h
            )
            {// Check for sequences of hyphens and put them in separate tokens (if necessary)
            EndOfSegmentIndication SegmentEndIndicationLater = SegmentEndIndication;
            SegmentEndIndication = no_indication;
            *h = '\0';
            wchar_t * nullbyte = h;
            if(pbuf < nullbyte)
                ch_loc = bracketsDotsEntitiesInitialsAbbreviations(file,flgs,pbuf,nullbyte,ch_loc);
            *h = '-';
            Dots.Put3(file,' ',flgs);
            while(*h == '-')
                {
                Dots.Put3(file,'-',flgs);
                ++h;
                }
            Dots.Put3(file,' ',flgs);
            SegmentEndIndication = SegmentEndIndicationLater;
            }

        if(*pbuf)
            {
            wchar_t * nullbyte = bufr+pos;
            ch_loc = bracketsDotsEntitiesInitialsAbbreviations(file,flgs,pbuf,nullbyte,ch_loc);
            }

        perhapsWriteSegmentDelimiter(file,ch,flgs,ch_loc);
        pos = 0;
        }
    else
        {
        perhapsWriteSegmentDelimiter(file,ch,flgs,ch_loc);
        }
    }

void segment::Put(STROEM * file,wchar_t ch,flags & flgs) // Called from GetPutBullet and doTheSegmentation
    {
    if(!file)
        return;

    if(!fileStarted)
        {
        pos = 0;
        bufr[0] = '\0';
        fileStarted = true;
        }

    if(pos >= (sizeof(bufr)/sizeof(bufr[0])) - 1)
        {
        Dots.PutN(file,bufr,(sizeof(bufr)/sizeof(bufr[0])) - 1,flgs);
        pos = 0;
        bufr[0] = '\0';
        }

    if(flgs.inhtmltag)
        {
        if(Option.emptyline)
            neededSegmentDelimiters = 2;
        else
            neededSegmentDelimiters = 1;
        if(flgs.firstofhtmltag)
            {
            if(Option.tokenize && !Option.suppressHTML && notstartofline)
                {
                bufr[pos++] = ' ';
                }
            flgs.firstofhtmltag = false;
            }
        bufr[pos++] = ch;
        bufr[pos] = '\0';
        Dots.PutN(file,bufr,pos,flgs);
        pos = 0;
        bufr[0] = '\0';
        }
    else if(ch == '\n' || ch == '\r')
        {
        switch(SegmentEndIndication)
            {
            case period_seen:
            case other_punctuation_seen:
                {
                lookWhatsInTheBuffer(file,'\n',flgs);
                --neededSegmentDelimiters;
                switch(ch)
                    {
                    case '\n':
                        SegmentEndIndication = newline_seen;
                        flgs.firstafterhtmltag = false; // Suppress space after tag
                        notstartofline = false;
                        break;
                    default:
                        notstartofline = true;
                        SegmentEndIndication = checkWhetherCharIsPunctuation(ch);
                    }
                break;
                }
            case newline_seen:
                {
                lookWhatsInTheBuffer(file,'\n',flgs);
                --neededSegmentDelimiters;
                switch(ch)
                    {
                    case '\n':
                        SegmentEndIndication = newline_seen;
                        flgs.firstafterhtmltag = false; // Suppress space after tag
                        notstartofline = false;
                        break;
                    default:
                        notstartofline = true;
                        SegmentEndIndication = checkWhetherCharIsPunctuation(ch);
                    }
                break;
                }
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
                lookWhatsInTheBuffer(file,' ',flgs);
                switch(ch)
                    {
                    case '\n':
                        SegmentEndIndication = newline_seen;
                        flgs.firstafterhtmltag = false; // Suppress space after tag
                        notstartofline = false;
                        break;
                    default:
                        notstartofline = true;
                        SegmentEndIndication = checkWhetherCharIsPunctuation(ch);
                    }
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

        if(flgs.htmltagcoming || wcschr(L" \t",ch) || ch == 0)
            {
            lookWhatsInTheBuffer(file,ch,flgs);
            }
        else
            {
            pos = addToBuffer(ch,flgs,bufr,pos);
            }

        if(!isFlatSpace(ch))// ch != ' ' && ch != '\t' && ch != 0xA0)
            {
            notstartofline = true;
            SegmentEndIndication = checkWhetherCharIsPunctuation(ch);
            }
        }
    }

