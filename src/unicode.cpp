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

/*This source is also used in OCRtidy*/

#include "data.h"

#if FILESTREAM

#include "unicode.h"
#include <assert.h>
#include <stdlib.h>

/*static char entity[30];
static int entp = -1;*/


static int FseekUNICODE(FILE * file,long offset,int origin)
    {
//    entp = -1;
    return fseek(file,offset,origin);
    }

static wint_t UngetUNICODE(wint_t ch,FILE * fi)
    {
    fseek(fi,-2,/*_*/_SEEK_CUR);
//    entp = -1;
    return ch;
    }

static wint_t LittleEndianFputc(wint_t c,FILE * fo)
    {
    fputc(c & 0xff,fo);
    fputc((c >> 8),fo);
    return c;
    }

static wint_t BigEndianFputc(wint_t c,FILE * fo)
    {
    fputc((c >> 8),fo);
    fputc(c & 0xff,fo);
    return c;
    }

static int UTF8start = 0;

static void FrewindUtf8(FILE * f)
    {
    fseek(f,UTF8start,SEEK_SET);
    }

static void FrewindUnicode(FILE * f)
    {
    fseek(f,2,SEEK_SET);
    }

static fputcFnc endian = LittleEndianFputc;

static wint_t PutEndian(wint_t c,FILE * fo)
    {
    if(c == '\n')
        endian('\r',fo); // why?

#if WINCE 
#else
#if WCHAR_MAX > 0xffff // not windows
    if(0x10000 <= c && c <= 0x10FFFF) // need surrogate pair
        {// Only if sizeof(wint_t) > 2!
        c -= 0x10000; // 1111  1111 1111  1111 1111
        int h = (c >> 10) | 0xD800;   // 1101 10 00 0000 0000
        int l = (c & 0x3ff) | 0xDC00; // 1101 11 00 0000 0000
        endian(h,fo);
        endian(l,fo);
        }
    else
#endif
#endif
        {
        endian(c,fo);
        }
    return c;
    }

#if 0
static wint_t doTheHTMLentityThing(wint_t c,FILE * fo,fputcFnc putToFile)
    {
    static char/*wchar_t*/ buf[20];
    static size_t bufi = 0;
    static int base = 10;
    if(bufi >= sizeof(buf))
        {
        for(size_t i = 0;i < bufi;++i)
            {
            putToFile(buf[i],fo);
            }
        bufi = 0;
        }
    if(bufi)
        {
        if(c == ';')
            {
            buf[bufi] = 0;
            long k = strtol/*wcstol*/(buf+2+(base>>4),NULL,base);
            putToFile((wint_t)k,fo);
            }
        else if(c == '#' && bufi == 1)
            {
            buf[bufi++] = '#';
            base = 10;
            }
        else if('0' <= c && c <= '9')
            {
            buf[bufi++] = (char)c;
            }
        else if(base == 16 && ('a' <= c && c <= 'f' || 'A' <= c && c <= 'F'))
            {
            buf[bufi++] = (char)c;
            }
        else if(bufi == 2 && (c == 'x' || c == 'X'))
            {
            base = 16;
            buf[bufi++] = (char)c;
            }
        else
            {
            for(size_t i = 0;i < bufi;++i)
                {
                putToFile(buf[i],fo);
                }
            bufi = 0;
            putToFile(c,fo);
            }
        }
    else
        {
        if(c == '&')
            {
            buf[bufi++] = '&';
            }
        else
            {
            putToFile(c,fo);
            }
        }
    return c;
    }

static wint_t FputcUnicodeHTMLentity(wint_t c,FILE * fo)
    {
    static bool bommed = false;
    if(!bommed)
        {
        PutEndian(0xfeff,fo);
        bommed = true;
        }
    return doTheHTMLentityThing(c,fo,PutEndian);
    }
#endif

static wint_t FputcUnicode(wint_t c,FILE * fo)
    {
    static bool bommed = false;
    if(!bommed)
        {
        PutEndian(0xfeff,fo);
        bommed = true;
        }
    return PutEndian(c,fo);
    }

static void UnicodeToUtf8(int w,FILE * fo )
    {
    // Convert unicode to utf8
    if ( w < 0x0080 )
        fputc(w,fo);
    else 
        {
        if(w < 0x0800) // 7FF = 1 1111 111111
            {
            fputc(0xc0 | (w >> 6 ),fo);
            }
        else
            {
            if(w < 0x10000) // FFFF = 1111 111111 111111
                {
                fputc(0xe0 | (w >> 12),fo);
                } 
            else // w < 110000
                { // 10000 = 010000 000000 000000, 10ffff = 100 001111 111111 111111
                fputc(0xf0 | (w >> 18),fo);
                fputc(0x80 | ((w >> 12) & 0x3f),fo);
                } 
            fputc(0x80 | ((w >> 6) & 0x3f),fo);
            }
        fputc(0x80 | (w & 0x3f),fo);
        }
    }

static wint_t PutUTF8(wint_t s,FILE * fo )
    {
    static int prev = 0;
    if(prev == 0)
        {
        if((s & 0xFC00) == 0xD800) // first word surrogat
            {
            prev = s;
            }
        else
            {
            if(s > 0)
                UnicodeToUtf8(s,fo);
            }
        }
    else
        {
        if((s & 0xFC00) == 0xDC00) // second word surrogat
            {
            s = (wint_t)((s & 0x3ff) + ((prev & 0x3ff) << 10) + 0x10000);
            UnicodeToUtf8(s,fo);
            }
        else
            {
            // Assume it is UCS-2, not UTF-16
            UnicodeToUtf8(prev,fo);
            if(s > 0)
                UnicodeToUtf8(s,fo); // You can call surrogate with a zero or -1 to empty prev.
            }
        prev = 0;
        }
    return s;
    }

#if 0
static wint_t FputcUTF8HTMLentity(wint_t c,FILE * fo)
    {
    return doTheHTMLentityThing(c,fo,PutUTF8);
    }
#endif

static wint_t FputcUTF8(wint_t c,FILE * fo)
    {
    return PutUTF8(c,fo);
    }

static wint_t littleendian(FILE * fi)
    {
    int kar;
    /*unsigned*/ wint_t s;

/*    if(entp >= 0)
        {
        wint_t ret = entity[entp++];
        if(!entity[entp])
            entp = -1;
        return ret;
        }
*/
    if((kar = getc(fi)) != EOF)
        {
        s = (wint_t)kar;
        if((kar = getc(fi)) == EOF)
            return WEOF;
        s += (wint_t)(kar << 8);
/*        if(s > 0xff)
            {
            sprintf(entity,"#%x;",s);
            entp = 0;
            return '&';
            }
        else*/
            return s;
        }
    return WEOF;
    }

static wint_t bigendian(FILE * fi)
    {
    int kar;
    /*unsigned*/ wint_t s;
    if((kar = getc(fi)) != EOF)
        {
        s = (wint_t)(kar << 8);
        if((kar = getc(fi)) == EOF)
            return WEOF;
        s += (wint_t)kar;
        /*
        if(s > 0xff)
            {
            sprintf(entity,"#%x;",s);
            entp = 0;
            return '&';
            }
        else*/
            return s;
        }
    return WEOF;
    }

static wint_t copy(FILE * fi)
    {
    int ret = getc(fi);
    return ret == EOF ? WEOF : (wint_t)ret;
    }

/*
static void bitpat(int c,int n)
    {
    for(int i = n; --i >= 0;)
        {
        if(c & (1 << i))
            putchar('1');
        else
            putchar('0');
        if(!(i % 8))
            putchar(' ');
        if(!(i % 4))
            putchar(' ');
        }
    }
*/

static long lastPos = 0L;

static wint_t UngetUTF8(wint_t ch,FILE * fi)
    {
    fseek(fi,lastPos,/*_*/_SEEK_SET);
    return ch;
    }

static wint_t UTF8(FILE * fi)
    {
#if WINCE 
#else
#if WCHAR_MAX == 0xffff // windows
    static wint_t next = 0;
    if(next)
        {
        wint_t ret = next;
        next = 0;
        return ret;
        }        
#endif
#endif
    wint_t k[6];
    lastPos = ftell(fi);
    int kk;
    if((kk = getc(fi)) != EOF)
        {
        k[0] = (wint_t)kk;
        if((k[0] & 0xc0) == 0xc0) // 11bbbbbb
            {
            int i = 1;
            
            for(wint_t k0 = k[0];(k0 << i) & 0x80;++i)
                {
                k[i] = (wint_t)getc(fi);
                }
            long int K = ((k[0] << i) & 0xff) << (5 * i - 6);
            int I = --i;
            while(i > 0)
                {
                K |= (k[i] & 0x3f) << ((I - i) * 6);
                --i;
                }
#if WINCE 
#else
#if WCHAR_MAX == 0xffff // windows
            if(0x10000 <= K && K <= 0x10FFFF) // need surrogate pair
                {
                K -= 0x10000; // 1111  1111 1111  1111 1111
                int h = (K >> 10) | 0xD800;   // 1101 10 00 0000 0000
                int l = (K & 0x3ff) | 0xDC00; // 1101 11 00 0000 0000
                next = (wint_t)l;
                K = h;
                }
#endif
#endif
            return (wint_t)K;
            }
        else
            return k[0];
        }
    else
        return WEOF;
    }

static bool is_littleendian(int b1,int b2)
    {
    return b1 == 0xff && b2 == 0xfe;
    }

static bool is_bigendian(int b1,int b2)
    {
    return b1 == 0xfe && b2 == 0xff;
    }

static bool is_UTF8(FILE * fi)
    {
    int k[6];
    bool ascii = true;
    rewind(fi);
    if(fgetc(fi) == 0xEF && fgetc(fi) == 0xBB && fgetc(fi) == 0xBF) // BOM found
        {
        UTF8start = 3; // used to rewind UTF8 file after BOM
        ascii = false;
        }
    else
        {
        UTF8start = 0;
        rewind(fi);
        }
              
    while((k[0] = getc(fi)) != EOF)
        {
        switch(k[0] & 0xc0) // 11bbbbbb
            {
            case  0xc0:
                {
                ascii = false;
                if((k[0] & 0xfe) == 0xfe)
                    {
                    rewind(fi);
                    return false;
                    }
                // Start of multibyte
                int i = 1;
                while((k[0] << i) & 0x80)
                    {
                    k[i] = getc(fi);
                    if((k[i++] & 0xc0) != 0x80) // 10bbbbbb
                        {
                        rewind(fi);
                        return false;
                        }
                    }
                int K = ((k[0] << i) & 0xff) << (5 * i - 6);
                int I = --i;
                while(i > 0)
                    {
                    K |= (k[i] & 0x3f) << ((I - i) * 6);
                    --i;
                    }
                if(K <= 0x7f)
                    {
                    rewind(fi);
                    return false; // overlong UTF-8 sequence
                    }
                break;
                }
            case 0x80: // 10bbbbbb
                // Not UTF-8
                rewind(fi);
                return false;
            default:
                ;
            }        
        }
    rewind(fi);
    return !ascii;
    }

void checkEncoding(FILE * fi,getcFnc * Getc,ungetcFnc * Ungetc,fseekFnc * Fseek,fputcFnc * Fputc,frewindFnc * Frewind,encodingType encoding)
    {
    assert(fi != stdin);
    int b1,b2;
    
    b1 = getc(fi);
    b2 = getc(fi);
    if(is_littleendian(b1,b2))
        {
        *Getc = littleendian;
        *Ungetc = UngetUNICODE;
        *Fseek = FseekUNICODE;
        *Frewind = FrewindUnicode;
        if(encoding != eISO)
            {
            *Fputc = FputcUnicode;//FputcUnicodeHTMLentity
            }
        endian = LittleEndianFputc;
        }
    else if(is_bigendian(b1,b2))
        {
        *Getc = bigendian;
        *Ungetc = UngetUNICODE;
        *Fseek = FseekUNICODE;
        *Frewind = FrewindUnicode;

        if(encoding != eISO)
            {
            *Fputc = FputcUnicode;//FputcUnicodeHTMLentity
            }
        endian = BigEndianFputc;
        }
    else if(is_UTF8(fi))
        {
        *Getc = UTF8;
        *Ungetc = UngetUTF8;
        *Frewind = FrewindUtf8;
        if(encoding != eISO)
            {
            *Fputc = FputcUTF8; //FputcUTF8HTMLentity
            }
        }
    else
        {
        *Getc = copy;
        *Frewind = rewind;
        }
    forceOutputUnicode(Fputc,encoding);
    /*
    switch(encoding)
        {
        case eUTF8:
            *Fputc = FputcUTF8;
            break;
        case eUTF16:
            *Fputc = FputcUnicode;
            break;
        case eISO:
        default:
            ;
        }
    */
    }

void forceOutputUnicode(fputcFnc * Fputc,encodingType encoding)
    {
    switch(encoding)
        {
        case eUTF8:
            *Fputc = FputcUTF8;
            break;
        case eUTF16:
            *Fputc = FputcUnicode;
            break;
        default:
            ;
        }
    }

#endif
