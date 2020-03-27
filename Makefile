#
# 'make'        build executable file
# 'make clean'  removes all .o and executable files
# 'make cleandep' deletes all .d files
#

### Define according to folder structure ###
INCDIR=include#	headers folder
SRCDIR=src#		source folder/s and file extension
BUILDDIR=build#	object files folder
DEPDIR=build#	dependencies folder
BINDIR=bin#		binaries folder

#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .c of all words in the macro SRCS
# with the .o suffix
#
# define the C source files
SRCS=$(shell find $(SRCDIR) -type f -name "*.cpp")
# define the C object files 
OBJS=$(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SRCS:.cpp=.o))
# define the auto-generated dependency files
DEPS=$(patsubst $(SRCDIR)/%,$(DEPDIR)/%,$(SRCS:.cpp=.d))

### Define compiling process ###
CXX=g++ # C compiler
INC=-I $(INCDIR)
CXXFLAGS=-Wall -Werror -Wno-unused -std=c++17 # Compile-time flags
LDLIBS=

# define the main files
TARGET=$(BINDIR)/web_sstt

$(TARGET): $(OBJS)
	echo $(SRCS) $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(INC) $(LDLIBS) -o $(TARGET)

# rule for .o files
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INC) $(LDLIBS) -c $< -o $@

# rule to generate a dep file by using the C preprocessor
# (see man cpp for details on the -MM and -MT options)
$(DEPDIR)/%.d: $(SRCDIR)/%.cpp
	@mkdir build 2>/dev/null
	($(CXX) $(CXXFLAGS) $(INC) $(LDLIBS) $< -MM -MT $(@:.d=.o); echo '\t$(CXX) $(CXXFLAGS) $(INC) $(LDLIBS) -c $< -o $(@:.d=.o)';) >$@

# Include dependencies files
-include $(DEPS)

.PHONY: clean cleandep

clean:
	rm -f $(OBJS) $(TARGET)

cleandep:
	rm -f $(DEPS)

cleandoc:
	rm -f doc/out/header*


dockerim: Dockerfile
	docker images 2>&1 | grep -qe ubuntu/server || docker build -t ubuntu/server:r1 .

run-nox: $(TARGET)
	docker run -it --rm \
		-h server \
		-p 8000:8000 \
		-v /tmp:/tmp/tmp:rw \
		-v `pwd`/bin:/home/alumno/ --name server ubuntu/server:r1

transfer: $(TARGET)
	scp -r bin src include Makefile alumno@192.168.56.104:/home/alumno


DOCFILE=Memoria.pdf

$(DOCFILE): doc/* src/* include/*
	pdflatex -halt-on-error -output-directory doc/out/ doc/header.tex
	cp doc/out/header.pdf $(DOCFILE)

docs: $(DOCFILE)

ZIPFILE=49277628J_Práctica_SSTT_1920.zip

$(ZIPFILE): $(DOCFILE)
	rm -f $(TARGET) $(BINDIR)/webserver.log $(BINDIR)/.bash_history $(ZIPFILE)
	tar cavf "49277628J_Práctica_SSTT_1920.zip" include src bin tests/capture-global.pcapng tests/pipelining.sh Makefile $(DOCFILE)

tar: $(ZIPFILE)

