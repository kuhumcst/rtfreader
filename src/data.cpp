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
