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
#include "data.h"
#include <string.h>

/*This source is also used in OCRtidy*/


#if FILESTREAM
#if WINCE 
void rewind(FILE * f)
    {
    fseek( f, 0L, SEEK_SET );
    }
#endif
#else
Str::Str(const char *filename,const char *mode)
    {
    len = strlen(filename)+1;
    b = new wchar_t[len];
    for(int i = 0;i < len;++i)
        {
        b[i] = filename[i];
        }
    p = 0;
    end = len - 1;
    if(strchr(mode,'a'))
        {
        p = end;
        }
    }
#endif

static wint_t _getc_(STROEM * file)
    {
#if FILESTREAM
    int ret = getc(file);
    if(ret == EOF)
        return WEOF;
    else
        return (wint_t)ret;
#else
    wint_t ret = file->get();
    if(ret == WEOF)
        return WEOF;
    else
        return ret;
#endif
    }

static wint_t _ungetc_(wint_t ch,STROEM * file)
    {
#if FILESTREAM
    int ret = ungetc(ch,file);
    if(ret == EOF)
        return WEOF;
    else
        return (wint_t)ret;
#else
    return file->unget(ch);
#endif
    }

static wint_t _fputc_(wint_t ch,STROEM * file)
    {
#if FILESTREAM
    int ret = fputc(ch,file);
    if(ret == EOF)
        return WEOF;
    else
        return (wint_t)ret;
#else
    return file->put(ch);
#endif
    }

static int _fseek_(STROEM * file,long offset,int origin)
    {
#if FILESTREAM
    return fseek(file,offset,origin);
#else
    return file->fseek(offset,origin);
#endif
    }

static long _ftell_(STROEM * file)
    {
#if FILESTREAM
    return ftell(file);
#else
    return file->ftell();
#endif
    }

static void _rewind_(STROEM * file)
    {
#if FILESTREAM
    rewind(file);
#else
    file->rewind();
#endif
    }

static int _fclose_(STROEM * file)
    {
#if FILESTREAM
    return fclose(file);
#else
    return file->fclose();
#endif
    }

getcFnc Getc = _getc_; // read one byte from a file or read one wide character from a string
ungetcFnc Ungetc = _ungetc_;
fseekFnc Fseek = _fseek_;
ftellFnc Ftell = _ftell_;
fputcFnc Fputc = _fputc_;
frewindFnc Frewind = _rewind_;
fcloseFnc Fclose = _fclose_;

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
        false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , true /* � */, false       , false       , false       , /*  128 -  144 */ /* 80 - 90 */
        false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , true /* � */, false       , false       , true /* � */, /*  144 -  160 */ /* 90 - a0 */
        false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  160 -  176 */ /* a0 - b0 */
        false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , false       , /*  176 -  192 */ /* b0 - c0 */
        true /* � */, true /* � */, true /* � */, true /* � */, true /* � */, true /* � */, true /* � */, false       , true /* � */, true /* � */, true /* � */, true /* � */, true /* � */, true /* � */, true /* � */, true /* � */, /*  192 -  208 */ /* c0 - d0 */
        false       , false       , true /* � */, true /* � */, true /* � */, true /* � */, true /* � */, false       , true /* � */, true /* � */, true /* � */, true /* � */, true /* � */, true /* � */, false       , false       , /*  208 -  224 */ /* d0 - e0 */
        true /* � */, true /* � */, true /* � */, true /* � */, true /* � */, true /* � */, true /* � */, false       , true /* � */, true /* � */, true /* � */, true /* � */, true /* � */, true /* � */, true /* � */, true /* � */, /*  224 -  240 */ /* e0 - f0 */
        false       , false       , true /* � */, true /* � */, true /* � */, true /* � */, true /* � */, false       , true /* � */, true /* � */, true /* � */, true /* � */, true /* � */, true /* � */, false       , true /* � */  /*  240 -  256 */ /* f0 - 100 */
    };


wchar_t const SEP[] = L"!\"\'(),./:;?[]`{}\x82\x83\x84\x85\x86\x87\x88\x89\x91\x92\x93\x94\x95\x96\x97\x98\xab\xbb\x02B9\x02BA\x02BB\x02BC\x02BD";
wchar_t const QUOTE[] = L"\'`\x91\x92\x02B9\x02BA\x02BB\x02BC\x02BD";
wchar_t const QUANTITY[] = L"$%\x80\x83\x89\xA2\xA3\xA5";//dollar, percent, euro, florin, promille, cent, pound, yen 
wchar_t const RBRACKET[] = L")}]";
wchar_t const LBRACKET[] = L"({[";

bool isSpace(int s)
    {
    bool ret;
    ret = (0 <= s && s < 256 && space[s]) || s == 0x3000;
    return ret;
    }
bool isFlatSpace(int s)
    {
    bool ret;
    ret = (s == '\t' || s == ' ' || s == 0xA0 || s == 0x3000 || s == '\f');
    return ret;
    }
