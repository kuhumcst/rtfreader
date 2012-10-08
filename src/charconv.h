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
#ifndef CHARCONV_H
#define CHARCONV_H

#include "data.h"

#include "letterfunc.h"

#define LATIN_1

#ifdef LATIN_1 /* ISO8859 *//* NOT DOS compatible! */
extern const wint_t AsciiEquivalent[];
#if UNICODE_CAPABLE
#else
extern const wchar_t lowerEquivalent[];
extern const wchar_t upperEquivalent[];
#endif
#endif
#if UNICODE_CAPABLE
#else
bool isUpper(int kar);
bool isLower(int kar);
extern const bool alpha[256];

/*TODO isAlpha must be made Unicode-capable*/

#define isAlpha(s) alpha[(int)(s) & 0xFF]
int strcasecmp(const /*unsigned char*/ wchar_t *s, const char *p);
#endif

#if FILESTREAM
unsigned char ISO8859toCodePage850(unsigned char kar);
unsigned char CodePage850toISO8859(unsigned char kar);
#endif
#define isDigit(s) ('0' <= (s) && (s) <= '9')

#endif

