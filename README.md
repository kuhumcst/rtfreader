#rtfreader#
Read an RTF or flat text file and output the text, one line per sentence.
RTF formatted lay-out is used to spot 'sentences' that behave in special ways: headings and list items.
When flat text is given as input, an empty line is understood as a sentence delimiter, even if a full stop is absent. Also, certain symbols at the start of the line, if repeated on following lines, are interpreted as 'bullets' in a list.
The output can optionally be tokenised. 
The program can be informed about *abbreviations* and *multiple word units* by adding extra paramaters.

RTF is chosen as rich input format because

1. it is an open standard
2. it is practically always valid because it is machine-made (contrary to e.g. HTML)
3. it is fairly easy to parse, without the need to also read style sheets (contrary to XML)
4. there are tools out there that can convert other formats to RTF while retaining essential lay out

**Online availability**

RTFreader is used as segmenter and tokeniser at CST's website
(https://cst.dk/tools/index.php?lang=en)
or
http://cst.dk:8080/tools/).

**Installing RTFreader on your computer**

RTFreader depends on some other repositories. To clone all necessary repositories and build RTFreader, look at makertfreader.bash in the doc folder. If you are in Linux, you can probably just run this script file.