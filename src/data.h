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
#ifndef DATA_H
#define DATA_H

#if !defined FILESTREAM
#define FILESTREAM 1
#endif

#include <wchar.h>

#if FILESTREAM

#include <stdio.h>
#define STROEM FILE

#define _SEEK_SET SEEK_SET
#define _SEEK_CUR SEEK_CUR
#define _SEEK_END SEEK_END

#if WINCE 
void rewind(FILE * f);
#endif


#else

#define _SEEK_SET 10
#define _SEEK_CUR 11
#define _SEEK_END 12

class Str
    {
    private:
        wchar_t * b;
        long p;
        long len;
        long end;
    public:
        Str()
            {
            p = 0;
            b = new wchar_t[256];
            len = 256;
            end = 0;
            }
        Str(wchar_t * s)
            {
            len = wcslen(s)+1;
            b = new wchar_t[len];
            wcscpy(b,s);
            p = 0;
            end = len - 1;
            }
        Str(const char *filename,const char *mode);
        ~Str()
            {
            delete [] b;
            }
        wchar_t * steal()
            {
            wchar_t * ret = b;
            b = new wchar_t[1];
            b[0] = '\0';
            p = 0;
            len = 1;
            end = 0;
            return ret;
            }
        wint_t get()
            {
            if(p >= end)
                return WEOF;
            return b[p++];
            }
        wint_t unget(wint_t c)
            {
            if(0 < p && p < end)
                p--;
            return c;
            }
        wint_t put(wint_t c)
            {
            if(p >= len - 1)
                {
                wchar_t * nb = new wchar_t[len<<1];
                wcscpy(nb,b);
                delete [] b;
                b = nb;
                len <<= 1;
                }
            b[p++] = c;
            b[p] = '\0';
            end = p;
            return c;
            }
        int fseek(long offset,int origin)
            {
            switch(origin)
                {
                //case _SEEK_SET:
                case _SEEK_SET:
                    if(offset < 0 || offset >= end)
                        return WEOF;
                    else
                        p = offset;
                    break;
                //case _SEEK_CUR:
                case _SEEK_CUR:
                    if(p + offset < 0 || p + offset >= end)
                        return WEOF;
                    else
                        p += offset;
                    break;
                //case _SEEK_END:
                case _SEEK_END:
                    if(end - offset < 0 || end - offset >= end)
                        return WEOF;
                    else
                        p += offset;
                    break;
                }
            return 0;
            }
        long ftell()
            {
            return p;
            }
        void rewind()
            {
            p = 0;
            }
        size_t fread(void *buffer,size_t size,size_t count)
            {
            size_t i;
            switch(size)
                {
                case 1:
                    {
                    char * g = (char *) buffer;
                    for(i = count;i > 0 && (*g++ = get()) != WEOF;--i)
                        ;
                    break;
                    }
                case 2:
                    {
                    short int * g = (short int *) buffer;
                    for(i = count;i > 0 && (*g++ = get()) != WEOF;--i)
                        ;
                    break;
                    }
                case 4:
                    {
                    long * g = (long *) buffer;
                    for(i = count;i > 0 && (*g++ = get()) != WEOF;--i)
                        ;
                    break;
                    }
                default:
                    {
                    char * g = (char *) buffer;
                    for(i = size * count;i > 0 && (*g++ = get()) != WEOF;--i)
                        ;
                    return count/size - i;
                    }
                }
            return count - i;
            }
        int fclose()
            {
            return 0;
            }
        size_t fwrite(const void *buffer,size_t size,size_t count)
            {
            size_t c = count;
            if(size == sizeof(wchar_t))
                {
                wchar_t * wbuf = (wchar_t *)buffer;
                while(c-- > 0)
                    {
                    put(*wbuf++);
                    }
                }
            else if(size == sizeof(char))
                {
                char * pbuf = (char *)buffer;
                while(c-- > 0)
                    {
                    put(*pbuf++);
                    }
                }
            else /* ??? */
                return 0;
            return count;
            }
    };

#define STROEM Str

#endif

typedef wint_t (*getcFnc)(STROEM * fi);
typedef wint_t (*ungetcFnc)(wint_t ch,STROEM * fi); 
typedef int (*fseekFnc)(STROEM * fi,long offset,int origin);
typedef long (*ftellFnc)(STROEM * fi); 
typedef wint_t (*fputcFnc)(wint_t c,STROEM * fo);
typedef int (*fcloseFnc)(STROEM * fi);
typedef void (*frewindFnc)(STROEM * fi);

struct flags;

extern getcFnc Getc;
extern ungetcFnc Ungetc;
extern fseekFnc Fseek;
extern ftellFnc Ftell;
extern fputcFnc Fputc;
extern fcloseFnc Fclose;
extern frewindFnc Frewind;

extern const bool space[256];
// ocrtidy.cpp only:
extern const bool vowels[256];


extern bool isSpace(int s);
extern bool isFlatSpace(int s);
#define isVowel(s) (0 <= s && s < 256 && vowels[s])

#if COLONISSENTENCEDELIMITER
#define isPunct(s) (s == ';' || s == '?' || s == '.' || s == '!' || s == ':')
#define isSemiPunct(s) (s == ',')
#else
#define isPunct(s) (s == ';' || s == '?' || s == '.' || s == '!')
#define isSemiPunct(s) (s == ',' || s == ':')
#endif
#define isSentencePunct(s) (s == '?' || s == '.' || s == '!')
#define isDigit(s) ('0' <= (s) && (s) <= '9')


extern const wchar_t SEP[];
extern const wchar_t QUOTE[]; 
extern const wchar_t QUANTITY[];
extern const wchar_t RBRACKET[];
extern const wchar_t LBRACKET[];

#endif
