ROOT := ../..
OUTDIR := $(ROOT)/bin
OBJDIR := $(ROOT)/obj/nsis

RM := del
MOVE := move

NSIS := "$(PROGRAMFILES)/NSIS/makensis.exe"
NSIS_FLAGS := /nocd

TARGETS := \
	setup.exe \

SOURCES := ${addprefix $(OUTDIR)/, \
	Beatrix.dll \
}

all : $(TARGETS)

clean :
	-$(RM) ${addprefix $(OUTDIR)/,$(TARGETS)}

setup.exe : setup.nsi $(SOURCES)
	$(NSIS) $(NSIS_FLAGS) "/XOutFile $(OUTDIR)/$@" $<