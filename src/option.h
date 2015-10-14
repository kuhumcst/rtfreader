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
#ifndef OPTION_H
#define OPTION_H

#define TEST 1

#include <wchar.h>

typedef enum {eNoconversion,eISO,eUTF8,eUTF16} encodingType;

struct optionStruct
    {
    bool Emptyline;           // -e              Insert empty line after line not ending with punctuation.
                              // -e-             Do not output empty lines.
    bool emptyline;           // (always true)
    bool tokenize;            // -T              Tokenize output text file.
                              // -T-             Do not tokenize output text file. (same as -P-).
    bool separateBullets;     // -b              Put bullets on separate lines.
                              // -b-             Do not put bullets on separate lines.
    bool A;                   // -A              Convert symbols in range #91-#95 to ASCII symbols (\' \" - *).
                              // -A-             Do not convert symbols in range #91-#95 to ASCII symbols (\' \" - *).
    bool B;                   // -B<char>        Convert bullets to <char>.
    wint_t bullet;            //   <char>
                              // -B              Suppress bullets.
    char * arga;              // -a<abbr>        Text file with abbreviations
    char * argm;              // -m<mwu>         Text file with multi-word-units
    char * argi;              // -i              Input (RTF- or text) file
    char * argt;              // -t<text output> Output text-file.
    int x;                    // -x              Input is plain text.
                              // -x-             Input is RTF.
                              // -x+             Input is RTF or plain text. Let the program find out.

    bool PennTreebankTokenization;
                              // -P              Tokenize output text file according to Penn Treebank rules.
                              // -P-             Do not tokenize output text. (same as -T-).
    bool spaceAtEndOfLine;    // -s              Write extra space before end of line
                              // -s-             Do not write space before end of line.
    bool suppressHTML;        // -H-             Suppress html-tags from output. (Flat text input only).
                              // -H              Keep html-tags (As tokens, if -T or -P is specified.) (Flat text input only).
    bool suppressNoise;       // -n-             Suppress noise caused by suboptimal ocr.
                              // -n              Keep noise.
    bool wordUnwrap;          // -w-             Undo wordwrap by removing end-of-line hyphens and white space.
                              // -w              Keep wordwrapping.
    bool keepEOLsequence;     // -r              Use same EOL-sequence as input, i.e. if input is DOS, output is DOS, etc.
                              //                 Assume DOS (CR LF \r\n) if no EOL-sequence present in input.
                              // -r-             Use newline (\n) as EOL-sequence (default).
    bool ParseAsXml;          // processor instruction ends with ?> Otherwise, they end with > (SGML, HTML)
    bool ParseAsHtml;         // script and style elements take CDATA, but only if parseAsXml is false! (So no XHTML)
                              // If parseAsXml is true, parseAsHtml is irrelevant
/*
Input:
aap <?php >? ?> noot. <script  class=". "> <p class="! "> sætning. </ p > </script>.

parseAsXml == false
parseAsHtml == false

aap <?php > ?
?
> noot .
 <script class=". "> <p class="! "> sætning .
 </ p > </script> .

parseAsXml == true
parseAsHtml == false

aap <?php >? ?> noot .
 <script class=". "> <p class="! "> sætning .
 </ p > </script> .

parseAsXml == false
parseAsHtml == true

aap <?php > ?
?
> noot .
 <script class=". "> <p class= " !
" > sætning .
 </ p > </script> .

parseAsXml == true
parseAsHtml == true

aap <?php >? ?> noot .
 <script class=". "> <p class="! "> sætning .
 </ p > </script> .


*/

    encodingType encoding;    // -E UTF8 -E UTF16 or -E ISO
    optionStruct()
        {
        tokenize = false;
        Emptyline = false;
        emptyline = true;
        separateBullets = false;
        arga = NULL;
        argm = NULL;
        argi = NULL;
//        argr = NULL;
        argt = NULL;
        bullet = 0;
        A = false;
        B = false;
        x = '+';
        //y = false;
        PennTreebankTokenization = false;
        spaceAtEndOfLine = false;
        suppressHTML = false;
        suppressNoise = false;
        wordUnwrap = false;
        keepEOLsequence = false;
        ParseAsXml = false;          // processor instruction ends with ?> Otherwise, they end with > (SGML, HTML)
        ParseAsHtml = false;         // script and style elements take CDATA, but only if parseAsXml is false! (So no XHTML)
        encoding = eNoconversion;
        }
    };

extern optionStruct Option;

#endif
