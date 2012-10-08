PNAME=rtfreader
VERSION=2
MINOR_NUMBER=1

BIN=~/bin/
BIN=

LIB=

PNAMESTATIC=$(BIN)$(PNAME)s
PNAMEDYNAMIC=$(BIN)$(PNAME)
PNAMEDYNAMICLIB=$(BIN)$(PNAME)dl
LINKERNAME=$(LIB)lib$(PNAME).so
SONAME=$(LINKERNAME).$(VERSION)
REALNAME=$(SONAME).$(MINOR_NUMBER)

SRCDIR     = .
LETTERFUNCDIR    = ../../letterfunc/src
SGMLDIR    = ../../parsesgml/src
GCCINC += -I$(SRCDIR) -I$(LETTERFUNCDIR) -I$(SGMLDIR)

CC=/usr/bin/g++ -O3 -Wall $(GCCINC) -pedantic -DNDEBUG -DNOSGMLCALLBACKS

# -fPIC or -fpic: enable 'position independent code' generation. Necessary for shared libs
# -fpic may generate smaller and faster code, but will have platform-dependent limitations
PIC=-fPIC
#PIC=

CCLINKSTATIC=/usr/bin/g++ -static
CCLINKDYNAMIC=/usr/bin/g++ 
CCCREATELIB=/usr/bin/g++ -shared -Wl,-soname,$(SONAME)

#DEBUG=-g
DEBUG=

GCCLINK=-L/usr/lib64 -lstdc++

RM=rm -f

PROJECTSRC=\
	abbrev.cpp\
	argopt.cpp\
	charconv.cpp\
	commonstuff.cpp\
	data.cpp\
	$(LETTERFUNCDIR)/letter.cpp\
	$(LETTERFUNCDIR)/letterfunc.cpp\
	main.cpp\
	mwu.cpp\
	ocrtidy.cpp\
	option.cpp\
	$(SGMLDIR)/parsesgml.cpp\
	readlist.cpp\
	readoptf.cpp\
	rtfstuff.cpp\
	txtstuff.cpp\
	unicode.cpp

CSTPROJECTSRC=\
	rtfreader.cpp

PROJECTOBJS=\
	abbrev.o\
	argopt.o\
	charconv.o\
	commonstuff.o\
	data.o\
	letter.o\
	letterfunc.o\
	main.o\
	mwu.o\
	ocrtidy.o\
	option.o\
	parsesgml.o\
	readlist.o\
	readoptf.o\
	rtfstuff.o\
	txtstuff.o\
	unicode.o

CSTPROJECTOBJS=\
	rtfreader.o

#                               #
#   Dependencies start here.    #
#                               #


$(PNAMEDYNAMIC): $(CSTPROJECTOBJS) $(PROJECTOBJS)
	$(CCLINKDYNAMIC) $(CSTPROJECTOBJS) $(PROJECTOBJS) -o $@ $(GCCLINK)

$(PNAMESTATIC): $(CSTPROJECTOBJS) $(PROJECTOBJS)
	$(CCLINKSTATIC) $(CSTPROJECTOBJS) $(PROJECTOBJS) -o $@ $(GCCLINK)

$(REALNAME) : $(PROJECTOBJS)
	$(CCCREATELIB) -o $@ $(PROJECTOBJS)
	ln -sf $(REALNAME) $(SONAME)
	ln -sf $(SONAME) $(LINKERNAME)

$(PNAMEDYNAMICLIB): $(REALNAME) $(CSTPROJECTOBJS)
	$(CCLINKDYNAMIC) $(CSTPROJECTOBJS) -L. -l$(PNAME) -o $@ $(GCCLINK)


all: $(PNAMESTATIC) $(PNAMEDYNAMIC) $(REALNAME) $(PNAMEDYNAMICLIB)

.cpp.o:
	$(CC) $(PIC) $(DEBUG) $(GCCINC) -c $(CSTPROJECTSRC) $(PROJECTSRC)

#.cpp.o:
#	$(CC) $(PIC) $(DEBUG) $(GCCINC) -c $?

clean:
	$(RM) *.o
	$(RM) $(REALNAME)
	$(RM) $(SONAME)
	$(RM) $(LINKERNAME)

