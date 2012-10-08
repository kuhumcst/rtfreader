#rtfreader#
Read an RTF or flat text file and output the text, one line per sentence.
RTF formatted lay-out is used to spot 'sentences' that behave in special ways: headings and list items.
When flat text is given as input, an empty line is understood as a sentence delimiter, even if a full stop is absent. Also, certain symbols at the start of the line, if repeated on following lines, are interpreted as 'bullets' in a list.
The output can optionally be tokenised. 
The program can be informed about *abbreviations* and *multiple word units* by adding extra paramaters.

RTF is chosen as rich input format because
a) it is an open standard
b) it is practically always valid because it is machine-made (contrary to e.g. HTML)
c) it is fairly easy to parse, without the need to also read style sheets (contrary to XML)
d) there are tools ot there that can convert other formats to RTF while retaining essential lay out
