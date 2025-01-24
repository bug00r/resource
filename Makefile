_CFLAGS:=$(CFLAGS)
CFLAGS:=$(_CFLAGS)
_LDFLAGS:=$(LDFLAGS)
LDFLAGS:=$(_LDFLAGS)

UNAME_S := $(shell uname -s)

ARFLAGS?=rcs
PATHSEP?=/
BUILDROOT?=build

BUILDDIR?=$(BUILDROOT)$(PATHSEP)$(CC)
BUILDPATH?=$(BUILDDIR)$(PATHSEP)

ifndef PREFIX
	INSTALL_ROOT=$(BUILDPATH)
else
	INSTALL_ROOT=$(PREFIX)$(PATHSEP)
	ifeq ($(INSTALL_ROOT),/)
	INSTALL_ROOT=$(BUILDPATH)
	endif
endif

ifdef DEBUG
	CFLAGS+=-ggdb
	ifeq ($(DEBUG),)
	CFLAGS+=-Ddebug=1
	else 
	CFLAGS+=-Ddebug=$(DEBUG)
	endif
endif

ifeq ($(M32),1)
	CFLAGS+=-m32
	BIT_SUFFIX+=32
endif

CFLAGS+=-std=c11 -Wpedantic -pedantic-errors -Wall -Wextra

BIT_SUFFIX=

ifeq ($(M32),1)
	CFLAGS+=-m32
	BIT_SUFFIX+=32
endif

CFLAGS+=-std=c11 -DIN_LIBXML -DLIBXML_STATIC -Wpedantic -Wall -Wextra -Wno-pointer-sign
## https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94391
## not working on linux CFLAGS+=-fpie -fno-direct-access-extern-object
_SRC_FILES+=resource resource_sqlite

LIBNAME:=resource
LIBEXT:=a
LIB:=lib$(LIBNAME).$(LIBEXT)
LIB_TARGET:=$(BUILDPATH)$(LIB)

OBJS+=$(patsubst %,$(BUILDPATH)%,$(patsubst %,%.o,$(_SRC_FILES)))

CFLAGS+=-I./src -I/c/dev/include
LDFLAGS+=-L/c/dev/lib$(BIT_SUFFIX) -L./$(BUILDPATH)


THIRD_PARTY_LIBS=sqlite3 exslt xslt xml2 archive crypto nettle lzma z lz4 bz2 zstd
REGEX_LIBS=pcre2_utils pcre2-8
#this c flags is used by regex lib
CFLAGS+=-DPCRE2_STATIC

ifeq ($(OS), Windows_NT)
	LDFLAGS+=-static
	OS_LIBS=kernel32 user32 gdi32 winspool comdlg32 advapi32 shell32 uuid ole32 oleaut32 comctl32 ws2_32 iconv
endif

ifeq ($(UNAME_S), Linux) 
	# nothing
endif

USED_LIBS=$(patsubst %,-l%, utils $(REGEX_LIBS) $(THIRD_PARTY_LIBS) $(OS_LIBS)  )

LDFLAGS+=$(USED_LIBS)

#wc -c < filename => if needed for after compression size of bytes
RES=zip_resource
RES_O=$(RES).o
RES_O_PATH=$(BUILDPATH)$(RES_O)
RES_7Z=$(RES).7z
RES_FILES_PATTERN=./data/*
ZIP=7z
ZIP_ARGS=a -t7z
ZIP_CMD=$(ZIP) $(ZIP_ARGS)

all: mkbuilddir $(LIB_TARGET)

$(LIB_TARGET): $(_SRC_FILES)
	$(AR) $(ARFLAGS) $(LIB_TARGET) $(OBJS)

$(_SRC_FILES):
	$(CC) $(CFLAGS) -c src/$@.c -o $(BUILDPATH)$@.o 

test_resource: mkbuilddir mkzip addzip $(LIB_TARGET)
	$(CC) $(CFLAGS) ./test/$@.c ./src/resource.c $(RES_O_PATH) -o $(BUILDPATH)$@.exe $(LDFLAGS)
	$(BUILDPATH)$@.exe

test_resource_sqlite: mkbuilddir mkdb $(LIB_TARGET)
	$(CC) $(CFLAGS) ./test/$@.c ./src/resource_sqlite.c $(BUILDPATH)/res_test_embed.o -o $(BUILDPATH)$@.exe $(LDFLAGS)
	cd $(BUILDPATH) ; ./$@.exe

.PHONY: clean mkbuilddir mkzip addzip test 

test: test_resource test_resource_sqlite

addzip:
	cd $(BUILDPATH); \
	ld -r -b binary $(RES_7Z) -o $(RES_O)

mkzip:
	-$(ZIP_CMD) $(BUILDPATH)$(RES_7Z) $(RES_FILES_PATTERN)

mkdb:
	cd test ; rm -f $(BUILDPATH)res_test.db ; cat res_test_init.sqlite | sqlite3 ; mv res_test.db ../$(BUILDPATH)
	cd test ; rm -f $(BUILDPATH)res_test_embed.db ; cat res_test_init_embed.sqlite | sqlite3 ; mv res_test_embed.db ../$(BUILDPATH)
	cd $(BUILDPATH); ld -r -b binary res_test_embed.db -o res_test_embed.o

mkbuilddir:
	mkdir -p $(BUILDDIR)
	
clean:
	-rm -dr $(BUILDROOT)

install:
	mkdir -p $(INSTALL_ROOT)include
	mkdir -p $(INSTALL_ROOT)lib$(BIT_SUFFIX)
	cp ./src/resource.h $(INSTALL_ROOT)include/resource.h
	cp ./src/resource.h $(INSTALL_ROOT)include/resource_sqlite.h
	cp $(BUILDPATH)$(LIB) $(INSTALL_ROOT)lib$(BIT_SUFFIX)/$(LIB)
