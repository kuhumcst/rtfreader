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
#include "charsource.h"
#include "flags.h"
#include "rtfstuff.h"
#include "txtstuff.h"
#include "commonstuff.h"
#include "segment.h"


/*

segment[Text]                                                       Recursively traverse RTF segment tree.
  |                                                                 Keep track of character properties (bold, italic,
  |                                                                 small caps, font size)
  |                                                                 Returns true if segment is a field. False otherwise.
  |                                                                 Sets begin and end offsets for candidate segments, 
  |                                                                 based on the interpretation of RTF tags such as \par
  |
  |                                                                 In case of flat text input: detect well-formed HTML-tags.
  |
  +--segmentdefault[Text]                                           Tries to spot sentence endings by looking at interpunction.
  |    |                                                                Knows of filenames, ellipsis ...
  +----+--doTheSegmentation[Text]
            |
            +--GetPut[Text]                                         Read segment. Convert rtf-tokens to their character equivalents.
            |     |                                                 Suppress optional hyphens.
            |     |
            +-----+------------PutHandlingLine                      Handle a line, detecting noise
                                 |
                                 +--PutHandlingWordWrap             Handle hyphens at end of line
                                 |    |
                                 +----+--Put                        Handle newlines
                                           |
                                           +--Put2                  Handle quotes, brackets and abbreviations
                                                |
                                                +--PutN             Repeated call of Put3 on whole buffer
                                                |    |
                                                +----+--Put3        Start new sentence if an abbreviation is followed by a capital 
                                                          |         letter in a new segment
                                                          |
                                                          +--regularizeWhiteSpace   Optionally translate 8-bit set characters to combinations 
                                                                    of 7-bit characters
*/
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "option.h"
#define skip_tekst 1
#define new_segment 2
#define character_property 4
#define hidden 8
#define reset 16
#define field 32
#define fonttbl 64
#define font 128
#define fcharset 256
#define eaten 512
#define negate 1024
#define cell 2048


/*



0 — ANSI
1 — Default
2 — Symbol
3 — Invalid
77 — Mac
128 — Shift Jis
129 — Hangul
130 — Johab
134 — GB2312
136 — Big5
161 — Greek
162 — Turkish
163 — Vietnamese
177 — Hebrew
178 — Arabic
179 — Arabic Traditional
180 — Arabic user
181 — Hebrew user
186 — Baltic
204 — Russian
222 — Thai
238 — Eastern European
254 — PC 437
255 — OEM 

2
128
 161
 162
 163
 177
 178
 186
 204
 238

charset	codepage	Windows/Mac name
 0	    1252	    ANSI
1	    0	        Default
2	    42	        Symbol
77	    10000	    Mac Roman
78	    10001	    Mac Shift Jis
79	    10003	    Mac Hangul
80	    10008	    Mac GB2312
81	    10002	    Mac Big5
82		            Mac Johab (old)
83	    10005	    Mac Hebrew
84	    10004	    Mac Arabic
85	    10006	    Mac Greek
86	    10081	    Mac Turkish
87	    10021	    Mac Thai
88	    10029	    Mac East Europe
89	    10007	    Mac Russian
128	    932	        Shift JIS
129	    949	        Hangul
130	    1361	    Johab
134	    936	        GB2312
136	    950	        Big5
 161	    1253	    Greek
 162	    1254	    Turkish
 163	    1258	    Vietnamese
 177	    1255	    Hebrew
 178	    1256	    Arabic 
179		            Arabic Traditional (old)
180		            Arabic user (old)
181		            Hebrew user (old)
 186	    1257	    Baltic
 204	    1251	    Russian
 222	    874	        Thai
 238	    1250	    Eastern European
254	    437	        PC 437
 255	    850	        OEM 

*/

struct codepage
    {
    int charset;
    int pos[224]; // 0x20 .. 0xFF
    };

codepage Codepages[]= 
{
  {0  ,{32,33,39,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,/*133 ellipsis*/8230,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255}},
  {2  ,{32,33,39,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,/*133 ellipsis*/8230,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,/*183 bullet 2022*/8226,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255}},
//{2  ,{32,33,39,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255}},
  {128,{32,33,39,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,63728,65377,65378,65379,65380,65381,65382,65383,65384,65385,65386,65387,65388,65389,65390,65391,65392,65393,65394,65395,65396,65397,65398,65399,65400,65401,65402,65403,65404,65405,65406,65407,65408,65409,65410,65411,65412,65413,65414,65415,65416,65417,65418,65419,65420,65421,65422,65423,65424,65425,65426,65427,65428,65429,65430,65431,65432,65433,65434,65435,65436,65437,65438,65439,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,63729,63730,63731}},
//{128,{32,33,39,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,63728,65377,65378,65379,65380,65381,65382,65383,65384,65385,65386,65387,65388,65389,65390,65391,65392,65393,65394,65395,65396,65397,65398,65399,65400,65401,65402,65403,65404,65405,65406,65407,65408,65409,65410,65411,65412,65413,65414,65415,65416,65417,65418,65419,65420,65421,65422,65423,65424,65425,65426,65427,65428,65429,65430,65431,65432,65433,65434,65435,65436,65437,65438,65439,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,63729,63730,63731}},
  {161,{32,33,39,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,/*133 ellipsis*/8230,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,901,902,163,164,165,166,167,168,169,63737,171,172,173,174,8213,176,177,178,179,900,181,182,183,904,905,906,187,908,189,910,911,912,913,914,915,916,917,918,919,920,921,922,923,924,925,926,927,928,929,63738,931,932,933,934,935,936,937,938,939,940,941,942,943,944,945,946,947,948,949,950,951,952,953,954,955,956,957,958,959,960,961,962,963,964,965,966,967,968,969,970,971,972,973,974,63739}},
  {162,{32,33,39,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,/*133 ellipsis*/8230,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,286,209,210,211,212,213,214,215,216,217,218,219,220,304,350,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,287,241,242,243,244,245,246,247,248,249,250,251,252,305,351,255}},
  {163,{32,33,39,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,/*133 ellipsis*/8230,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,258,196,197,198,199,200,201,202,203,768,205,206,207,272,209,777,211,212,416,214,215,216,217,218,219,220,431,771,223,224,225,226,259,228,229,230,231,232,233,234,235,769,237,238,239,273,241,803,243,244,417,246,247,248,249,250,251,252,432,8363,255}},
  {177,{32,33,39,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,/*133 ellipsis*/8230,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,8362,165,166,167,168,169,215,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,247,187,188,189,190,191,1456,1457,1458,1459,1460,1461,1462,1463,1464,1465,1466,1467,1468,1469,1470,1471,1472,1473,1474,1475,1520,1521,1522,1523,1524,63629,63630,63631,63632,63633,63634,63635,1488,1489,1490,1491,1492,1493,1494,1495,1496,1497,1498,1499,1500,1501,1502,1503,1504,1505,1506,1507,1508,1509,1510,1511,1512,1513,1514,63636,63637,8206,8207,63638}},
  {178,{32,33,39,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,1662,130,131,132,/*133 ellipsis*/8230,134,135,136,137,1657,139,140,1670,1688,1672,1711,145,146,147,148,149,150,151,1705,153,1681,155,156,8204,8205,1722,160,1548,162,163,164,165,166,167,168,169,1726,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,1563,187,188,189,190,1567,1729,1569,1570,1571,1572,1573,1574,1575,1576,1577,1578,1579,1580,1581,1582,1583,1584,1585,1586,1587,1588,1589,1590,215,1591,1592,1593,1594,1600,1601,1602,1603,224,1604,226,1605,1606,1607,1608,231,232,233,234,235,1609,1610,238,239,1611,1612,1613,1614,244,1615,1616,247,1617,249,1618,251,252,8206,8207,1746}},
  {186,{32,33,39,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,/*133 ellipsis*/8230,134,135,136,137,138,139,140,168,711,184,144,145,146,147,148,149,150,151,152,153,154,155,156,175,731,159,160,63740,162,163,164,63741,166,167,216,169,342,171,172,173,174,198,176,177,178,179,180,181,182,183,248,185,343,187,188,189,190,230,260,302,256,262,196,197,280,274,268,201,377,278,290,310,298,315,138,323,325,211,332,213,214,215,370,321,346,362,220,379,142,223,261,303,257,263,228,229,281,275,269,233,378,279,291,311,299,316,154,324,326,243,333,245,246,247,371,322,347,363,252,380,158,729}},
  {204,{32,33,39,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,1026,1027,130,1107,132,/*133 ellipsis*/8230,134,135,128,137,1033,139,1034,1036,1035,1039,1106,145,146,147,148,149,150,151,152,153,1113,155,1114,1116,1115,1119,160,1038,1118,1032,164,1168,166,167,1025,169,1028,171,172,173,174,1031,176,177,1030,1110,1169,181,182,183,1105,8470,1108,187,1112,1029,1109,1111,1040,1041,1042,1043,1044,1045,1046,1047,1048,1049,1050,1051,1052,1053,1054,1055,1056,1057,1058,1059,1060,1061,1062,1063,1064,1065,1066,1067,1068,1069,1070,1071,1072,1073,1074,1075,1076,1077,1078,1079,1080,1081,1082,1083,1084,1085,1086,1087,1088,1089,1090,1091,1092,1093,1094,1095,1096,1097,1098,1099,1100,1101,1102,1103}},
  {238,{32,33,39,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,/*133 ellipsis*/8230,134,135,136,137,138,139,346,356,142,377,144,145,146,147,148,149,150,151,152,153,154,155,347,357,158,378,160,711,728,321,164,260,166,167,168,169,350,171,172,173,174,379,176,177,731,322,180,181,182,183,184,261,351,187,317,733,318,380,340,193,194,258,196,313,262,199,268,201,280,203,282,205,206,270,272,323,327,211,212,336,214,215,344,366,218,368,220,221,354,223,341,225,226,259,228,314,263,231,269,233,281,235,283,237,238,271,273,324,328,243,244,337,246,247,345,367,250,369,252,253,355,729}}
};

rtfSource::rtfSource(STROEM * sourceFile,paragraph * outputtext):charSource(sourceFile,outputtext),staticEat(0)
    {
    firsttext.b.CR = 1;
    firsttext.b.LF = 0;
    firsttext.b.LS = 0;
    firsttext.b.SD = 1;
    lastfont = 0;
    fonttable = new MSfont[lastfont + 1];
    fonttable[lastfont].f = 0;
    fonttable[lastfont].pos = Codepages[0].pos;
    pASCIIfyFunction = ASCIIfyRTF;
    }

int rtfSource::Uchar(int f,int c)
    {
    if(c < 128)//32)
        return c;
    if(c > 255)
        return c;
    if(f < 0)
        return c;
    int i;
    for(i = lastfont;i >= 0;--i)
        if(fonttable[i].f == f)
            return fonttable[i].pos[c - 32];
    return c;
    }




typedef struct CharToken
  {
  const char * token;
  int ISOcode;
  } CharToken;

/* tokens that demarcate segments (can not be part of a segment): */
static const CharToken tokensThatDemarcateSegments[] =
  {{"tab",'\t'}     // Special characters, Symbol
  ,{"u8232",'\n'}
  ,{"par",-1}       // Special characters, Symbol
  ,{"line",-1}      // Special characters, Symbol
  ,{"intbl",-1}//??? Paragraph Formatting Properties
  ,{"footnote",-1}  // Footnotes, Destination
//  ,{"box",-1}//???
  ,{"'02",2} // (seen in a footnote)
  ,{NULL,0}
  };

static const CharToken tokensThatDemarcateSegmentsImmediately[] =
  {{"cell",-1}      // Special characters, Symbol
  ,{NULL,0}
  };

static const char * tokensThatTakeArgument[] =
  {"*"                  // 20090811
  ,"fonttbl"            // Font Table, Destination, part of header
  ,"filetbl"            // File table, Destination, part of header
  ,"colortbl"           // Color table, Destination, part of header
  ,"stylesheet"         // Style Sheet, Destination
  ,"stylerestrictions"  // Style restrictions group , part of header
  ,"listtables"         // List tables, part of header (mentioned, but not documented in RTF-doc)
  ,"listtable"          // List table, Destination, part of header
  ,"listoverridetable"  // List Override table, Destination, part of header
  ,"revtbl"             // Track changes, Destination, part of header
  ,"rsidtable"          // Revision Save ID table, part of header
  ,"mathprops"          // math properties group, part of header
  ,"generator"          // Generator area, Destination, part of header
  ,"pntext"             // Bullets and numbering,  Destination
  ,"pict"               // Pictures, Destination
  ,"object"             // Objects, Destination
  ,"template"           // Document Formatting Properties, Destination
  ,"author"             // Information Group, Destination
  ,"bkmkstart"          // Bookmarks, Destination
  ,"bkmkend"            // Bookmarks, Destination
  ,"title"              // Information Group, Destination
  ,"fldinst"            // Fields, Destination
  ,"operator"           // Information Group, Destination
  ,"xe"                 // Index Entries, Destination
//  ,"field"          // Fields, Destination
  ,"fldinst"            // Fields, Destination
  ,"viewkind"           // Document Format Properties, Value
  ,"viewscale"          // Document Format Properties, Value
  ,"viewzk"             // Document Format Properties, Value
  ,"company"            // Information Group, Destination
  ,"pntxta"             // Bullets and numbering, Destination
  ,"pntxtb"             // Bullets and numbering, Destination
  ,"background"         // Word 97 through Word 2002 RTF for Drawing Objects (Shapes), Destination
  ,"userprops"          // Information Group, Destination
  ,"shp"
  ,"password"
  ,"xmlnstbl"
  ,"wgrffmtfilter"
  ,NULL
  };

/* Problem: links are coded as fields. Consequence: line break after link. */
static const char * tokensThatAreField[] =
  {"field"          // Fields, Destination
  ,NULL
  };

static const char * tokensThatHide[] =
  {"v" /* hidden text */              // Font (Character) Formatting Properties Toggle
  ,"comment"
  ,NULL
  };

static const char * tokensThatReset[] =
  {"pard"           
  ,NULL
  };

/* Tokens that can be part of a segment: */

static const CharToken tokensThatArePartOfTheText[] =
  {{"lquote",0x91}
  ,{"rquote",0x92}
  ,{"ldblquote",0x93}
  ,{"rdblquote",0x94}
  ,{"bullet",0x2022}//0x95}
  ,{"bullet",0x95}
  ,{"endash",0x96}
  ,{"emdash",0x97}
  ,{"'95",149}
  ,{"~",' '} // Bart 20060303 \~ is Nonbreaking space
  ,{"\\",'\\'}
  ,{"{",'{'}
  ,{"}",'}'}
  ,{"_",'-'}  // Nonbreaking hyphen
  ,{NULL,0}
  };



typedef struct CharProp
  {
  const char * prop;
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

//static const int uninitialised = -3;
//static const int notSpecified = -2;
//static const int mixed = -1;

typedef struct CharPropN
  {
  const char * prop;
  int N;
  int def;
  } CharPropN;

static CharPropN characterPropertyWithNumericalParameter[] = /* strncmp + number*/
    {
         {"fs",notSpecified,24} // Font size in half-points (default is 24)
        ,{"f",mixed,0/*?*/} // Font number
        ,{"fcharset",notSpecified,0/*?*/} // Font number
        ,{"uc",notSpecified,0/*?*/} // number of bytes after unicode \u72346456 to skip: \uc2\u26085\'93\'fa skip \'93\'fa
        ,{NULL,0,0}
    };

char * rtfSource::parseRTFtoken(int level)
    {
    wint_t ch;
    int position = 0;
    bool perhapsUnicode = false;
    bool positive = true;
    int shortInt = 0;
    int index = 1;
    static char token[100];
    token[0] = '\\';
    while((ch = Getc(sourceFile)) != WEOF && ch != 26)
        {
#ifdef LOGGING
        log(level,0,false);
        tentative = ch;
#endif
        token[index++] = (char)ch;
        token[index] = 0;
        switch(ch)
            {
            case 'u':
                if(position == 0)
                  {
                  perhapsUnicode = true;
                  }
                break;
            case '-':
            case '*':
            case ':':
            case '_':
            case '|':
            case '~':
                if(position == 0) // detected symbol
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
                    Ungetc(ch,sourceFile);
                    token[--index] = 0;
                    return token;
                    }
                break;
            case '{':
            case '}':
                if(position == 0) // detected '{' or '}'
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
                    Ungetc(ch,sourceFile);
                    token[--index] = 0;
                    return token;
                    }
            case '\\':  // detected '\'
                if(position == 0)
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
                    Ungetc(ch,sourceFile); // Oops, start of next token
                    token[--index] = 0;
                    return token;
                    }
            case '\'':
                {
                if(position == 0)
                    {
#ifdef LOGGING
                    int ch;// TODO remove when done with logging
#endif
                    ///*unsigned*/ wchar_t kar;
                    //const CharToken * chartoken;
                    token[index++] =
#ifdef LOGGING
                    ch =
#endif
                    (char)Getc(sourceFile);
#ifdef LOGGING
                    log(level,ch,false);
#endif
                    token[index++] = (char)(ch = Getc(sourceFile));
#ifdef LOGGING
                    log(level,ch,false);
#endif
                    token[index] = 0;
                    return token;
                    }
                }
                // drop through
            case 0xA0:
            case 0x3000:
            case ' ':
                token[--index] = 0;
                return token;
            case '\n':
            case '\r':
#ifdef LOGGING
                log(level,0,false);
#endif
                if(index == 2)
                    strcpy(token,"\\par");
                else
                    token[--index] = 0;
                return token;
            default :
                if(perhapsUnicode)
                    {
                    if(ch == '-')
                        {
                        if(position == 1)
                            positive = false;  // \u-12345
                        else if(!positive)       // \u--
                            perhapsUnicode = false;
                        }

                    if(perhapsUnicode)
                        {
                        if('0' <= ch && ch <= '9')
                            {
                            shortInt = 10 * shortInt + (ch - '0');
                            if(  ( positive && shortInt > (0xffff / 10))
                              || (!positive && shortInt > (0x7fff / 10))
                              )
                                return token; // Cannot absorb more digits 
                                // after this one. (I assume that we should
                                // accept both signed and unsigned short
                                // integers.)
                            }
                        else if(shortInt != 0)    // \u248? The ? must be put back.
                            {
                            Ungetc(ch,sourceFile);
                            token[--index] = 0;
                            return token;
                            }
                        else
                            {
                            perhapsUnicode = false;
                            }
                        }
                    }
                ;
            }
        ++position;
        }
    if(  (ch == WEOF || ch == 26 /*20150916*/)
      && level != 0
      )
        {
        fprintf(stderr,"cstrtfreader: %d unbalanced braces in %s\n",level,Option.argi);
        exit(-1);
        }
#ifdef LOGGING
    tentative = 0;
#endif
    return NULL; // only reachable if level == 0
    }


int rtfSource::TranslateToken(const char * token,int f)
// Can return < 0 !
    {
    if(staticEat > 0)
        {
        --staticEat;
        return 0;
        }
    const CharToken * chartoken;
    for(chartoken = tokensThatArePartOfTheText
        ;chartoken->token
        ;++chartoken
        )
        {
        if(!strcmp(token,chartoken->token))
            {
            return chartoken->ISOcode;
            }
        }

    if(*token == '\'')
        {
        char buf[3];
        buf[0] = token[1];
        buf[1] = token[2];
        buf[2] = '\0';
        int ret = strtol(buf,NULL,16);
        ret = Uchar(f,ret);
        return ret;
        }
    if(*token == 'u')
        {
        if((token[1] == '-' || ('0' <= token[1] && token[1] <= '9')))
            {
            int ret = strtol(token+1,NULL,10);
            if(ret < 0) //Not necessary, wint_t is unsigned
                ret += 0x10000;
            staticEat = staticUc;
            return ret;
            }
        }
    return 0;
    }

static int hidingToken(const char * token)
    {
    const char ** p;
    for(p = tokensThatHide
        ;*p
        ;++p
        )
        {
        if(!strcmp(token,*p))
            {
            return hidden;
            }
        else if(  !strncmp(token,*p,strlen(*p))
               && !strcmp(token + strlen(*p),"0")
               )
            {
            return (negate | hidden);
            }
        }
    return 0;
    }

static int level;

void rtfSource::resetCharProp() // called after \pard
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
        if(PN->N == notSpecified || firsttext.EOL)
            PN->N = PN->def;
        else if(PN->N != PN->def)
            PN->N = mixed; // Different font sizes in same segment. Cannot use font size info to discern header.
        }
    }

int rtfSource::interpretToken(char * tok,int f)
    {
    const char ** p;
    const CharToken * chartoken;
    char token[80];
    if(*tok != '\\')
        {
        return -(int)(unsigned char)*tok;
        }
    if(staticEat > 0)// && tok[1] == '\'')
        {
        --staticEat;
        return eaten;
        }
    ++tok;// skip backslash
    if(*tok == '{' || *tok == '}' || *tok == '\\')
        return -*tok;
    strcpy(token,tok);
    size_t end = strlen(tok) - 1;
    while(  tok[end] == (char)' ' 
         || tok[end] == (char)(unsigned char)0xa0
         || tok[end] == (char)'\''
         || tok[end] == (char)'\n'
         || tok[end] == (char)'\r'
         )
        token[end--] = '\0';
    for(p = tokensThatTakeArgument
        ;*p
        ;++p
        )
        {
        if(!strcmp(token,*p))
            {
            if(!strcmp("fonttbl",*p))
                {
                return skip_tekst|fonttbl;
                }
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
            return field;
            }
        }


    int ret = hidingToken(token);
    if(ret)
        return ret;

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
    ret = TranslateToken(token,f);
    if(ret)
        {
        if(ret == 133 || ret == 8233 || ret == 8232) // NEL, Paragraph Separator (PS = 2029 hex) and Line Separator (LS = 2028 hex)
                           // see http://www.unicode.org/standard/reports/tr13/tr13-5.html
            return new_segment;
        return -ret;
        }

    for(chartoken = tokensThatDemarcateSegments
        ;chartoken->token
        ;++chartoken
        )
        {
        if(!strcmp(token,chartoken->token))
            {
            return new_segment;
            }
        }

    for(chartoken = tokensThatDemarcateSegmentsImmediately
        ;chartoken->token
        ;++chartoken
        )
        {
        if(!strcmp(token,chartoken->token))
            {
            return cell;
            }
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
                if(PN->N == notSpecified || firsttext.EOL)
                    PN->N = FS; // First font size spec in segment.
                else if(PN->N != FS)
                    PN->N = mixed; // Different font sizes in same segment. Cannot use font size info to discern header.
                return character_property;
                }
            }
        }
    return 0;
    }


bool rtfSource::writeparAfterCharacterPropertyChange(charprops oldprops,charprops newprops)
    {
    if(oldnl)
        {
        if(oldprops.fs != newprops.fs)
            {
            if(oldprops.fs != uninitialised && newprops.fs != mixed)
                {
                return true;
                }
            }
        if(oldprops.f != newprops.f)
            {
            if(oldprops.f != uninitialised && newprops.f != mixed)
                {
                return true;
                }
            }
        if(oldprops.i != newprops.i)
            {
            if(oldprops.i != uninitialised && newprops.i != mixed)
                {
                return true;
                }
            }
        if(oldprops.b != newprops.b)
            {
            if(oldprops.b != uninitialised && newprops.b != mixed)
                {
                return true;
                }
            }
        if(oldprops.scaps != newprops.scaps)
            {
            if(oldprops.scaps != uninitialised && newprops.scaps != mixed)
                {
                return true;
                }
            }
        }
    return false;
    }


void  rtfSource::doTheSegmentation(charprops CharProps,bool newlineAtEndOffset,bool forceEndOfSegmentAfter)
// Sets begin_offset to -1 and oldnl to false to signal that segment is 'done'.
// Otherwise, begin_offset and oldnl are not changed. 
    {
    if(-1L < begin_offset && begin_offset < end_offset)
        {
#ifdef LOGGING
            log(0,'|',false);
        char temp[256];
        sprintf(temp,"[%ld,%ld]i=%d,b=%d,scaps=%d,fs=%d,f=%d,nl=%d,oldnl=%d",begin_offset,end_offset,i,b,scaps,fs,f,nl,oldnl);
        logn(temp);
#endif
        int alreadyEaten = staticEat;
        static charprops old;



        if(oldnl)
            {
            if(Option.emptyline)
                outputtext->PutHandlingLine('\n',flgs);
            outputtext->PutHandlingLine('\n',flgs);
            }


        if(WriteParAfterHeadingOrField || writeparAfterCharacterPropertyChange(old,CharProps))
            {
            if(oldnl)
                {
                flgs.in_abbreviation = false; // 20150917
                flgs.expectCapitalizedWord = false; // 20150917
                if(Option.emptyline)
                    {
                    outputtext->PutHandlingLine('\n',flgs);
                    }
                outputtext->PutHandlingLine('\n',flgs);///
                outputtext->PutHandlingLine('\n',flgs);
                }
            WriteParAfterHeadingOrField = false;
            }


        wint_t ch = bulletStuff(CharProps.f);
        begin_offset = -1;

        if(forceEndOfSegmentAfter)
            {
            outputtext->PutHandlingLine('\n',flgs);
            outputtext->PutHandlingLine('\n',flgs);
            }
        else if(newlineAtEndOffset)
            {



                outputtext->PutHandlingLine('\n',flgs);
            }
        else
            {
            switch(ch)
                {
                case ';':
                case '?':
//                case '.':
                case '!':
#if COLONISSENTENCEDELIMITER
                case ':':
#endif
              //  case '\n':
                    outputtext->PutHandlingLine('\n',flgs);
                    break;



                default:
                    {
                    }
                }
            }
        old = CharProps;
        oldnl = false;
        flgs.bbullet = false;
        staticEat = alreadyEaten;
        }
    }


void rtfSource::segmentdefault(int ch,int sstatus,charprops CharProps)
    {
    if(sstatus & fonttbl)
        {
        // extract fonttables
        }
    else if(  !(sstatus & (skip_tekst|hidden))
           && ch != '\n' && ch != '\r'
           )
        {
        bool sentenceEnd = checkSentenceStartDueToBullet(ch);
        firsttext.EOL = 0;
        end_offset = Ftell(sourceFile);// Bart 20030512. Otherwise the sentence delimiter isn't part of the segment
        if(  sentenceEnd
          || (end_offset - begin_offset > MAXSEGMENTLENGTH && isSpace(ch)) // check for buffer overflow
          )
            {
            doTheSegmentation(CharProps,false,false);
            firsttext.b.SD = 1;
            }
        if(!isSpace(ch))
            {
            firsttext.b.LS = 0;
            }
        }
    }


bool rtfSource::segment(int level
                        ,int sstatus
                        ,bool PrevIsField  // True if previous sibling block contains a \field
                        ,charprops CharProps
                        )
    {
    ::level = level;
    wint_t ch; // 20090528 int --> wint_t
    curr_pos = Ftell(sourceFile);
    bool prevIsField = false;
    bool isField = false;
    int fcharsetProp = 0;
    while((ch = Getc(sourceFile)) != WEOF && ch != 26)
        {
        switch(ch)
            {
            case '{':
                {
                flgs.in_fileName = false;
#ifdef LOGGING
                log(level,ch,true);
                if(sstatus & (skip_tekst|hidden))
                    log(0,'@',false);
#endif
                prevIsField = segment(level+1,sstatus,prevIsField,CharProps);
                staticUc = CharProps.uc; // reset
                staticEat = 0; // reset

#ifdef LOGGING
                if(sstatus & (skip_tekst|hidden))
                    log(0,'@',false);
                log(level,'}',true);
#endif
                break;
                }
            case '}':
                {
                flgs.in_fileName = false;
                firsttext.b.SD = 1;
                doTheSegmentation(CharProps,false,false);
                return isField;
                }
            case '\\':
                {
#ifdef LOGGING
                log(level,ch,false);
#endif
                char * token = parseRTFtoken(level);
                if(token)
                    {
#ifdef LOGGING
                    char temp[256];
                    sprintf(temp,"A fs %d",fs);
                    logn(temp);
#endif
                    characterProperty[0].status = CharProps.i;
                    characterProperty[2].status = CharProps.b;
                    characterProperty[4].status = CharProps.scaps;
                    characterPropertyWithNumericalParameter[0].N = CharProps.fs;
                    characterPropertyWithNumericalParameter[1].N = CharProps.f;
                    characterPropertyWithNumericalParameter[3].N = CharProps.uc;
                    int interpretation = interpretToken(token,CharProps.f);
                    if(interpretation == eaten)
                        break;
                    CharProps.fs = characterPropertyWithNumericalParameter[0].N;
                    CharProps.f = characterPropertyWithNumericalParameter[1].N;
                    CharProps.uc = characterPropertyWithNumericalParameter[3].N;
                    staticUc = CharProps.uc;
                    CharProps.i = characterProperty[0].status;
                    CharProps.b = characterProperty[2].status;
                    CharProps.scaps = characterProperty[4].status;
#ifdef LOGGING
                    sprintf(temp,"B fs %d",fs);
                    logn(temp);
#endif
                    if(sstatus & fonttbl)
                        {
                        fcharsetProp = characterPropertyWithNumericalParameter[2].N;

                        if(CharProps.f >= 0 && fcharsetProp >= 0)
                            {
                            int ii;
                            for(ii = lastfont;ii >= 0;--ii)
                                {
                                if(fonttable[ii].f == CharProps.f)
                                    break;
                                }
                            if(  (ii < 0) // font not found
                              || (CharProps.f == 0) // or going to overrule default (ANSI) font
                              ) 
                                {
                                if(ii < 0)
                                    ++lastfont;
                                MSfont * newfonttable = new MSfont[lastfont + 1];
                                if(ii < 0)
                                    memcpy(newfonttable+1,fonttable,lastfont * sizeof(MSfont));
                                newfonttable[0].f = CharProps.f;
                                size_t j;
                                for(j = 0;j < sizeof(Codepages)/sizeof(Codepages[0]);++j)
                                    {
                                    if(Codepages[j].charset == fcharsetProp)
                                        {
                                        newfonttable[0].pos = Codepages[j].pos;
                                        break;
                                        }
                                    }
                                if(j == sizeof(Codepages)/sizeof(Codepages[0]))
                                    newfonttable[0].pos = Codepages[0].pos; // hack: assume charset0
                                delete [] fonttable;
                                fonttable = newfonttable;
                                }

                            characterPropertyWithNumericalParameter[2].N = -1;
                            }

                        }
                    else if(interpretation < 0) // just a character (sign inverted)
                        {
                        staticUc = CharProps.uc;
                        segmentdefault(-interpretation,sstatus,CharProps);
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
                            WriteParAfterHeadingOrField = true;
                            }

                        if(interpretation == field)
                            {
                            // It is perhaps not a good idea to enforce new lines after each field.
                            // Many fields (e.g., hyperlinks) are in-line.
                            isField = true;
                            }
                        else if(interpretation & new_segment) // 20130301 == -> &
                            {
                            if(interpretation & reset) // \pard
                                {
                                sstatus &= ~hidden;
                                characterProperty[0].status = CharProps.i;
                                characterProperty[2].status = CharProps.b;
                                characterProperty[4].status = CharProps.scaps;
                                characterPropertyWithNumericalParameter[0].N = CharProps.fs;
                                characterPropertyWithNumericalParameter[1].N = CharProps.f;
                                resetCharProp();
                                CharProps.fs = characterPropertyWithNumericalParameter[0].N;
                                CharProps.f = characterPropertyWithNumericalParameter[1].N;
                                CharProps.i = characterProperty[0].status;
                                CharProps.b = characterProperty[2].status;
                                CharProps.scaps = characterProperty[4].status;
                                }

                            if(begin_offset < 0)
                                {
                                oldnl = true; // found \par between segments, eg
                                // {\b\fs47\cf1 4}\par {\b\f1\fs22\cf2 The idea\par }
                                // Also happens if two \par follow each other
                                // Or after a \field!
                                }
                            else
                                {
                                doTheSegmentation(CharProps,true,false);
                                }
                            firsttext.b.SD = 1;
                            firsttext.b.LS = 1;
                            }
                        else if(interpretation & cell)
                            {
                            if(begin_offset < 0)
                                {
                                oldnl = true;
                                }
                            else
                                {
                                doTheSegmentation(CharProps,true,true);
                                }
                            firsttext.b.SD = 1;
                            firsttext.b.LS = 1;
                            }
                        else if(interpretation & reset) // \pard
                            {
                            sstatus &= ~hidden;
                            characterProperty[0].status = CharProps.i;
                            characterProperty[2].status = CharProps.b;
                            characterProperty[4].status = CharProps.scaps;
                            characterPropertyWithNumericalParameter[0].N = CharProps.fs;
                            characterPropertyWithNumericalParameter[1].N = CharProps.f;
                            resetCharProp();
                            CharProps.fs = characterPropertyWithNumericalParameter[0].N;
                            CharProps.f = characterPropertyWithNumericalParameter[1].N;
                            CharProps.i = characterProperty[0].status;
                            CharProps.b = characterProperty[2].status;
                            CharProps.scaps = characterProperty[4].status;
                            }
                        else if(interpretation & negate)
                            {
                            if(interpretation & hidden)
                                sstatus &= ~hidden;
                            }
                        else
                            {
                            sstatus |= interpretation;
                            }
                        }
                    }
                break;
                }
            default:
                {
#ifdef LOGGING
                log(level,ch,false);
#endif
                if(staticEat > 0)
                    --staticEat;
                else
                    segmentdefault(ch,sstatus,CharProps);
                }
            }
        curr_pos = Ftell(sourceFile);
        }

    outputtext->PutHandlingLine('\n',flgs); // 20090811 Flush last line

    if(  (ch == WEOF || ch == 26 /*20150916*/)
      && level != 0
      )
        {
        fprintf(stderr,"cstrtfreader: %d unbalanced braces in %s\n",level,Option.argi);
        exit(-1);
        }
    return isField;
    }



bool rtfSource::escapeSequence(int ch,int f,bool & okToWrite)
    {
    if(ch == '\\')
        {
        char * token = parseRTFtoken(1);
        assert(token != 0);// because level == 1
        int interpretation = TranslateToken(token+1,f);
        if(interpretation)
            {
            outputtext->PutHandlingLine((wint_t)interpretation,flgs);
            }
        else
            {
            int hiding = hidingToken(token+1);
            if(hiding)
                {
                if(hiding & negate)
                    okToWrite = true;
                else
                    okToWrite = false;
                }
            }
        return true;
        }
    return false;
    }


