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
#include "ocrtidy.h"
#include "commonstuff.h"
#include "letterfunc.h"

static const bool regularNonAlphaNum[256] =
    {
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  00 -  16 */ /* 00 - 10 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  16 -  32 */ /* 10 - 20 */
    false       , true /* ! */, true /* " */, false       , false       , false       , false       , true /* ' */, true /* ( */, true /* ) */, false       , false       , true /* , */, true /* - */, true /* . */, false       , /*  32 -  48 */ /* 20 - 30 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , true /* : */, true /* ; */, false       , false       , false       , true /* ? */, /*  48 -  64 */ /* 30 - 40 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  64 -  80 */ /* 40 - 50 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  80 -  96 */ /* 50 - 60 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  96 -  112 */ /* 60 - 70 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  112 -  128 */ /* 70 - 80 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  128 -  144 */ /* 80 - 90 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  144 -  160 */ /* 90 - a0 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  160 -  176 */ /* a0 - b0 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  176 -  192 */ /* b0 - c0 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  192 -  208 */ /* c0 - d0 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  208 -  224 */ /* d0 - e0 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  224 -  240 */ /* e0 - f0 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false         /*  240 -  256 */ /* f0 - 100 */
    };

static bool RegularNonAlphaNum(int kar)
    {
    if(0 <= kar && kar < 256)
        return regularNonAlphaNum[kar];
    else
        {
        return false;
        /*
        switch(kar)
            {
            case 0x2022:
                return true;
            default:
                return false;
            }
        */
        }
    }

static const bool irregularNonAlphaNum[256] =
    {
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  00 -  16 */ /* 00 - 10 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  16 -  32 */ /* 10 - 20 */
    false       , false       , false       , true /* # */, true /* $ */, true /* % */, true /* & */, false       , false       , false       , true /* * */, true /* + */, false       , false       , false       , true /* / */, /*  32 -  48 */ /* 20 - 30 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , true /* < */, true /* = */, true /* > */, false       , /*  48 -  64 */ /* 30 - 40 */
    true /* @ */, false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  64 -  80 */ /* 40 - 50 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , true /* [ */, true /* \ */, true /* ] */, false       , true /* _ */, /*  80 -  96 */ /* 50 - 60 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  96 -  112 */ /* 60 - 70 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , true /* { */, true /* | */, true /* } */, false       , false       , /*  112 -  128 */ /* 70 - 80 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  128 -  144 */ /* 80 - 90 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  144 -  160 */ /* 90 - a0 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  160 -  176 */ /* a0 - b0 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  176 -  192 */ /* b0 - c0 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  192 -  208 */ /* c0 - d0 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  208 -  224 */ /* d0 - e0 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  224 -  240 */ /* e0 - f0 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  240 -  256 */ /* f0 - 100 */
    };

static bool IrregularNonAlphaNum(int kar)
    {
    if(0 <= kar && kar < 256)
        return irregularNonAlphaNum[kar];
    else
        {
        return false;
        /*
        switch(kar)
            {
            case 0x2022:
                return true;
            default:
                return false;
            }
        */
        }
    }



static const bool veryIrregularNonAlphaNum[256] =
    {
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  00 -  16 */ /* 00 - 10 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  16 -  32 */ /* 10 - 20 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  32 -  48 */ /* 20 - 30 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  48 -  64 */ /* 30 - 40 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  64 -  80 */ /* 40 - 50 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , true /* ^ */, false       , /*  80 -  96 */ /* 50 - 60 */
    true /* ` */, false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  96 -  112 */ /* 60 - 70 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , true /* ~ */, true /*  */, /*  112 -  128 */ /* 70 - 80 */
    true /* 80*/, true /* 81*/, true /* 82*/, true /* 83*/, true /* 84*/, true /* 85*/, true /* 86*/, true /* 87*/, true /* 88*/, true /* 89*/, true /* 8a*/, true /* 8b*/, true /* 8c*/, true /* 8d*/, true /* 8e*/, true /* 8f*/, /*  128 -  144 */ /* 80 - 90 */
    true /* 90*/, true /* 91*/, true /* 92*/, true /* 93*/, true /* 94*/, true /* 95*/, true /* 96*/, true /* 97*/, true /* 98*/, true /* 99*/, true /* 9a*/, true /* 9b*/, true /* 9c*/, true /* 9d*/, true /* 9e*/, true /* 9f*/, /*  144 -  160 */ /* 90 - a0 */
    true /*   */, true /* ¡ */, true /* ¢ */, true /* £ */, true /* ¤ */, true /* ¥ */, true /* ¦ */, true /* § */, true /* ¨ */, true /* © */, true /* ª */, true /* « */, true /* ¬ */, true /* ­ */, true /* ® */, true /* ¯ */, /*  160 -  176 */ /* a0 - b0 */
    true /* ° */, true /* ± */, true /* ² */, true /* ³ */, true /* ´ */, true /* µ */, true /* ¶ */, true /* · */, true /* ¸ */, true /* ¹ */, true /* º */, true /* » */, true /* ¼ */, true /* ½ */, true /* ¾ */, true /* ¿ */, /*  176 -  192 */ /* b0 - c0 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  192 -  208 */ /* c0 - d0 */
    false       , false       , false       , false       , false       , false       , false       , true /* × */, false       , false       , false       , false       , false       , false       , false       , false       , /*  208 -  224 */ /* d0 - e0 */
    false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  224 -  240 */ /* e0 - f0 */
    false       , false       , false       , false       , false       , false       , false       , true /* ÷ */, false       , false       , false       , false       , false       , false       , false       , false       , /*  240 -  256 */ /* f0 - 100 */
    };

static bool VeryIrregularNonAlphaNum(int kar)
    {
    if(0 <= kar && kar < 256)
        return veryIrregularNonAlphaNum[kar];
    else
        {
        switch(kar)
            {
            case 0x2022:
                return true;
            default:
                return false;
            }
        }
    }

double textBadness(const wchar_t * line,size_t len)
    {
    if(len < 2)
        return 0;
    int bad = 0;
    while((len > 0 && *line == ' ') || *line == '\t' || *line == 0x3000)
        {
        --len;
        ++line;
        }
    while(len > 0 && isFlatSpace(line[len-1]))
        --len;
    size_t k;
    size_t chars[256];
    for(k = 0;k < 256;++k)
        chars[k] = 0;
    
    for(k = 0;k < len;++k)
        {
        wint_t i = line[k];
        if(0 <= i && i < sizeof(chars))
            ++chars[i];
        }
    if(len > 30)
        {
        if(len/30 > chars[(int)' '] + chars[(int)'\t'] + 1)
            {
            //printf("at least one blank for every 30 characters\n");
            bad += 4; // at least one blank for every 30 characters
            }
        }
    for(k = 0;k + 2 < len;++k)
        {
        int kar = line[k];
        if(!isFlatSpace(kar))
            {
            if(kar == line[k+1] && kar == line[k+2])
                {
                //printf("string of %c%c%c\n",kar,kar,kar);
                bad += 5; // 3 equal letters on a row
                }
            }
        }
    bool hasVowel = false;
    bool hasConsonant = false;
    int numConsonant = 0;
    int numVowel = 0;
    int numConsonantOnArow = 0;
    //bool hasUpper = false;
    bool hasLower = false;
    bool hasUpperInTheMiddle = false;
    bool first = true;
    for(k = 0;k < len + 1;++k)
        {
        int kar;
        if(k == len || (kar = line[k]) == ' ')
            {
            if(hasConsonant)
                {
                if(hasVowel)
                    {
                    if(numConsonant > 4 && numConsonant > numVowel * 5)
                        {
                        //printf("word with many consonants per vowel\n");
                        bad += 2;    // word with many consonants per vowel
                        }
                    if(numConsonantOnArow > 5)
                        {
                        //printf("word with many consonants on a row\n");
                        bad += 2;    // word with many consonants per vowel
                        }
                    }
                else
                    {
                    //printf("word without vowel\n");
                    bad += 3;    // word without vowel
                    }
                }
            if(hasUpperInTheMiddle/*hasUpper && hasLower*/)
                {
                //printf("mixed case\n");
                bad += 4; // mixed case
                }
            hasConsonant = hasVowel = false;
           /* hasUpper =*/ hasLower = false;
            hasUpperInTheMiddle = false;
            first = true;
            numConsonant = 0;
            numVowel = 0;
            numConsonantOnArow = 0;
            }
        else if(k < len)
            {
            if(isAlpha(kar))
                {
                if(isVowel(kar))
                    {
                    hasVowel = true;
                    ++numVowel;
                    numConsonantOnArow = 0;
                    }
                else
                    {
                    hasConsonant = true;
                    ++numConsonant;
                    ++numConsonantOnArow;
                    }
                if(!isUpper(kar))
                    hasLower = true;
                else if(!first && !isLower(kar))
                    {
                   // hasUpper = true;
                    if(hasLower)
                        hasUpperInTheMiddle = true;
                    }
                }
            else
                {
                hasConsonant = hasVowel = true; // forgiving if non-word
                if(RegularNonAlphaNum(kar))
                //if(regularNonAlphaNum[kar])
                    {
                    //printf("regularNonAlphaNum\n");
                    ++bad;
                    }
                else if(IrregularNonAlphaNum(kar))
                //else if(irregularNonAlphaNum[kar])
                    {
                    //printf("irregularNonAlphaNum\n");
                    bad+=2;
                    }
                else if(VeryIrregularNonAlphaNum(kar))
                //else if(veryIrregularNonAlphaNum[kar])
                    {
                    bad+=3;
                    }
                }
            first = false;
            }
        }
    if(len == 0)
        return 0.0;
    else
        {
        //printf("bad %d len %d\n",bad,len);
        return (double)bad / ((double)len + 2); // a single comma gives 0.333
        }
    }
