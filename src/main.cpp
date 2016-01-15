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

#define VERSION "2.1"
#define DATE "2016.01.15"


#include "data.h"

#if FILESTREAM

#include "option.h"
#include "readoptf.h"
#include "argopt.h"
#include "rtfreader.h"
#include "abbrev.h"
#include "unicode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char opts[] = "?@:A:a:b:B:e:E:hH:i:m:n:P:r:s:t:T:vw:x:X:";


static void doSwitch(int c,char * locoptarg,char * progname,optionStruct & Option)
    {
    switch (c)
        {
        case '@':
            readOptsFromFile(locoptarg,progname,Option,doSwitch,opts);
            break;
        case 'A':
                Option.A = !(locoptarg && *locoptarg == '-');
            break;
        case 'a':
            Option.arga = locoptarg;
            break;
        case 'm':
            Option.argm = locoptarg;
            break;
        case 'i':
            Option.argi = locoptarg;
            break;
        case 't':
            Option.argt= locoptarg;
            break;
        case 'H':
            if(!strcmp(locoptarg,"TML"))
                Option.ParseAsHtml = true;
            else
                Option.suppressHTML = locoptarg && *locoptarg == '-';
            break;
        case 'X':
            if(!strcmp(locoptarg,"ML"))
                Option.ParseAsXml = true;
            break;
        case 'n':
            Option.suppressNoise = locoptarg && *locoptarg == '-';
            break;
        case 'w':
            Option.wordUnwrap = locoptarg && *locoptarg == '-';
            break;
        case 'h':
        case '?':
            printf("    Convert RTF-file to flat text.\n");
            printf("usage:\n");
            printf("%s [-@<option file>] -i<RTF-input> -t<text output> [other options]\n",progname);
            printf("    -@<option file> Read options from file.\n");
            printf("    -i<RTF-input>   Input RTF-file\n");
            printf("    -t<text output> Output text-file. One sentence per line. List items and\n"
                   "                    headers are treated as sentences and put on lines for\n"
                   "                    themselves. Bullets are on separate lines.\n");
            printf("    -A              Convert symbols in range #91-#95 to ASCII symbols (\' \" - *).\n");
            printf("    -A-             Do not convert symbols in range #91-#95 to ASCII symbols (\' \" - *). (default)\n");
            printf("    -a<abbr>        Text file with abbreviations\n");
            printf("    -B<char>        Convert bullets to <char>.\n");
            printf("    -B              Suppress bullets.\n");
            printf("    -b              Put bullets on separate lines.\n");
            printf("    -b-             Do not put bullets on separate lines. (default)\n");
            printf("    -e              Insert empty line after line not ending with punctuation.\n");
            printf("    -e-             Do not output empty lines. (default)\n");
            printf("    -H              Keep html-tags (As tokens, if -T or -P is specified.) (Flat text input only). (default)\n");
            printf("    -H-             Suppress html-tags from output. (Flat text input only).\n");
            printf("    -HTML           Parse as HTML: <style> and <script> take CDATA, but only if not -XML (default off).\n");
            printf("    -h              Usage.\n");
            printf("    -m<mwu>         Text file with multi-word-units\n");
            printf("    -n              Keep noise. (default)\n");
            printf("    -n-             Suppress noise caused by suboptimal ocr.\n");
            printf("    -P              Tokenize output text file according to Penn Treebank rules.\n");
            printf("    -P-             Do not tokenize output text. (same as -T-). (default)\n");
            printf("    -r              Use same EOL-sequence as input, i.e. if input is DOS, output is DOS, etc.\n");
            printf("    -r-             Use newline (\\n) as EOL-sequence (default).\n");
            printf("    -s              Write extra space before end of line\n");
            printf("    -s-             Do not write space before end of line. (default)\n");
            printf("    -T              Tokenize output text file.\n");
            printf("    -T-             Do not tokenize output text file. (same as -P-). (default)\n");
            printf("    -v              Print program version and exit\n");
            printf("    -w              Keep wordwrapping. (default)\n");
            printf("    -w-             Undo wordwrap by removing end-of-line hyphens and white space.\n");
            printf("    -x              Input is plain text.\n");
            printf("    -x-             Input is RTF.\n");
            printf("    -x+             Input is RTF or plain text. Let the program find out. (default)\n");
            printf("    -XML            Parse as XML: processor instructions end with ?> (default off).\n");
            printf("    -E<encoding>    Force output encoding: UTF8, ISO or UTF16 (default: same encoding as input if text, UTF8 if RTF)\n");
            exit(0);
            break;
/*
-P -b -y -e- -B -A  
-T -b -y -e -s 
*/
        case 'e':
            Option.Emptyline = !(locoptarg && *locoptarg == '-');
            break;
        case 'E':
            if(locoptarg)
                {
                if(!strcmp(locoptarg,"ISO")
                    ||!strcmp(locoptarg,"1"))
                    Option.encoding = eISO;
                else if(!strcmp(locoptarg,"UTF8") 
                    || !strcmp(locoptarg,"utf8")
                    || !strcmp(locoptarg,"UTF-8")
                    || !strcmp(locoptarg,"utf-8")
                    || !strcmp(locoptarg,"2"))
                    Option.encoding = eUTF8;
                else if(!strcmp(locoptarg,"UTF16") 
                    || !strcmp(locoptarg,"utf16") 
                    || !strcmp(locoptarg,"UTF-16") 
                    || !strcmp(locoptarg,"utf-16") 
                    || !strcmp(locoptarg,"UNICODE") 
                    || !strcmp(locoptarg,"unicode")
                    || !strcmp(locoptarg,"3"))
                    Option.encoding = eUTF16;
                else if(!strcmp(locoptarg,"-") 
                    || !strcmp(locoptarg,"0"))
                    Option.encoding = eNoconversion;
                }
            break;
        case 'b':
            Option.separateBullets = !(locoptarg && *locoptarg == '-');
            break;
        case 'B':
            Option.B = true;
            if(locoptarg)
                {
                Option.bullet = (wint_t)(unsigned char)*locoptarg;
                }
            else
                {
                Option.bullet = false;
                }
            break;
        case 'x':
            if(locoptarg)
                {
                if(*locoptarg == '-')
                    {
                    Option.x = '-';
                    }
                else
                    {
                    Option.x = '+'; // default, let program decide
                    }
                }
            else
                {
                Option.x = 0;
                }
            break;
        case 'T':
            Option.tokenize = !(locoptarg && *locoptarg == '-');
            break;
        case 'P':
            Option.PennTreebankTokenization = Option.tokenize = !(locoptarg && *locoptarg == '-');
            break;
        case 's':
            Option.spaceAtEndOfLine = !(locoptarg && *locoptarg == '-');;
            break;
        case 'r':
            Option.keepEOLsequence = !(locoptarg && *locoptarg == '-');
            break;
        case 'v':
            printf("Version: %s (%s)\n",VERSION,DATE);
            exit(0);
        }
    }


static bool readArgs(int argc, char * argv[],optionStruct & Option)
    {
    int c;
    while((c = getopt(argc,argv, opts)) != -1)
        {
        doSwitch(c,optarg,argv[0],Option);
        }
    return true;
    }


#if 0
    void createCharLists()
        {
        FILE * fa;
#if 0
        fa = fopen("alpha.txt","w");
        if(fa)
            {
            int i;
            for(i = 32;i < 256;++i)
                {
                fprintf(fa,"%c",i);
                }
            fclose(fa);
            }

#endif
#if 1
        fa = fopen("alpha.txt","w");
        if(fa)
            {
            char vowel[] = "AEIOUYaeiouyåúü¿¡¬√ƒ≈∆»… ÀÃÕŒœ“”‘’÷ÿŸ⁄€‹›‡·‚„‰ÂÊËÈÍÎÏÌÓÔÚÛÙıˆ¯˘˙˚¸˝ˇ";
            int i;
            for(i = 0;i < 256;)
                {
                if(i && strchr(vowel,i))
                    {
                    fprintf(fa,"true ",i);
                    fprintf(fa,"/* %c */, ",i);
                    }
                else
                    {
                    fprintf(fa,"false",i);
                    fprintf(fa,"       , ");
                    }
                ++i;
                if((i % 16) == 0)
                    fprintf(fa,"/*  %.2d -  %2.d */ /* %0.2x - %02x */\n",i - 16,i,i-16,i);
                }
            fclose(fa);
            }
        fa = fopen("punct.txt","w");
        if(fa)
            {
            char punct[] = "!\"'(),-.:;?";
            int i;
            for(i = 0;i < 256;)
                {
                if(i && strchr(punct,i))
                    {
                    fprintf(fa,"true ",i);
                    fprintf(fa,"/* %c */, ",i);
                    }
                else
                    {
                    fprintf(fa,"false",i);
                    fprintf(fa,"       , ");
                    }
                ++i;
                if((i % 16) == 0)
                    fprintf(fa,"/*  %.2d -  %2.d */ /* %0.2x - %02x */\n",i - 16,i,i-16,i);
                }
            fclose(fa);
            }
        fa = fopen("funny.txt","w");
        if(fa)
            {
            char funny[] = "#$%&*+/<=>@[\\]_{|}";
            int i;
            for(i = 0;i < 256;)
                {
                if(i && strchr(funny,i))
                    {
                    fprintf(fa,"true ",i);
                    if((i & 0x7F) >= 32)
                        fprintf(fa,"/* %c */, ",i);
                    else
                        fprintf(fa,"/* %0.2x*/, ",i);
                    }
                else
                    {
                    fprintf(fa,"false",i);
                    fprintf(fa,"       , ");
                    }
                ++i;
                if((i % 16) == 0)
                    fprintf(fa,"/*  %.2d -  %2.d */ /* %0.2x - %02x */\n",i - 16,i,i-16,i);
                }
            fclose(fa);
            }
        fa = fopen("veryfunny.txt","w");
        if(fa)
            {
            char funny[] = "^`~ÄÅÇÉÑÖÜáàâäãåçéèêëíìîïñóòôöõúùûü†°¢£§•¶ß®©™´¨≠ÆØ∞±≤≥¥µ∂∑∏π∫ªºΩæø◊˜";
            int i;
            for(i = 0;i < 256;)
                {
                if(i && strchr(funny,i))
                    {
                    fprintf(fa,"true ",i);
                    if((i & 0x7F) >= 32)
                        fprintf(fa,"/* %c */, ",i);
                    else
                        fprintf(fa,"/* %0.2x*/, ",i);
                    }
                else
                    {
                    fprintf(fa,"false",i);
                    fprintf(fa,"       , ");
                    }
                ++i;
                if((i % 16) == 0)
                    fprintf(fa,"/*  %.2d -  %2.d */ /* %0.2x - %02x */\n",i - 16,i,i-16,i);
                }
            fclose(fa);
            }
#endif
        }
#endif


int main(int argc,char ** argv)
    {
/*
    createCharLists();
    int x = 128;
    FILE * fchars = fopen("chars.txt","w");
    for(x = 32;x < 256;++x)
        {
        fprintf(fchars,"{\\f2 0x%02X: }{\\'%02x \\par}",x,x);
        }
    fclose(fchars);




*/
    readArgs(argc, argv,Option);
    
    if(Option.arga)
        {
        FILE * a = fopen(Option.arga,"rb");
        if(a)
            {
            readAbbreviations(a);
            fclose(a);
            }
        else
            {
            fprintf(stderr,"cstrtfreader: Error in opening abbreviations file %s\n",Option.arga);
            return 1;
            }
        }
    

    STROEM * sourceFile = NULL;
    
    if(Option.argi)
        {
        sourceFile = fopen(Option.argi,"rb");
        if(!sourceFile)
            {
            fprintf(stderr,"cstrtfreader: Error in opening input file %s\n",Option.argi);
            return 1;
            }
        checkEncoding(sourceFile,&Getc,&Ungetc,&Fseek,&Fputc,&Frewind,Option.encoding); // Find out what kind
            // of encoding has been used and set function pointers accordingly.
        }
    else
        {
        fprintf(stderr,"cstrtfreader: No input file specified. See %s -h\n",argv[0]);
        return 1;
        }
    
    if(Option.argt)
        {
        return doit(sourceFile, Option.argt) ? 0 : 1;
        }
    else
        {
        char extension[]=".segments";
        char * oname = new char[strlen(Option.argi)+sizeof(extension)];
        sprintf(oname,"%s%s",Option.argi,extension);
        int ret = doit(sourceFile, oname) ? 0 : 1;
        delete oname;
        return ret;
        }
    }
#endif
