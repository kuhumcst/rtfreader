/*
RTFREADER - reads document in RTF format and writes same document as text, one sentence per line and tokenised (optional).

segment                                                          Recursively traverse RTF segment tree. Keep track of character properties (bold, italic, small caps, font size)
   |                                                             Returns true if segment is a field. False otherwise.
   |                                                             Sets begin and end offsets for candidate segments, based on the interpretation of RTF tags such as \par
   |
   +--segmentdefault                                             Tries to spot sentence endings by looking at interpunction. Knows of filenames, ellipsis ...
           |
           +--doTheSegmentation
                   | |
                   | +--GetPutBullet                             Skip rest of segment. Print bullet character and a blank.
                   |         |
                   +--GetPut |                                   Read segment. Convert rtf-tokens to their character equivalents. Suppress optional hyphens.
                   |     |   |
                   +-----+---+--Put                              Handle newlines
                                 |
                                 +--Put2                         Handle quotes, brackets and abbreviations
                                      |
                                      +--PutN                    Repeated call of Put3 on whole buffer
                                      |    |
                                      +----+--Put3               Start new sentence if an abbreviation is followed by a capital letter in a new segment
                                                |
                                                +--Put4          Optionally translate 8-bit set characters to combinations of 7-bit characters
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "argopt.h"
#include "abbrev.h"
#include "mwu.h"
#include "charconv.h"
#include "readoptf.h"
//#include "log.h" // LOGRTF.TXT
//LOGCODE
/*
-a fork -m mwu -i .... -t sents.txt -e- -T -B -b -A

*/

#define INDENT 2
#define TEXTAFTERCLOSINGBRACKET 1
#define COLONISSENTENCEDELIMITER 0


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

#define isSpace(s) space[(int)(s) & 0xFF]




//#define LOGGING
#ifdef LOGGING
static int tentative = 0;
static FILE * fplog = NULL;

static void indent(int level)
    {
    while(level-- > 0)
        fputc(' ',fplog);
    }

static void log2(int level,int ch,bool branch)
    {
    static bool newline = true;
    static bool indented = false;
#if !INDENT
    if(ch)
        fprintf(fplog,"%c",ch);
#else
    if(branch)
        {
        if(ch == '{')
            {
            if(newline)
                {
                if(!indented)
                    indent(level);
                fprintf(fplog,"{");
                }
            else
                {
                fputc('\n',fplog);
                indent(level);
                fputc('{',fplog);
//                fprintf(fplog,"\n%*c{",level+1,' ');
                }
            indented = true;
            newline = false;
            }
        else
            {
            if(newline)
                {
                if(!indented)
                    indent(level);
//                fprintf(fplog,"}\n%*c",level,' ');
                fprintf(fplog,"}\n");
//                indent(level);
//                indented = true;
                }
            else
                {
//                fprintf(fplog,"\n%*c}\n%*c",level+1,' ',level,' ');
                fputc('\n',fplog);
                indent(level);
                fprintf(fplog,"}\n");
//                indent(level);
                }
            indented = false;
            newline = true;
            }
        }
    else if(ch == '\n')
        {
        if(!newline)
            {
//            fprintf(fplog,"\n%*c",level,' ');
            fprintf(fplog,"\n");
//            indent(level);
            indented = false;
            newline = true;
            }
        }
/*    else if(ch == '\r')
        fprintf(fplog,"\r%*c",level,' ');*/
    else if(ch)
        {
        if(newline)
            {
            if(!isSpace(ch))
                {
                if(!indented)
                    {
                    indent(level);
                    indented = true;
                    }
                newline = false;
                fprintf(fplog,"%c",ch);
                }
            }
        else
            {
            fprintf(fplog,"%c",ch);
            }
        }
#endif
    }


static void log(int level,int ch,bool branch)
    {
    if(!fplog)
        fplog = fopen("rtflog.txt","wb");
    if(tentative)
        {
        log2(level,tentative,branch);
        tentative = 0;
        }
    log2(level,ch,branch);
    }

static void logn(const char * s)
    {
    if(s)
        while(*s)
            {
            log(0,*s++,false);
            }
    }
#endif

enum {FALSE,TRUE};
#define skip_tekst 1
#define new_segment 2
#define character_property 4
#define hidden 8
#define reset 16
#define field 32
#define MAXSEGMENTLENGTH 10000

static FILE * sourceFile,* targetFile,* outputtext;
enum eStatus { uninitialised = -3, notSpecified = -2, mixed = -1, Off = 0, On = 1 };

enum PersonName {not_a_name,initial/*,family_name*/};

struct flags
    {
//    bool newSegment:1;
    bool in_abbreviation:1;
    bool htmltagcoming:1;
    bool firstofhtmltag:1;
    bool notstartofline:1;
    bool writtentoline:1;
    bool inhtmltag:1;
    bool firstafterhtmltag:1;
    bool bbullet;
    PersonName person_name:2;
    int number_final_dot:2;
    };

static bool segmentText(flags & flgs);

typedef struct CharToken
  {
  char * token;
  int ISOcode;
  } CharToken;

/* tokens that demarcate segments (can not be part of a segment): */
static const CharToken tokensThatDemarcateSegments[] =  /* strcmp */
  {{"tab",'\t'}     // Special characters, Symbol
  ,{"cell",-1}      // Special characters, Symbol
  ,{"par",-1}       // Special characters, Symbol
  ,{"line",-1}      // Special characters, Symbol
  ,{"intbl",-1}//??? Paragraph Formatting Properties
  ,{"footnote",-1}  // Footnotes, Destination
//  ,{"box",-1}//???
  ,{"'02",2} // (seen in a footnote)
  ,{NULL,0}
  };

static const char * tokensThatTakeArgument[] = /* strncmp */
  {"fonttbl"        // Font Table, Destination
  ,"stylesheet"     // Style Sheet, Destination
  ,"pntext"         // Bullets and numbering,  Destination
  ,"pict"           // Pictures, Destination
  ,"object"         // Objects, Destination
  ,"template"       // Document Formatting Properties, Destination
  ,"author"         // Information Group, Destination
  ,"bkmkstart"      // Bookmarks, Destination
  ,"bkmkend"        // Bookmarks, Destination
  ,"title"          // Information Group, Destination
  ,"fldinst"        // Fields, Destination
  ,"operator"       // Information Group, Destination
  ,"xe"             // Index Entries, Destination
//  ,"field"          // Fields, Destination
  ,"fldinst"          // Fields, Destination
  ,"viewkind"   // Document Format Properties, Value
  ,"viewscale"  // Document Format Properties, Value
  ,"viewzk"     // Document Format Properties, Value
  ,"company"        // Information Group, Destination
  ,"pntxta"         // Bullets and numbering, Destination
  ,"pntxtb"         // Bullets and numbering, Destination
  ,"revtbl"         // Track changes, Destination
  ,"background"     // Word 97 through Word 2002 RTF for Drawing Objects (Shapes), Destination
  ,"userprops"      // Information Group, Destination
  ,"listtable"
  ,"listoverridetable"
  ,"shp"
  ,"generator"
  ,"password"
  ,"xmlnstbl"
  ,"wgrffmtfilter"
  ,NULL
  };

static const char * tokensThatAreField[] = /* strncmp */
  {"field"          // Fields, Destination
  ,NULL
  };

static const char * tokensThatToggle[] = /* strncmp */
  {"v" /* hidden text */              // Font (Character) Formatting Properties Toggle
  ,NULL
  };

static const char * tokensThatReset[] = /* strncmp */
  {"pard"           
  ,NULL
  };

/* Tokens that can be part of a segment: */

static const CharToken tokensThatArePartOfTheText[] = /* strcmp */
  {{"lquote",0x91}
  ,{"rquote",0x92}
  ,{"ldblquote",0x93}
  ,{"rdblquote",0x94}
  ,{"bullet",0x95}
  ,{"endash",0x96}
  ,{"emdash",0x97}
  ,{"'95",149}
  ,{"~",' '} // Bart 20060303 \~ is Nonbreaking space
  ,{NULL,0}
  };



typedef struct CharProp
  {
  char * prop;
  eStatus status;
  bool onn;
  } CharProp;

static CharProp characterProperty[] = /* strcmp */
    {{"i",notSpecified,true}
    ,{"i0",notSpecified,false}
    ,{"b",notSpecified,true}  // bold
    ,{"b0",notSpecified,false} // bold off
    ,{"scaps",notSpecified,true}
    ,{"scaps0",notSpecified,false}
    ,{NULL,notSpecified,false}
    };

static const int Uninitialised = -3;
static const int NotSpecified = -2;
static const int Mixed = -1;

typedef struct CharPropN
  {
  char * prop;
  int N;
  int def;
  } CharPropN;

static CharPropN characterPropertyWithNumericalParameter[] = /* strncmp + number*/
    {
         {"fs",NotSpecified,24} // Font size in half-points (default is 24)
        ,{"f",Mixed,0/*?*/} // Font number
        ,{NULL,0,0}
    };

union startLine
    {
    struct
        {
        unsigned int SD:1; // set at start of text and upon seeing segment delimiter (bullet, full stop, colon, ...)
        unsigned int LF:1;
        unsigned int CR:1;
        unsigned int WS:1;
        unsigned int LS:1; // Leading space (Bart 20061214)
        } b;
    unsigned int EOL:3; // SD, LF and CR together
    unsigned int i;     // all five together
    };

class html_tag_class;

typedef enum {notag,tag,endoftag,endoftag_startoftag} estate;
//typedef estate (html_tag_class::*t_state)(int kar);
estate (html_tag_class::*tagState)(int kar);

class html_tag_class
    {
    private:
    public:
        estate def(int kar);
        estate lt(int kar);
        estate element(int kar);
        estate elementonly(int kar);
        estate gt(int kar);
        estate emptytag(int kar);
        estate atts(int kar);
        estate name(int kar);
        estate value(int kar);
        estate invalue(int kar);
        estate singlequotes(int kar);
        estate doublequotes(int kar);
        estate endvalue(int kar);
        estate markup(int kar);
        estate h1(int kar);
        estate h2(int kar);
        estate comment(int kar);
        estate h3(int kar);
        estate h4(int kar);
        estate endtag(int kar);
        html_tag_class()
            {
            tagState = &html_tag_class::def;
            }
    };


    estate html_tag_class::def(int kar)
        {
        switch(kar)
            {
            case '<':
                tagState = &html_tag_class::lt;
                return tag;
            default:
                return notag;
            }
        }
    estate html_tag_class::lt(int kar)
        {
        switch(kar)
            {
            case '>':
                tagState = &html_tag_class::def;
                return notag;
            case '!':
                tagState = &html_tag_class::markup;
                return tag;
            case '/':
                tagState = &html_tag_class::endtag;
            case 0xA0:
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                return tag;
            default:
                if('A' <= kar && kar <= 'Z' || 'a' <= kar && kar <= 'z')
                    {
                    tagState = &html_tag_class::element;
                    return tag;
                    }
                else
                    {
                    tagState = &html_tag_class::def;
                    return notag;
                    }
            }
        }

    estate html_tag_class::element(int kar)
        {
        switch(kar)
            {
            case '<':
                tagState = &html_tag_class::lt;
                return endoftag_startoftag;
            case '>':
                tagState = &html_tag_class::def;
                return endoftag;
            case '-':
            case '_':
            case ':':
            case '.':
                return tag;
            case 0xA0:
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                tagState = &html_tag_class::atts;
                return tag;
            case '/':
                tagState = &html_tag_class::emptytag;
                return tag;
            default:
                if('0' <= kar && kar <= '9' || 'A' <= kar && kar <= 'Z' || 'a' <= kar && kar <= 'z')
                    return tag;
                else
                    {
                    tagState = &html_tag_class::def;
                    return notag;
                    }
            }
        }

    estate html_tag_class::elementonly(int kar)
        {
        switch(kar)
            {
            case '>':
                tagState = &html_tag_class::def;
                return endoftag;
            case '-':
            case '_':
            case ':':
            case '.':
                return tag;
            case 0xA0:
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                tagState = &html_tag_class::gt;
                return tag;
            default:
                if('0' <= kar && kar <= '9' || 'A' <= kar && kar <= 'Z' || 'a' <= kar && kar <= 'z')
                    return tag;
                else
                    {
                    tagState = &html_tag_class::def;
                    return notag;
                    }
            }
        }

    estate html_tag_class::gt(int kar)
        {
        switch(kar)
            {
            case '>':
                tagState = &html_tag_class::def;
                return endoftag;
            case 0xA0:
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                return tag;
            default:
                tagState = &html_tag_class::def;
                return notag;
            }
        }

    estate html_tag_class::emptytag(int kar)
        {
        switch(kar)
            {
            case '>':
                tagState = &html_tag_class::def;
                return endoftag;
            case 0xA0:
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                return tag;
            default:
                tagState = &html_tag_class::def;
                return notag;
            }
        }

    estate html_tag_class::atts(int kar)
        {
        switch(kar)
            {
            case '>':
                tagState = &html_tag_class::def;
                return endoftag;
            case 0xA0:
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                return tag;
            case '/':
                tagState = &html_tag_class::emptytag;
                return tag;
            default:
                if('A' <= kar && kar <= 'Z' || 'a' <= kar && kar <= 'z')
                    {
                    tagState = &html_tag_class::name;
                    return tag;
                    }
                else
                    {
                    tagState = &html_tag_class::def;
                    return notag;
                    }
            }
        }

    estate html_tag_class::name(int kar)
        {
        switch(kar)
            {
            case '>':
                tagState = &html_tag_class::def;
                return endoftag;
            case '-':
            case '_':
            case ':':
            case '.':
                return tag;
            case 0xA0:
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                tagState = &html_tag_class::atts;
                return tag;
            case '/':
                tagState = &html_tag_class::emptytag;
                return tag;
            case '=':
                tagState = &html_tag_class::value;
                return tag;
            default:
                if('0' <= kar && kar <= '9' || 'A' <= kar && kar <= 'Z' || 'a' <= kar && kar <= 'z')
                    return tag;
                else
                    {
                    tagState = &html_tag_class::def;
                    return notag;
                    }
            }
        }

    estate html_tag_class::value(int kar)
        {
        switch(kar)
            {
            case '>':
            case '/':
            case '=':
                tagState = &html_tag_class::def;
                return notag;
            case 0xA0:
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                return tag;
            case '\'':
                tagState = &html_tag_class::singlequotes;
                return tag;
            case '"':
                tagState = &html_tag_class::doublequotes;
                return tag;
            case '-':
            case '_':
            case ':':
            case '.':
                tagState = &html_tag_class::invalue; 
            default:
                if('0' <= kar && kar <= '9' || 'A' <= kar && kar <= 'Z' || 'a' <= kar && kar <= 'z')
                    {
                    tagState = &html_tag_class::invalue;
                    return tag;
                    }
                else
                    {
                    tagState = &html_tag_class::def;
                    return notag;
                    }
            }
        }

    estate html_tag_class::invalue(int kar)
        {
        switch(kar)
            {
            case '>':
                tagState = &html_tag_class::def;
                return endoftag;
            case 0xA0:
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                tagState = &html_tag_class::atts;
                return tag;
            default:
                if('0' <= kar && kar <= '9' || 'A' <= kar && kar <= 'Z' || 'a' <= kar && kar <= 'z')
                    {
                    return tag;
                    }
                else
                    {
                    tagState = &html_tag_class::def;
                    return notag;
                    }
            }
        }

    estate html_tag_class::singlequotes(int kar)
        {
        switch(kar)
            {
            case '\'':
                tagState = &html_tag_class::endvalue;
                return tag;
            default:
                return tag;
            }
        }

    estate html_tag_class::doublequotes(int kar)
        {
        switch(kar)
            {
            case '\"':
                tagState = &html_tag_class::endvalue;
                return tag;
            default:
                return tag;
            }
        }

    estate html_tag_class::endvalue(int kar)
        {
        switch(kar)
            {
            case '>':
                tagState = &html_tag_class::def;
                return endoftag;
            case 0xA0:
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                tagState = &html_tag_class::atts;
                return tag;
            default:
                tagState = &html_tag_class::def;
                return notag;
            }
        }

    estate html_tag_class::markup(int kar)
        {
        switch(kar)
            {
            case '>':
                tagState = &html_tag_class::def;
                return endoftag;
            case '-':
                tagState = &html_tag_class::h1;
                return tag;
            default:
                return tag;
            }
        }
    estate html_tag_class::h1(int kar)
        {
        switch(kar)
            {
            case '>':
                tagState = &html_tag_class::def;
                return endoftag;
            case '-':
                tagState = &html_tag_class::h2;
                return tag;
            default:
                tagState = &html_tag_class::markup;
                return tag;
            }
        }

    estate html_tag_class::h2(int kar)
        {
        switch(kar)
            {
            case '>':
                tagState = &html_tag_class::def;
                return endoftag;
            case '-':
                tagState = &html_tag_class::h3;
                return tag;
            default:
                tagState = &html_tag_class::comment;
                return tag;
            }
        }

    estate html_tag_class::comment(int kar)
        {
        switch(kar)
            {
            case '-':
                tagState = &html_tag_class::h3;
                return tag;
            default:
                return tag;
            }
        }
    estate html_tag_class::h3(int kar)
        {
        switch(kar)
            {
            case '>':
                tagState = &html_tag_class::def;
                return notag;
            case '-':
                tagState = &html_tag_class::h4;
                return tag;
            default:
                tagState = &html_tag_class::comment;
                return tag;
            }
        }

    estate html_tag_class::h4(int kar)
        {
        switch(kar)
            {
            case '>':
                tagState = &html_tag_class::def;
                return endoftag;
            case '-':
                tagState = &html_tag_class::h1;
                return tag;
            default:
                tagState = &html_tag_class::markup;
                return tag;
            }
        }

    estate html_tag_class::endtag(int kar)
        {
        switch(kar)
            {
            case '>':
                tagState = &html_tag_class::def;
                return endoftag;
            case 0xA0:
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                return tag;
            default:
                if('A' <= kar && kar <= 'Z' || 'a' <= kar && kar <= 'z')
                    {
                    tagState = &html_tag_class::elementonly;
                    return tag;
                    }
                else
                    {
                    tagState = &html_tag_class::def;
                    return notag;
                    }
            }
        }


struct optionStruct
    {
    bool emptyline;
    bool tokenize;
    bool separateBullets;
    bool A; // A option present
    bool B; // B option present
    char * arga;
    char * argm;
    char * argi;
    char * argr;
    char * argt;
    int bullet;
    int x;
    //bool y;
    bool PennTreebankTokenization;
    bool spaceAtEndOfLine; // Bart 20050311: this is how Dorte's tokenizer works.
    bool suppressHTML;
    optionStruct()
        {
        tokenize = false;
        emptyline = false;
        separateBullets = false;
        arga = NULL;
        argm = NULL;
        argi = NULL;
        argr = NULL;
        argt = NULL;
        bullet = 0;
        A = false;
        B = false;
        x = '+';
        //y = false;
        PennTreebankTokenization = false;
        spaceAtEndOfLine = false;
        suppressHTML = false;
        }
    };

static optionStruct Option;
//static const char opts[] = "?@:A:a:b:B:e:hH:i:m:P:r:s:t:T:x:y:";
static const char opts[] = "?@:A:a:b:B:e:hH:i:m:P:r:s:t:T:x:";

static void doSwitch(int c,char * locoptarg,char * progname,optionStruct & Option)
    {
    switch (c)
        {
        case '@':
            readOptsFromFile(locoptarg,progname,Option,doSwitch,opts);
            break;
        case 'A':
            if(locoptarg && *locoptarg == '-')
                {
                Option.A = false;
                }
            else
                {
                Option.A = true;
                }
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
        case 'r':
            Option.argr= locoptarg;
            break;
        case 't':
            Option.argt= locoptarg;
            break;
        case 'H':
            if(locoptarg && *locoptarg == '-')
                {
                Option.suppressHTML = true;
                }
            else
                {
                Option.suppressHTML = false;
                }
            break;
        case 'h':
        case '?':
            printf("usage:\n");
            printf("============================\n");
            printf("    Convert RTF-file to flat text.\n");
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
            printf("    -h              Usage.\n");
            printf("    -m<mwu>         Text file with multi-word-units\n");
            printf("    -P              Tokenize output text file according to Penn Treebank rules.\n");
            printf("    -P-             Do not tokenize output text. (same as -T-). (default)\n");
            printf("    -r<RTF-output>  Output RTF-file. Empty lines are inserted between headers\n"
                   "                    and body text. Bullets are placed on separate lines.\n"
                   "                    This output can be converted to text by MS-Word.\n");
            printf("    -s              Write extra space before end of line\n");
            printf("    -s-             Do not write space before end of line. (default)\n");
            printf("    -T              Tokenize output text file.\n");
            printf("    -T-             Do not tokenize output text file. (same as -P-). (default)\n");
            printf("    -x              Input is plain text.\n");
            printf("    -x-             Input is RTF.\n");
            printf("    -x+             Input is RTF or plain text. Let the program find out. (default)\n");
//            printf("    -y              Input is RTF or plain text. Let the program find out. (default)\n");
//            printf("    -y-             Input depends on option -x.\n");
            exit(0);
            break;
/*
-P -b -y -e- -B -A  
-T -b -y -e -s 
*/
        case 'e':
            if(locoptarg && *locoptarg == '-')
                {
                Option.emptyline = false;
                }
            else
                {
                Option.emptyline = true;
                }
            break;
        case 'b':
            if(locoptarg && *locoptarg == '-')
                {
                Option.separateBullets = false;
                }
            else
                {
                Option.separateBullets = true;
                }
            break;
        case 'B':
            Option.B = true;
            if(locoptarg)
                {
                Option.bullet = (int)(unsigned char)*locoptarg;
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
            /*
        case 'y':
            if(locoptarg && *locoptarg == '-')
                {
                Option.y = false;
                }
            else
                {
                Option.y = true;
                }
            break;
            */
        case 'T':
            if(locoptarg && *locoptarg == '-')
                {
                Option.tokenize = false;
//                Option.PennTreebankTokenization = false;
                }
            else
                {
                Option.tokenize = true;
                }
            break;
        case 'P':
            if(locoptarg && *locoptarg == '-')
                {
                Option.tokenize = false;
                Option.PennTreebankTokenization = false;
                }
            else
                {
                Option.PennTreebankTokenization = true;
                Option.tokenize = true;
                }
            break;
        case 's':
            if(locoptarg && *locoptarg == '-')
                {
                Option.spaceAtEndOfLine = false;
                }
            else
                {
                Option.spaceAtEndOfLine = true;
                }
            break;
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


static char * parseRTFtoken(int level/*,int stat*/)
    {
    int ch;
    bool first = true;
    int index = 1;
    static char token[100];
    token[0] = '\\';
    while((ch = getc(sourceFile)) != EOF && ch != 26)
        {
#ifdef LOGGING
        log(level,0,false);
        tentative = ch;
#endif
        token[index++] = ch;
        token[index] = 0;
        switch(ch)
            {
            case '-':
            case '*':
            case ':':
            case '_':
            case '|':
            case '~':
                if(first) // detected symbol
                    {
#ifdef LOGGING
                    log(level,0,false);
#endif
                    return token;
                    }
                else if(ch != '-')   // detected end of token
                    {
#ifdef LOGGING
                    tentative = 0;
#endif
                    ungetc(ch,sourceFile);
                    token[--index] = 0;
                    return token;
                    }
                break;
            case '{':
            case '}':
                if(first) // detected '{' or '}'
                    {
#ifdef LOGGING
                    log(level,0,false);
#endif
                    return token;
                    }
                else    // detected syntactical brace
                    {
#ifdef LOGGING
                    tentative = 0;
#endif
                    ungetc(ch,sourceFile);
                    token[--index] = 0;
                    return token;
                    }
            case '\\':  // detected '\'
                if(first)
                    {
#ifdef LOGGING
                    log(level,0,false);
#endif
                    return token;
                    }
                else    // detected start of new tag
                    {
#ifdef LOGGING
                    tentative = 0;
#endif
                    ungetc(ch,sourceFile);
                    token[--index] = 0;
                    return token;
                    }
            case '\'':
                {
                if(first)
                    {
#ifdef LOGGING
                    int ch;// TODO remove when done with logging
#endif
                    unsigned char kar;
                    const CharToken * chartoken;
                    token[index++] =
#ifdef LOGGING
                    ch =
#endif
                    getc(sourceFile);
#ifdef LOGGING
                    log(level,ch,false);
#endif
                    token[index++] = ch = getc(sourceFile);
#ifdef LOGGING
                    log(level,ch,false);
#endif
                    token[index] = 0;
                    kar = (unsigned char)(int)strtol(token+2,NULL,16);
                    for(chartoken = tokensThatDemarcateSegments
                        ;chartoken->token
                        ;++chartoken
                        )
                        {
                        if(kar == chartoken->ISOcode)
                            {
                            return token;
                            }
                        }
                    for(chartoken = tokensThatArePartOfTheText
                        ;chartoken->token
                        ;++chartoken
                        )
                        {
                        if(kar == chartoken->ISOcode)
                            {
                            return token;
                            }
                        }
                    token[0] = kar;
                    token[1] = '\0';
                    return token;
                    }
                }
                // drop through
            case 0xA0:
            case ' ':
#ifdef LOGGING
                log(level,0,false);
#endif
                return token;
            default :
                ;
            }
        first = false;
        }
    if(ch == EOF && level != 0)
        printf("\n%d unbalanced braces\n",level);
#ifdef LOGGING
    tentative = 0;
#endif
    return NULL;
    }

static int level;

static void resetCharProp(const startLine firsttext)
    {
    for(CharProp * P = characterProperty
        ;P->prop
        ;++P
        )
        {
        if(P->onn)
            {
            if(P->status == notSpecified || firsttext.EOL)
                P->status = Off; // First bold spec in segment.
            else if(P->status == On)
                P->status = mixed; // Different bold specs in same segment. Cannot use bold spec info to discern header.
            }
        }
    for(CharPropN * PN = characterPropertyWithNumericalParameter
        ;PN->prop
        ;++PN
        )
        {
        if(PN->N == NotSpecified || firsttext.EOL)
            PN->N = PN->def;
        else if(PN->N != PN->def)
            PN->N = Mixed; // Different font sizes in same segment. Cannot use font size info to discern header.
        }
    }

static int TranslateToken(const char * token)
    {
    const CharToken * chartoken;
    for(chartoken = tokensThatArePartOfTheText
        ;chartoken->token
        ;++chartoken
        )
        {
        if(!strcmp(token,chartoken->token))
            return chartoken->ISOcode;
        }

    if(*token == '\'')
        {
        char buf[3];
        buf[0] = token[1];
        buf[1] = token[2];
        buf[2] = '\0';
        return (int)(unsigned char)(int)strtol(buf,NULL,16);
        }

    return 0;
    }

static int interpretToken(char * tok,const startLine firsttext)
    {
    const char ** p;
    const CharToken * chartoken;
    char token[80];
    if(*tok != '\\')
        return -(int)(unsigned char)*tok;
    ++tok;// skip backslash
    strcpy(token,tok);
    int end = strlen(tok) - 1;
    while(tok[end] == ' ' || tok[end] == (char)0xA0 || tok[end] == '\'' || tok[end] == '\n' || tok[end] == '\r')
        token[end--] = '\0';
    for(p = tokensThatTakeArgument
        ;*p
        ;++p
        )
        {
        if(!strcmp(token,*p))
            {
#ifdef LOGGING
            log(0,'@',false);
#endif
            return skip_tekst;
            }
        }
    for(p = tokensThatAreField
        ;*p
        ;++p
        )
        {
        if(!strcmp(token,*p))
            {
#ifdef LOGGING
//            log(0,'$',false);
#endif
            return field;
            }
        }
    for(p = tokensThatToggle
        ;*p
        ;++p
        )
        {
        if(!strcmp(token,*p))
            {
            return hidden; // 
            }
        }
    for(p = tokensThatReset
        ;*p
        ;++p
        )
        {
        if(!strcmp(token,*p))
            {
            return reset|new_segment;
            }
        }
    int ret = TranslateToken(token);
    if(ret)
        return -ret;

    for(chartoken = tokensThatDemarcateSegments
        ;chartoken->token
        ;++chartoken
        )
        {
        if(!strcmp(token,chartoken->token))
            return new_segment;
        }
    
    for(CharProp * P = characterProperty
        ;P->prop
        ;++P
        )
        {
        if(!strcmp(token,P->prop))
            {
            if(P->onn)
                {
                if(P->status == notSpecified || firsttext.EOL)
                    P->status = On; // First bold spec in segment.
                else if(P->status == Off)
                    P->status = mixed; // Different bold specs in same segment. Cannot use bold spec info to discern header.
                }
            else
                {
                CharProp * Q = P - 1;
                if(Q->status == notSpecified || firsttext.EOL)
                    Q->status = Off; // First unbold spec in segment.
                else if(Q->status == On)
                    Q->status = mixed; // Different bold specs in same segment. Cannot use bold spec info to discern header.
                }
            return character_property;
            }
        }
    for(CharPropN * PN = characterPropertyWithNumericalParameter
        ;PN->prop
        ;++PN
        )
        {
        if(!strncmp(token,PN->prop,strlen(PN->prop)))
            {
            char * q = token+strlen(PN->prop);
            int FS = 0;
            for(;*q && isDigit(*q);++q)
                FS = 10*FS + (*q - '0');
            if(!*q)
                {
                if(PN->N == NotSpecified || firsttext.EOL)
                    PN->N = FS; // First font size spec in segment.
                else if(PN->N != FS)
                    PN->N = Mixed; // Different font sizes in same segment. Cannot use font size info to discern header.
                return character_property;
                }
            }
        }
    return 0;
    }


static const char SEP[] = "!\"\'(),./:;?[]`{}\x82\x83\x84\x85\x86\x87\x88\x89\x91\x92\x93\x94\x95\x96\x97\x98\xab\xbb";// Bart 20051207
static const char QUOTE[] = "\'`\x92";
static const char QUANTITY[] = "$%\x80\x83\x89\xA2\xA3\xA5";//dollar, percent, euro, florin, promille, cent, pound, yen 
static const char RBRACKET[] = ")}]";
static const char LBRACKET[] = "({[";


static bool isSep(int a)
    {
    if(0xA0 <= a && a < 0xC0)
        return true;
    return strchr(SEP,a) != 0;
    }

static bool isQuantity(int a)
    {
    return strchr(QUANTITY,a) != 0;
    }

static void Put4(int c,flags & flgs) // Called from Put3
    {
    static int last = 0;
    if(c != ' ' && c != 0xA0)
        {
        if(c == '\n')
            {
            if(Option.spaceAtEndOfLine)
                fputc(' ',outputtext);// Add extra space before new line (compatibility mode).
            flgs.writtentoline = false;
            }
        else
            {
            flgs.writtentoline = true;
            if(last == ' ' ||  last == 0xA0)
                fputc(' ',outputtext);
            }
        fputc(c,outputtext);
        }
    last = c;
    }

static void Put4A(int c,flags & flgs)// Called from Put3
    {
    int k = AsciiEquivalent[(unsigned char)(c & 255)];
    if(k == -1)
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

static void (*pPut4)(int c,flags & flgs) = Put4;

static void Put3(int ch,flags & flgs) // called from PutN, Put2 and GetPut
    {
    /* Put3 generally causes a newline (ch=='\n') to be written.
       Exception: inside htmltags.
    */

    static int last = 0;
    static int trailingDotFollowingNumber = 0;

    if(flgs.inhtmltag)
        {
        if(trailingDotFollowingNumber)
            {
            if(Option.tokenize)
                pPut4(' ',flgs); // insert blank before dot if number followed by dot is at the end of the line
            pPut4(trailingDotFollowingNumber,flgs);
            trailingDotFollowingNumber = 0;
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
                if(isSpace((int)(unsigned char)ch))
                    {
                    last = ' ';
                    }
                else 
                    {
                    if(last == ' ' ||  last == 0xA0)
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
        if(ch == 0xA0) // 20071112
           ch = ' ';
        else if((unsigned char)ch < ' ') // replace tabs by spaces and all other non-white space by an asterisk
            {
            if(ch == '\t' || ch == '\v') // Bart 20050221
                ch = ' ';
            else if(ch != '\n')
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
            if(trailingDotFollowingNumber)
                {
                if(Option.tokenize)
                    pPut4(' ',flgs); // insert blank before dot if number followed by dot is at the end of the line
                pPut4(trailingDotFollowingNumber,flgs);
                trailingDotFollowingNumber = 0;
                }
            }
        else if(last == ' ' || last == 0xA0)
            {
            int lastToWrite = ' ';
            if(isUpper(ch)) // Might be an indication that a new sentence starts here. 
                            // Check preceding token for trailing dot that might be a
                            // sentence delimiter after all.
                {
                if(trailingDotFollowingNumber) // ... in 1999. Next month ...   ch=='N', last is ' '
                                               //            ^ Not written from here
                    {// Regard dot as sentence delimiter after all
                    if(Option.tokenize)
                        pPut4(' ',flgs); // Insert blank before dot if number followed by dot is followed by capitalised word.
                    pPut4(trailingDotFollowingNumber,flgs);
                    trailingDotFollowingNumber = 0;
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
                            lastToWrite = '\n'; // Abbreviation seems to be the last word of the previous sentence
                            break;
                        }
                    }
                }
            else if(trailingDotFollowingNumber)
                { // Now we suppose that the dot trailing the number is part of that number.
                pPut4(trailingDotFollowingNumber,flgs);
                trailingDotFollowingNumber = 0;
                }
            if(lastToWrite != ' ' && lastToWrite != 0xA0 || flgs.writtentoline)
                pPut4(lastToWrite,flgs);
            flgs.writtentoline = (lastToWrite == ' ' || lastToWrite == 0xA0);
            flgs.in_abbreviation = false;
            }
        if(flgs.number_final_dot == 1) // This can only be the case if ch == separating character
            {
            trailingDotFollowingNumber = ch;
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

static void PutN(char * buf,int len,flags & flgs) // called from Put2
    {
    for(int i = 0;i < len;++i)
        {
        Put3(buf[i],flgs);
        }
    }

void Shift(char *end,int save,char buf2[256],int offset)
    {
    if(save)
        *end = save;
    char * e = buf2+strlen(buf2);
    *(e+1) = '\0';
    for(;e > buf2 + offset;--e)
        *e = *(e - 1);
    *e = ' ';
    }

// see http://www.cis.upenn.edu/~treebank/tokenizer.sed
void PennTreebankTokenize(char buf[],size_t lastNonSeparatingCharacter,bool & abbreviation,flags & flgs ,bool & number)
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
        else if(aa > 0 && aa < lastNonSeparatingCharacter && strchr(QUOTE,buf[aa]))
            {
            buf[aa] = '\'';
            }
        }
    aa = 0;
    int save = buf[lastNonSeparatingCharacter+1];
    buf[lastNonSeparatingCharacter+1] = '\0';
    char buf2[256];
    strcpy(buf2,buf+aa);
    buf[lastNonSeparatingCharacter+1] = save;
    save = '\0';
    int len = strlen(buf2);
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
        char * last3 = buf2 + len - 3;
        if(*last3 == '\'')
            {
            char * last2 = last3 + 1;
            if(  !strcasecmp(last2,"ll")
              || !strcasecmp(last2,"ve")
              || !strcasecmp(last2,"re")
              )
                shift = 3;
            }
        else if(!strcasecmp(last3,"n't"))
            shift = 3;
        }
    if(!shift && len > 2)
        {
        char * last2 = buf2 + len - 2;
        if(*last2 == '\'')
            {
            char * last1 = buf2 + len - 1;
            if(  !strcasecmp(last1,"s")
              || !strcasecmp(last1,"m")
              || !strcasecmp(last1,"d")
              )
            shift = 2;
            }
        }
    if(shift)
        {
        char * e = buf2+strlen(buf2);
        *(e+1) = '\0';
        for(;e > buf2 + len - shift;--e)
            *e = *(e - 1);
        *e = ' ';
        }
    char * end = strchr(buf2,' ');
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
tis = t is
twas = t was
wanna = wan na
whaddya = wha dd ya
whatcha = wha t cha
*/

    if(  !strcasecmp(buf2,"tis")
      || !strcasecmp(buf2,"twas")
      )
        {
        Shift(end,save,buf2,1);
        }
    else if(!strcasecmp(buf2,"d'ye"))
        {
        Shift(end,save,buf2,2);
        }
    else if(  !strcasecmp(buf2,"cannot")
           || !strcasecmp(buf2,"gimme")
           || !strcasecmp(buf2,"gonna")
           || !strcasecmp(buf2,"gotta")
           || !strcasecmp(buf2,"lemme")
           || !strcasecmp(buf2,"wanna")
           )
        {
        Shift(end,save,buf2,3);
        }
    else if(!strcasecmp(buf2,"more'n"))
        {
        Shift(end,save,buf2,4);
        }
    else if(!strcasecmp(buf2,"whatcha"))
        {
        Shift(end,save,buf2,4);
        Shift(end,save,buf2,3);
        }
    else if(!strcasecmp(buf2,"whaddya"))
        {
        Shift(end,save,buf2,5);
        Shift(end,save,buf2,3);
        }
    else
        {
        if(save)
            *end = save;
        }
    PutN(buf2,strlen(buf2),flgs);
    }

static void Put2(const int ch,flags & flgs) // Called from Put
    {
    // Check for brackets, quotes, valuta and other quantities,
    // abbreviations and numbers.

    // Put2 generally writes a newline for each received newline. Exceptions
    // are newlines emitted after numbers and abbreviations with trailing
    // dots, in which case the newline is replaced by a white space.
    // However, this whitespace is possibly changed back to a newline.
    // This happens if a trailing dot after a number is followed by an
    // upper case letter, indicating the start of a sentence.
    static char buf[256]; // Buffer to keep token that must be analysed for 
                          // brackets, dots, etc.
                          // The analysis is done when ch is whitespace and
                          // also when an html-tag is going to begin.
                          // It is necessary to call Put2 with 
                          // flgs.htmltagcoming=true only for the character
                          // immediately preceding the opening brace.
    static size_t pos = 0;
    int ch_loc = ch; // If ch is newline and the preceding token is an 
                     // abbreviation or number with a dot in the end, the dot 
                     // is regarded as part of that token and the newline is
                     // replaced with a simple blank.
    if(pos >= sizeof(buf) - 1)
        {
        PutN(buf,sizeof(buf) - 1,flgs);
        pos = 0;
        buf[0] = '\0';
        }

    if(flgs.inhtmltag)
        {
        if(flgs.firstofhtmltag)
            {
            if(Option.tokenize && !Option.suppressHTML && flgs.notstartofline)
                {
                buf[pos++] = ' ';
                }
            flgs.firstofhtmltag = false;
            }
        buf[pos++] = ch;
        buf[pos] = '\0';
        PutN(buf,pos,flgs);
        pos = 0;
        buf[0] = '\0';
        }
    else if(flgs.htmltagcoming || strchr(" \t\n\r",ch) || ch == 0)
        {
        if(pos)
            {
            buf[pos] = '\0';
            char * pbuf = buf;


            do
                {// Check for sequences of hyphens and put them in separate tokens (if necessary)
                char * h = strstr(pbuf,"--");
                char * nullbyte = buf+pos;
                if(h)
                    {
                    *h = '\0';
                    nullbyte = h;
                    }
                else
                    nullbyte = buf+pos;

                if(nullbyte > pbuf)
                    {










            //size_t /*int*/ firstNonSeparatingCharacter,lastNonSeparatingCharacter,a2,b2;
            char * firstNonSeparatingCharacter;
            char * lastNonSeparatingCharacter;
            char * a2;
            char * b2;
            bool lquote = false;
            bool rquote = false;
            /*char * dlquote = NULL;
            char * drquote = NULL;*/
            //int rbracket = 0;
            char * rbracket = NULL;
            for(firstNonSeparatingCharacter = pbuf;*firstNonSeparatingCharacter && isSep(*firstNonSeparatingCharacter);++firstNonSeparatingCharacter)
                {
                if(strchr(QUOTE,*firstNonSeparatingCharacter))
                    lquote = true;
                /*
                if(*firstNonSeparatingCharacter == '\"')
                    dlquote = firstNonSeparatingCharacter;*/
                }
            for(lastNonSeparatingCharacter = nullbyte - 1;lastNonSeparatingCharacter >= firstNonSeparatingCharacter && isSep(*lastNonSeparatingCharacter);--lastNonSeparatingCharacter)
                {
                if(!rbracket && strchr(RBRACKET,*lastNonSeparatingCharacter))
                    rbracket = lastNonSeparatingCharacter;

                if(strchr(QUOTE,*lastNonSeparatingCharacter))
                    rquote = true;
                else
                    rquote = false;
                /*
                if(*lastNonSeparatingCharacter == '\"')
                    drquote = lastNonSeparatingCharacter;
                    */
                }

/*                if(!dlquote || !drquote)
                dlquote = drquote = NULL;*/
            
            if(rquote && !lquote)
                ++lastNonSeparatingCharacter; // A right quote without corresponding left quote is regarded part of the word, ikk'
            else if(rbracket)
                {
                char * L = strpbrk(firstNonSeparatingCharacter,LBRACKET);
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
                    char * p;
                    if(  pbuf < (p=strchr(firstNonSeparatingCharacter,'&'))
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
                    char * p;
                    if(  pbuf < (p=strchr(firstNonSeparatingCharacter,';'))
                      && p <= lastNonSeparatingCharacter
                      )
                        --firstNonSeparatingCharacter;// html character entity, for example &amp;cetera
                    }

                char * aa;
                // Write the separating characters at the beginning
                for(aa = pbuf;aa < firstNonSeparatingCharacter;++aa)
                    {
                    /*
                    if(Option.PennTreebankTokenization && dlquote == aa)
                        {
                        Put3('`',in_abbreviation);
                        Put3('`',in_abbreviation);
                        }
                    else*/
                    Put3(*aa,flgs);
                    if(Option.tokenize)
                        Put3(' ',flgs);
                    }
                bool abbreviation = false;
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
                            Put3(*aa,flgs);
                            }
                        }
                    }
                char * SeparatingCharacter = lastNonSeparatingCharacter + 1;
                if(*SeparatingCharacter == '.') // This is the 
                    // dot immediately following a word or number 
                    // (sequence of one or more non-separating characters)
                    {
                    if(!abbreviation)
                        {
                        if(SeparatingCharacter == firstNonSeparatingCharacter+1) // 20040120 Single characters followed by period are always abbreviations
                            {
                            abbreviation = true;
                            possibly_initial = isUpper(*firstNonSeparatingCharacter);
                            }
                        else if(firstNonSeparatingCharacter > pbuf) // 20040120 word starts with separator, e.g. "(Ref."
                            {
                            abbreviation = true;
                            }
                        else
                            {
                            // Check whether the word (including the final dot) is in the list of abbreviations.
                            int save = *(SeparatingCharacter+1);
                            *(SeparatingCharacter+1) = '\0';
                            if(Abbreviation(firstNonSeparatingCharacter))
                                {
                                abbreviation = true;
                                }
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
            while(true);



            pos = 0;
            }
        if(ch)
            {
            Put3(ch_loc,flgs);
            }
        }
    else
        {
        if(flgs.firstafterhtmltag)
            {
            flgs.firstafterhtmltag = false;
            if(Option.tokenize && !Option.suppressHTML)
                {
                buf[pos++] = ' ';
                }
            }
        buf[pos++] = ch;
        }
    }

static void Put(int ch,flags & flgs) // Called from GetPut, GetPutBullet and doTheSegmentation
    {
    if(!outputtext)
        return;
    static int last = '\n'; // last character that was not space or tab (we ignore trailing spaces and tabs)
    static bool CRseen = false;
    static bool NLseen = false;
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
                    if(NLseen && ch == '\n' || CRseen && ch == '\r')
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
            if(ch != ' ' && ch != '\t' && ch != 0xA0)
                {
                if(Option.emptyline)
                    neededSegmentDelimiters = 2;
                else
                    neededSegmentDelimiters = 1;
                }
            Put2(ch,flgs);
            }
        }
    if(ch != ' ' && ch != '\t' && ch != 0xA0)
        {
        switch(ch)
            {
            case '\r':
                CRseen = true;
                last = '\n';
                flgs.firstafterhtmltag = false; // Supress space after tag
                flgs.notstartofline = false;
                break;
            case '\n':
                NLseen = true;
                last = '\n';
                flgs.firstafterhtmltag = false; // Supress space after tag
                flgs.notstartofline = false;
                break;
            default:
                CRseen = NLseen = false;
                flgs.notstartofline = true;
                last = ch;
            }
        }
    }


static int GetPutText(const long end_offset,flags & flgs)
    {
//    static bool skipSpace = false;
    int ch = 0;
    while(ftell(sourceFile) < end_offset - 1)
        {
        ch = getc(sourceFile);
        if(targetFile)
            fputc(ch,targetFile);
        if(ch != '\n' && ch != '\r' || flgs.inhtmltag)
            {
            Put(ch,flgs);
            }
        }
    if(flgs.htmltagcoming)
        {
        flgs.inhtmltag = true;
        flgs.firstofhtmltag = true;
        flgs.htmltagcoming = false;
        }
    if(ftell(sourceFile) < end_offset)
        {
        ch = getc(sourceFile);
        if(targetFile)
            fputc(ch,targetFile);
        if(ch != '\n' && ch != '\r' || flgs.inhtmltag)
            {
            Put(ch,flgs);
            }
        }
    return ch;
    }

static int GetPut(const long end_offset,flags & flgs)
    {
    int ch = 0;
    while(ftell(sourceFile) < end_offset)
        {
        ch = getc(sourceFile);
        if(targetFile)
            fputc(ch,targetFile);
        if(ch == '\\')
            {
            ch = getc(sourceFile);
            if(targetFile)
                fputc(ch,targetFile);
            switch(ch)
                {
                case '\'':
                    {
                    char buf[3];
                    ch = getc(sourceFile);
                    buf[0] = ch;
                    if(targetFile)
                        fputc(ch,targetFile);
                    ch = getc(sourceFile);
                    buf[1] = ch;
                    if(targetFile)
                        fputc(ch,targetFile);
                    buf[2] = '\0';
                    ch = (unsigned char)(int)strtol(buf,NULL,16);
                    Put(ch,flgs);
                    break;
                    }
                case '\\':
                    {
                    Put(ch,flgs);
                    break;
                    }
                case '-': // Optional hyphen
                case '*': // Marks a destination whose text should be ignored if not understood by the RTF reader.
                case ':': // Specifies a subentry in an index entry.
                case '|': // Formula character. (Used by Word 5.1 for the Macintosh as the beginning delimiter for a string of formula typesetting commands.)
                    {
                    break;
                    }
                case '_':
                    {
                    Put('-',flgs); // Nonbreaking hyphen
                    break;
                    }
                case '~': // Bart 20060303 Nonbreaking space
                    {
                    Put(' ',flgs);
                    break;
                    }
                default:
                    {
                    char buf[256];
                    size_t i = 1;
                    buf[0] = ch;
                    while(ftell(sourceFile) < end_offset && (ch = getc(sourceFile)) != EOF && /*ControlWordChar(ch)*/ ch != ' ' && ch != 0xA0)
                        {
                        if(i+1 < sizeof(buf))
                            buf[i++] = ch;
                        }
                    buf[i] = '\0';
                    int c = TranslateToken(buf);
                    if(c)
                        Put(c,flgs);
                    }
                }
            }
        else if(ch != '\n' && ch != '\r')
            Put(ch,flgs);
        }
    return ch;
    }

static int GetPutBullet(const long end_offset,flags & flgs)
    {
    int ch = 0;
    while(ftell(sourceFile) < end_offset)
        {
        ch = getc(sourceFile);
        }
    if(targetFile)
        fputc(Option.bullet,targetFile);
    Put(Option.bullet,flgs);
    Put(' ',flgs);
    return ch;
    }

static int doTheSegmentationText(const bool nl,bool & oldnl,const long end_offset,long & begin_offset,flags & flgs,bool & WritePar,long & targetFilePos)
    {
    int ret = 0; // 0 if space
    if(-1L < begin_offset && begin_offset < end_offset)
        {
#ifdef LOGGING
            log(0,'|',false);
        char temp[256];
        sprintf(temp,"[%ld,%ld]",begin_offset,end_offset);
        logn(temp);
#endif
//        flgs.newSegment = true; // Signal to Put3 to let it handle sentence-ending abbreviations
        long cur = ftell(sourceFile);
        
        static bool myoldnl = 0;
        bool writepar = false;
        int ch = 0;
        if(!oldnl)
            oldnl = myoldnl;
        if(oldnl)
            {
            Put(' ',flgs);
            }
        if(targetFile)
            {
            fseek(sourceFile,targetFilePos,SEEK_SET);
            while(ftell(sourceFile) < begin_offset)
                {
                fputc(getc(sourceFile),targetFile);
                }
            }
        fseek(sourceFile,begin_offset,SEEK_SET);
/*        char temp[256];
        sprintf(temp,"{fs %d -> %d}",oldfs,fs);
        logn(temp);*/
        if(writepar || WritePar)
            {
            if(oldnl)
                {
                if(Option.emptyline)
                    {
                    if(targetFile)
                        fprintf(targetFile,"\\par ");
                    Put('\n',flgs);
                    }
                Put('\n',flgs);
                Put('\n',flgs);
                }
//            writepar = false;
            WritePar = false;
            }
        myoldnl = nl;
        static int nbullets = 0;
        if(flgs.bbullet)
            {
            if(!Option.B)
                {
                if(Option.separateBullets)
                    {
                    if(nbullets < 5)
                        {
                        Put('\n',flgs);
                        Put('\n',flgs);
                        }
                    ret = ch = GetPutText(end_offset,flgs);
//                    fprintf(outputtext,"A%c",ret);// TEST
                    if(Option.emptyline)
                        {
                        if(targetFile)
                            fprintf(targetFile,"\\par ");
                        Put('\n',flgs);
                        ret = 0;
                        }
                    if(nbullets < 5)
                        {
                        Put('\n',flgs);
                        Put('\n',flgs);
                        ret = 0;
                        }
                    flgs.bbullet = false;
                    ++nbullets;
                    }
                else
                    {
                    nbullets = 0;
                    ch = GetPutText(end_offset,flgs);
//                    fprintf(outputtext,"B%c",ret);// TEST
                    ret = 1;
                    }
                }
            else if(Option.bullet)
                {
                if(Option.separateBullets)
                    {
                    if(nbullets < 5)
                        {
                        Put('\n',flgs);
                        Put('\n',flgs);
                        }
                    ret = ch = GetPutBullet(end_offset,flgs);
//                    fprintf(outputtext,"C%c",ret);// TEST
                    if(Option.emptyline)
                        {
                        if(targetFile)
                            fprintf(targetFile,"\\par ");
                        Put('\n',flgs);
                        ret = 0;
                        }
                    if(nbullets < 5)
                        {
                        Put('\n',flgs);
                        Put('\n',flgs);
                        ret = 0;
                        }
                    flgs.bbullet = false;
                    ++nbullets;
                    }
                else
                    {
                    nbullets = 0;
                    ret = ch = GetPutBullet(end_offset,flgs);
//                    fprintf(outputtext,"D%c",ret);// TEST
                    }
                }
            else
                {
                if(Option.separateBullets)
                    {
                    if(nbullets < 5)
                        {
                        Put('\n',flgs);
                        Put('\n',flgs);
                        }
                    if(Option.emptyline)
                        {
                        if(targetFile)
                            fprintf(targetFile,"\\par ");
                        Put('\n',flgs);
                        }
                    flgs.bbullet = false;
                    ++nbullets;
                    }
                else
                    {
                    nbullets = 0;
                    }
                }
            }
        else
            {
            nbullets = 0;
            if(flgs.htmltagcoming)
                {
//                fprintf(outputtext,"((",ret); // TEST
                ret = ch = GetPutText(end_offset,flgs);
  //              fprintf(outputtext,"))"); // TEST
                }
            else
                ret = ch = GetPutText(end_offset,flgs);
            }

        if(nl)
            {
            if(ch == '\n' || ch == '\r')
                Put(ch,flgs);
            else
                Put('\n',flgs);
            ret = 0;
            }
        else
            {
            switch(ch)
                {
                case ';':
                case '?':
                case '.':
                case '!':
#if COLONISSENTENCEDELIMITER
                case ':':
#endif
                case '\n':
                    Put('\n',flgs);
                    ret = 0;
                    break;
                default:
                    {
                    }
                }
            }
        targetFilePos = end_offset;
        fseek(sourceFile,cur,SEEK_SET);
        begin_offset = end_offset;//-1L; 20040120 There are no characters to be excluded between end_offset and begin_offset
        oldnl = false;
        flgs.bbullet = false;
        }
    return ret && !isSpace(ret);
    }

static long doTheSegmentation(eStatus i, eStatus b, eStatus scaps, int fs,int f,bool nl,bool &oldnl,const long end_offset,const long begin_offset,flags & flgs,bool & WritePar,long & targetFilePos)
    {
    if(-1L < begin_offset && begin_offset < end_offset)
        {
#ifdef LOGGING
            log(0,'|',false);
        char temp[256];
        sprintf(temp,"[%ld,%ld]i=%d,b=%d,scaps=%d,fs=%d,f=%d,nl=%d,oldnl=%d",begin_offset,end_offset,i,b,scaps,fs,f,nl,oldnl);
        logn(temp);
#endif
//        flgs.newSegment = true; // Signal to Put3 to let it handle sentence-ending abbreviations
        long cur = ftell(sourceFile);
        
        static int oldfs = Uninitialised;
        static int oldf = Uninitialised;
        static eStatus oldi = uninitialised;
        static eStatus oldb = uninitialised;
        static eStatus oldscaps = uninitialised;
//        static bool myoldnl = 0;
        bool writepar = false;
        int ch = 0;
//        if(!oldnl)
  //          oldnl = myoldnl;
        if(oldnl)
            {
            if(Option.emptyline)
                Put('\n',flgs);
            Put('\n',flgs);
            }
        if(targetFile)
            {
            fseek(sourceFile,targetFilePos,SEEK_SET);
            while(ftell(sourceFile) < begin_offset)
                {
                fputc(getc(sourceFile),targetFile);
                }
            }
        else
            fseek(sourceFile,begin_offset,SEEK_SET);
/*        char temp[256];
        sprintf(temp,"{fs %d -> %d}",oldfs,fs);
        logn(temp);*/
        if(oldfs != fs)
            {
            if(oldfs != Uninitialised && fs != Mixed)
                {
                writepar = true;
                }
            oldfs = fs;
            }
            /* Font changes can happen in the middle of a word */
        if(oldnl && oldf != f)
            {
            if(oldf != Uninitialised && f != Mixed)
                {
                writepar = true;
                }
            oldf = f;
            }/**/
        if(oldi != i)
            {
            if(oldi != uninitialised && i != mixed)
                {
                writepar = true;
                }
            oldi = i;
            }
        if(oldb != b)
            {
            if(oldb != uninitialised && b != mixed)
                {
                writepar = true;
                }
            oldb = b;
            }
        if(oldscaps != scaps)
            {
            if(oldscaps != uninitialised && scaps != mixed)
                {
                writepar = true;
                }
            oldscaps = scaps;
            }
        if(writepar || WritePar)
            {
            if(oldnl)
                {
                if(Option.emptyline)
                    {
                    if(targetFile)
                        fprintf(targetFile,"\\par ");
                    Put('\n',flgs);
                    }
                Put('\n',flgs);
                Put('\n',flgs);
                }
//            writepar = false;
            WritePar = false;
            }
//        myoldnl = nl;
        static int nbullets = 0;
        if(flgs.bbullet)
            {
            if(!Option.B)
                {
                if(Option.separateBullets)
                    {
                    if(nbullets < 5)
                        {
                        Put('\n',flgs);
                        Put('\n',flgs);
                        }
                    ch = GetPut(end_offset,flgs);
                    if(Option.emptyline)
                        {
                        if(targetFile)
                            fprintf(targetFile,"\\par ");
                        Put('\n',flgs);
                        }
                    if(nbullets < 5)
                        {
                        Put('\n',flgs);
                        Put('\n',flgs);
                        }
                    flgs.bbullet = false;
                    ++nbullets;
                    }
                else
                    {
                    nbullets = 0;
                    ch = GetPut(end_offset,flgs);
                    }
                }
            else if(Option.bullet)
                {
                if(Option.separateBullets)
                    {
                    if(nbullets < 5)
                        {
                        Put('\n',flgs);
                        Put('\n',flgs);
                        }
                    ch = GetPutBullet(end_offset,flgs);
                    if(Option.emptyline)
                        {
                        if(targetFile)
                            fprintf(targetFile,"\\par ");
                        Put('\n',flgs);
                        }
                    if(nbullets < 5)
                        {
                        Put('\n',flgs);
                        Put('\n',flgs);
                        }
                    flgs.bbullet = false;
                    ++nbullets;
                    }
                else
                    {
                    nbullets = 0;
                    ch = GetPutBullet(end_offset,flgs);
                    }
                }
            else
                {
                if(Option.separateBullets)
                    {
                    if(nbullets < 5)
                        {
                        Put('\n',flgs);
                        Put('\n',flgs);
                        }
                    if(Option.emptyline)
                        {
                        if(targetFile)
                            fprintf(targetFile,"\\par ");
                        Put('\n',flgs);
                        }
                    flgs.bbullet = false;
                    ++nbullets;
                    }
                else
                    {
                    nbullets = 0;
                    }
                }
            }
        else
            {
            nbullets = 0;
            ch = GetPut(end_offset,flgs);
            }
        if(nl)
            Put('\n',flgs);
        else
            {
            switch(ch)
                {
                case ';':
                case '?':
                case '.':
                case '!':
#if COLONISSENTENCEDELIMITER
                case ':':
#endif
              //  case '\n':
                    Put('\n',flgs);
                    break;
                default:
                    {
                    }
                }
            }
        targetFilePos = end_offset;
        fseek(sourceFile,cur,SEEK_SET);
        //begin_offset = -1L;
        oldnl = false;
        flgs.bbullet = false;
        return -1;//begin_offset
        }
    return begin_offset;
    }
        
typedef int (*lookahead_fnc)(FILE * fp,const startLine firsttext);

static int lookaheadText(FILE * fp,const startLine firsttext)
    {
    int next = getc(fp);
    ungetc(next,fp);
    return next;
    }

static int lookaheadRTF(FILE * fp,const startLine firsttext)
    {
    long pos = ftell(fp);
    int next = getc(fp);
    if(next == '\\')
        {
        char * token = parseRTFtoken(1);
        if(token)
            {
            int interpretation = interpretToken(token,firsttext);
            if(interpretation < 0)
                next = -interpretation;
            }
        fseek(fp,pos,SEEK_SET);
        }
    else
        {
        ungetc(next,fp);
        }
    return next;
    }

static bool checkSentenceEnd(int ch,long & begin_offset,lookahead_fnc lookahead,const startLine firsttext,bool & in_fileName,const long curr_pos,flags & flgs)
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
    static int in_number = 0;
    static int in_word = 0;
    static bool character_entity = false;

    if(isSpace(ch))
        {
        if(firsttext.EOL)
            {
            begin_offset = curr_pos; //ftell(sourceFile) - 1;
            }
        if(flgs.bbullet)
            {
            sentenceEnd = true; // force writing of segment
            }
        }
    else if(flgs.bbullet)
        {
        flgs.bbullet = (  (ch & 0x80) 
                  || (ch == '-') 
                  || (ch == '*') 
                  ||    (unsigned char)ch < ' ' 
                     && ch != '\n' 
                     && ch != '\r' 
                     && ch != '\t' 
                     && ch != '\v' 
                     && ch != '\b' 
                  ); // The first bullet-like character is immediately followed by more bullet-like characters.
        }
    else 
        {
        if(firsttext.EOL)
            {
            begin_offset = curr_pos; //ftell(sourceFile) - 1;
            }
        if(firsttext.b.LS)
            flgs.bbullet = (  (ch & 0x80) 
                           || (ch == '-') 
                           || (ch == '*') 
                           ||    (unsigned char)ch < ' ' 
                              && ch != '\n' 
                              && ch != '\r' 
                              && ch != '\t' 
                              && ch != '\v' 
                              && ch != '\b' 
                           ); // First character on line seems to indicate a bullet.

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
                in_fileName = false;
                break;
            case ')':
                /* Closing parenthesis is NOT regarded as sentence delimiter if
                        it is part of a file name               c:\file(a).txt
                        it has no opening parenthesis           So we say A) this and b) that.
                */
                if(!in_fileName && in_parentheses)
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
                if(!in_fileName)
                    {
                    int next = lookahead(sourceFile,firsttext);
                    if(!islower(next))
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
                int next = lookahead(sourceFile,firsttext);
                if(strchr(" \\/:*?\"<>|",next))
                    in_fileName = false;
                if(!in_fileName)
                    {
                    if(next == '.')
                        {
                        ellipsis++;
                        }
                    else if(  !ellipsis
                           && !(  (in_word && isAlpha(next))
                               || (in_number && (next == ' ' || next == 0xA0 || isDigit(next)))
                               )
                           )
                        sentenceEnd = true;
                    }
                break;
                }
            case ':':
                /* Colon is NOT regarded as sentence delimiter if
                        it stands between digits                                2:3
                        it has a letter to the left and a slash to the right    c:/
                */
                in_fileName = false;
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
                    int next = lookahead(sourceFile,firsttext);
                    if(next == '\\' || next == '/')
                        in_fileName = true;
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
                in_fileName = false;
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

static void segmentdefaultText(int ch,long & begin_offset,lookahead_fnc lookahead,startLine & firsttext,flags & flgs,long & targetFilePos,bool & oldnl,long & end_offset,bool & WritePar)
    {
    static bool in_fileName = false;
    if(  ch ==  '\n'
      || ch == '\r'
      || ch == EOF
      || ch == 26
      )
        {
        in_fileName = false;
        if(firsttext.b.CR && ch == '\r' || firsttext.b.LF && ch == '\n') // two times LF or two times CR implicates empty line!
            WritePar = true; // Bart 20050714. Headers weren't set 
        // apart when asked to put empty lines after lines not ending 
        // with punctuation.
        end_offset = ftell(sourceFile);// Bart 20040120. Suppose that end of line is end of segment
        doTheSegmentationText(true,oldnl,end_offset,begin_offset,flgs,WritePar,targetFilePos); // Bart 20040120. true because: Suppose that end of line is end of segment
//        fputc('^',outputtext);// TEST
        if(ch == '\n')
            {
            firsttext.b.LF = 1;
            }
        else if(ch == '\r')
            {
            firsttext.b.CR = 1;
            }
        else
            {
            firsttext.b.CR = 1;
            firsttext.b.LF = 1;
            }
        firsttext.b.WS = 1;
        if(firsttext.EOL) // Bart 20061214
            firsttext.b.LS = 1;
        }
    else
        {
        int EOL = firsttext.EOL;
        bool sentenceEnd = checkSentenceEnd(ch,begin_offset,lookahead,firsttext,in_fileName,end_offset,flgs);
//        fputc('~',outputtext);// TEST
        end_offset = ftell(sourceFile);// Bart 20030512. Otherwise the sentence delimiter isn't part of the segment
//Bart 20061214        firsttext.EOL = 0; // resets SD, CR and LF
        if(  sentenceEnd 
          || flgs.htmltagcoming
          || flgs.inhtmltag
          || end_offset - begin_offset > MAXSEGMENTLENGTH && isSpace(ch) // check for buffer overflow
          )
            {
            doTheSegmentationText(false,oldnl,end_offset,begin_offset,flgs,WritePar,targetFilePos);
            firsttext.b.SD = 1;
            firsttext.b.WS = 0;
            firsttext.b.LS = 0;
//            fputc('^',outputtext);// TEST
            }
        if(isSpace(ch))
            {
           // fprintf(outputtext,"CH=%c%d",ch,ch);//TEST
            firsttext.b.WS = 1;
            if(EOL)
                firsttext.b.LS = 1;
            }
        else
            {
            firsttext.b.WS = 0;
            firsttext.b.LS = 0;
            }
        firsttext.EOL = 0; // resets SD, CR and LF
        }
    }

static void segmentdefault(int ch,int sstatus,eStatus i,eStatus b,eStatus scaps,int fs,int f,long & end_offset,long & begin_offset,lookahead_fnc lookahead,startLine & firsttext,bool & in_fileName,const long curr_pos,flags & flgs,bool & WritePar,long & targetFilePos,bool & oldnl)
    {
    if(  !(sstatus & (skip_tekst|hidden))
      && ch != '\n' && ch != '\r'
      )
        {
        bool sentenceEnd = checkSentenceEnd(ch,begin_offset,lookahead,firsttext,in_fileName,curr_pos,flgs);
        firsttext.EOL = 0;
//            fputc('~',outputtext);// TEST
        end_offset = ftell(sourceFile);// Bart 20030512. Otherwise the sentence delimiter isn't part of the segment
        if(  sentenceEnd
          || end_offset - begin_offset > MAXSEGMENTLENGTH && isSpace(ch) // check for buffer overflow
          )
            {
            begin_offset = doTheSegmentation(i,b,scaps,fs,f,false,oldnl,end_offset,begin_offset,flgs,WritePar,targetFilePos);
            firsttext.b.SD = 1;
//            fputc('^',outputtext);// TEST
            }
        if(!isSpace(ch))
            {
            firsttext.b.WS = 0;
            firsttext.b.LS = 0;
            }
        }
    }


static bool segmentText(flags & flgs)
    {
    startLine firsttext = {{1,0,0,1}}; // SD, LF, CR, WS
    long targetFilePos = 0L;
    bool oldnl = false;
    bool WritePar = false;
    int ch;
    long begin_offset = 0; // The position that indicates the first character that has not been written to output.
    //long end_offset = 0;   // When calling doTheSegmentationText, this indicates the last position to be written.
    long curr_pos; // After parsing a html-tag, seeking to curr_pos brings you back to the position where the parsed sequence started.                   
    long tagendpos = 0;
    curr_pos = ftell(sourceFile);
    html_tag_class html_tag;
    do
        {
        ch = getc(sourceFile);
        estate Seq = notag;
/*
        if(isSpace(ch))
            {
            firsttext.b.WS = 1;
            if(ch == '\r')
                firsttext.b.CR = 1;
            else if(ch == '\n')
                firsttext.b.LF = 1;
            }
        else
            {
            firsttext.i = 0;
            }
*/
        if(curr_pos >= tagendpos)
            {
            if(flgs.inhtmltag)
                {
                flgs.firstafterhtmltag = true;
                flgs.inhtmltag = false;
                }

            flgs.htmltagcoming = false;
            if(ch != EOF && ch != 26 && (html_tag.*tagState)(ch) == tag)
                {
                do
                    {
                    ch = getc(sourceFile);
                    if(ch == EOF || ch == 26)
                        {
                        Seq = notag;
                        break;
                        }
                    Seq = (html_tag.*tagState)(ch);
                    }
                while(Seq == tag || Seq == endoftag_startoftag);
                        /*notag,tag,endoftag,endoftag_startoftag*/

                if(Seq == notag)
                    { // Not an HTML tag. Backtrack.
                    fseek(sourceFile,curr_pos,SEEK_SET);
                    }
                else
                    {
                    tagendpos = ftell(sourceFile);
                    }
                ch = getc(sourceFile);
                }
            if(Seq == endoftag)
                {
                fseek(sourceFile,curr_pos,SEEK_SET);
                ch = getc(sourceFile);
    /*          long endpos = ftell(sourceFile);
                fseek(sourceFile,curr_pos,SEEK_SET);
                fprintf(outputtext,"START");
                do
                    {
                    fputc(fgetc(sourceFile),outputtext);
                    }
                while(ftell(sourceFile) < endpos - 1);
                fprintf(outputtext,"END");
                fseek(sourceFile,endpos,SEEK_SET);*/
                flgs.htmltagcoming = true;
                }
            }
        else if(flgs.htmltagcoming)
            {
            flgs.inhtmltag = true;
            flgs.htmltagcoming = false;
            }


        if(flgs.htmltagcoming)
            {
            segmentdefaultText(ch,begin_offset,lookaheadText,firsttext,flgs,targetFilePos,oldnl,curr_pos,WritePar);
            }
        else if(flgs.inhtmltag)
            {
            segmentdefaultText(ch,begin_offset,lookaheadText,firsttext,flgs,targetFilePos,oldnl,curr_pos,WritePar);
            }
        else
            segmentdefaultText(ch,begin_offset,lookaheadText,firsttext,flgs,targetFilePos,oldnl,curr_pos,WritePar);
        }
    while(ch != EOF && ch != 26);

    if(targetFile)
        {
        fseek(sourceFile,targetFilePos,SEEK_SET);
        while((ch = getc(sourceFile)) != EOF && ch != 26)
            {
            fputc(ch,targetFile);
            }
        }
    return false;
    }

static bool segment(int level,int sstatus,bool PrevIsField
    ,eStatus i
    ,eStatus b
    ,eStatus scaps
    ,int fs
    ,int f,long & begin_offset,flags & flgs)
    {
    static long end_offset = 0;
    static startLine firsttext = {{1,0,0,1}};  // Turned on by start of segment (rtf). Turned off in all other cases.
    static bool in_fileName = false;
    static bool WritePar = false;
    static long targetFilePos = 0L;
    static bool oldnl = false;

    static long curr_pos;

    ::level = level;
    int ch;
//    int local_status = 0;
/*
    eStatus i = notSpecified;
    eStatus b = notSpecified;
    eStatus scaps = notSpecified;
    int fs = NotSpecified;
    int f = NotSpecified;
*/
    curr_pos = ftell(sourceFile);
    bool prevIsField = false;
    bool isField = false;
    while((ch = getc(sourceFile)) != EOF && ch != 26)
        {
        switch(ch)
            {
            case '{':
                in_fileName = false;
#ifdef LOGGING
                log(level,ch,true);
                if(sstatus & (skip_tekst|hidden))
                    log(0,'@',false);
#endif
                prevIsField = segment(level+1,/*local_status |*/ sstatus,prevIsField,i,b,scaps,fs,f,begin_offset,flgs);
#ifdef LOGGING
                if(sstatus & (skip_tekst|hidden))
                    log(0,'@',false);
                log(level,'}',true);
#endif
                break;
            case '}':
                in_fileName = false;
                firsttext.b.SD = 1;
                begin_offset = doTheSegmentation(i,b,scaps,fs,f,false,oldnl,end_offset,begin_offset,flgs,WritePar,targetFilePos);
                return isField;
            case '\\':
                {
#ifdef LOGGING
                log(level,ch,false);
#endif
                char * token = parseRTFtoken(level/*,*//*local_status | *//*sstatus*/);
                if(token)
                    {
#ifdef LOGGING
        char temp[256];
        sprintf(temp,"A fs %d",fs);
        logn(temp);
#endif
                    characterProperty[0].status = i;
                    characterProperty[2].status = b;
                    characterProperty[4].status = scaps;
                    characterPropertyWithNumericalParameter[0].N = fs;
                    characterPropertyWithNumericalParameter[1].N = f;
                    int interpretation = interpretToken(token,firsttext);
                    fs = characterPropertyWithNumericalParameter[0].N;
                    f = characterPropertyWithNumericalParameter[1].N;
                    i = characterProperty[0].status;
                    b = characterProperty[2].status;
                    scaps = characterProperty[4].status;
#ifdef LOGGING
        sprintf(temp,"B fs %d",fs);
        logn(temp);
#endif
                    if(interpretation < 0) // just a character (sign inverted)
                        {
                        segmentdefault(-interpretation,(/*local_status|*/sstatus),i,b,scaps,fs,f,end_offset,begin_offset,lookaheadRTF,firsttext,in_fileName,curr_pos,flgs,WritePar,targetFilePos,oldnl);
                        }
                    else
                        {
                        if(PrevIsField)
                            {
                            PrevIsField = false;
#ifdef LOGGING
                        log(0,'#',false);
#endif
                            interpretation = new_segment;
                            WritePar = true;
                            }

                        if(interpretation == field)
                            {
                            isField = true;
                            }
                        else if(interpretation == new_segment)
                            {
                            if(begin_offset < 0)
                                {
                                oldnl = true; // found \par between segments, eg
                                              // {\b\fs47\cf1 4}\par {\b\f1\fs22\cf2 The idea\par }
                                              // Also happens if two \par follow each other
                                }
                            else
                                {
                                begin_offset = doTheSegmentation(i,b,scaps,fs,f,true,oldnl,end_offset,begin_offset,flgs,WritePar,targetFilePos);
                                }
                            firsttext.b.SD = 1;
                            firsttext.b.WS = 1;
                            firsttext.b.LS = 1;
                            }
                        else if(interpretation & reset)
                            {
                            /*local_*/sstatus &= ~hidden;
                            characterProperty[0].status = i;
                            characterProperty[2].status = b;
                            characterProperty[4].status = scaps;
                            characterPropertyWithNumericalParameter[0].N = fs;
                            characterPropertyWithNumericalParameter[1].N = f;
                            resetCharProp(firsttext);
                            fs = characterPropertyWithNumericalParameter[0].N;
                            f = characterPropertyWithNumericalParameter[1].N;
                            i = characterProperty[0].status;
                            b = characterProperty[2].status;
                            scaps = characterProperty[4].status;
                            }
                        else
                            /*local_*/sstatus |= interpretation;
                        }
                    }
                break;
                }
            default:
#ifdef LOGGING
                log(level,ch,false);
#endif
                segmentdefault(ch,(/*local_status|*/sstatus),i,b,scaps,fs,f,end_offset,begin_offset,lookaheadRTF,firsttext,in_fileName,curr_pos,flgs,WritePar,targetFilePos,oldnl);
            }
        curr_pos = ftell(sourceFile);
        }
    if(ch == EOF && level != 0)
        printf("\n%d unbalanced braces\n",level);
    else if(targetFile)
        {
        fseek(sourceFile,targetFilePos,SEEK_SET);
        while((ch = getc(sourceFile)) != EOF && ch != 26)
            {
#ifdef LOGGING
//            log(level,ch,false);
#endif
            fputc(ch,targetFile);
            }
        }
    return isField;
    }
    
int main(int argc,char ** argv)
    {
    flags flgs = {0};

    eStatus i = notSpecified;
    eStatus b = notSpecified;
    eStatus scaps = notSpecified;
    int fs = NotSpecified;
    int f = NotSpecified;

    readArgs(argc, argv,Option);
    if(Option.A)
        pPut4 = Put4A;
    if(Option.arga)
        {
//        LOG("arga %s",Option.arga);
        FILE * a = fopen(Option.arga,"rt");
        if(a)
            {
            readAbbreviations(a);
            }
        else
            {
            printf("\nError in opening abbreviations file %s\n",Option.arga);
            return 1;
            }
        }
    if(Option.argi)
        {
//        LOG("argi %s",Option.argi);
        sourceFile = fopen(Option.argi,"rb");
        if(!sourceFile)
            {
            printf("\nError in opening input file %s\n",Option.argi);
            return 1;
            }
        }
    else
        {
        printf("No input file specified. See %s -h\n",argv[0]);
        return 1;
        }
    
    if(Option.argt)
        {
//        LOG("argt %s",Option.argt);
        outputtext = fopen(Option.argt,"wb");
        if(!outputtext)
            {
            printf("\nError in opening output file %s\n",Option.argt);
            fclose(sourceFile); // Option.argi
            return 1;
            }
        }
    else
        {
        outputtext = NULL;
        }
    
    if(Option.argr)
        {
//        LOG("argr %s",Option.argr);
        targetFile = fopen(Option.argr,"wb");
        if(!targetFile)
            {
            printf("\nError in opening output file %s\n",Option.argr);
            fclose(sourceFile);
            if(outputtext)
                fclose(outputtext);
            return 1;
            }
        }
    if(Option.x == '+')
        {
        char buf[6];
        fread(buf,1,5,sourceFile);
        rewind(sourceFile);
        buf[5] = '\0';
        if(strcmp(buf,"{\\rtf"))
            segmentText(flgs);
        else
            {
            long begin_offset = 0;
            segment(0,0,false,i,b,scaps,fs,f,begin_offset,flgs);
            }
        }
    else if(Option.x == 0)
        segmentText(flgs);
    else
        {
        long begin_offset = 0;
        segment(0,0,false,i,b,scaps,fs,f,begin_offset,flgs);
        Put2(0,flgs);
        }
    Put2(0,flgs); // Empty buffer

    fclose(sourceFile);
    if(outputtext)
        {
        Put4('\n',flgs);
        fclose(outputtext);
        if(Option.argm)
            {
//            LOG("argm %s",Option.argm);
            FILE * a = fopen(Option.argm,"rt");
            if(a)
                {
                readMWU(a);
//                readAbbreviations(a,mwuX,mwucnt,mwu,mwulen);
                outputtext = fopen(Option.argt,"rb+");
                if(!outputtext)
                    {
                    printf("\nError in opening output file %s\n",Option.argt);
                    fclose(sourceFile); // Option.argi
                    return 1;
                    }
                int c/* = EOF*/;
                long pos = ftell(outputtext);
                do
                    {
                    char buf[1024];
                    size_t /*int*/ i = 0;
                    while((c = fgetc(outputtext)) != EOF && c != '\n' && i + 1 < sizeof(buf))
                        {
                        buf[i++] = c;
                        }
                    buf[i] = '\0';
                    char * p = buf;
                    bool rewrite = false;
                    do
                        {
                        while(*p == ' ' || *p == (char)0xA0)
                            ++p;
                        if(*p)
                            {
                            int len = MWU(p);
                            if(len)
                                {
                                rewrite = true;
                                p += len;
                                }
                            while(*p && *p != ' ' && *p != (char)0xA0)
                                ++p;
                            }
                        else
                            break;
                        }
                    while(true);
                    long npos = ftell(outputtext);
                    if(rewrite)
                        {
                        fseek(outputtext,pos,SEEK_SET);
                        fwrite(buf,i,1,outputtext);
                        fseek(outputtext,npos,SEEK_SET);
                        }
                    pos = npos;
                    }
                while(c != EOF);
                }
            else
                {
                printf("\nError in opening multi-word-unit file %s\n",Option.arga);
                return 1;
                }

            }
        }
    if(targetFile)
        fclose(targetFile);
    return 0;
    }
