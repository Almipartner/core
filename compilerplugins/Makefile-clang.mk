#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

# Make sure variables in this Makefile do not conflict with other variables (e.g. from gbuild).

# Set to 1 if you need to debug the plugin).
CLANGDEBUG=

# Compile flags, you may occasionally want to override these:
ifeq ($(OS),WNT)
# See LLVM's cmake/modules/AddLLVM.cmake and LLVM build's
# tools/llvm-config/BuildVariables.inc:
# * Ignore "warning C4141: 'inline': used more than once" as emitted upon
#   "LLVM_ATTRIBUTE_ALWAYS_INLINE inline" in various LLVM include files.
# * Ignore "warning C4577: 'noexcept' used with no exception handling mode
#   specified; termination on exception is not guaranteed. Specify /EHsc".
CLANGCXXFLAGS=/nologo /D_HAS_EXCEPTIONS=0 /wd4141 /wd4577 /EHs-c- /GR-
ifeq ($(CLANGDEBUG),)
CLANGCXXFLAGS+=/O2 /Oi
else
CLANGCXXFLAGS+=/DEBUG
endif
else # WNT
CLANGCXXFLAGS=-Wall -Wextra -Wundef
ifeq ($(CLANGDEBUG),)
CLANGCXXFLAGS+=-O2
else
CLANGCXXFLAGS+=-g
endif
endif

# Whether to make plugins use one shared ASTRecursiveVisitor (plugins run faster).
# By default enabled, disable if you work on an affected plugin (re-generating takes time).
LO_CLANG_SHARED_PLUGINS=1
#TODO:
ifeq ($(OS),WNT)
LO_CLANG_SHARED_PLUGINS=
endif

# Whether to use precompiled headers for the analyzer too. Does not apply to compiling sources.
LO_CLANG_USE_ANALYZER_PCH=1

# The uninteresting rest.

include $(SRCDIR)/solenv/gbuild/gbuild.mk
include $(SRCDIR)/solenv/gbuild/Output.mk

CLANG_COMMA :=,

ifeq ($(OS),WNT)
CLANG_DL_EXT = .dll
CLANG_EXE_EXT = .exe
else
CLANG_DL_EXT = .so
CLANG_EXE_EXT =
endif

# Clang headers require these.
CLANGDEFS=-D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS
ifneq ($(OS),WNT)
CLANGDEFS += -fno-rtti
endif
# All include locations needed (using -isystem silences various warnings when
# including those files):
ifeq ($(OS),WNT)
CLANGINCLUDES=-I$(CLANGDIR)/include
else
CLANGINCLUDES=$(if $(filter /usr,$(CLANGDIR)),,-isystem $(CLANGDIR)/include)
endif

LLVMCONFIG=$(CLANGDIR)/bin/llvm-config

# Clang/LLVM libraries are intentionally not linked in, they are usually built as static libraries, which means the resulting
# plugin would be big (even though the clang binary already includes it all) and it'd be necessary to explicitly specify
# also all the dependency libraries.

CLANGINDIR=$(SRCDIR)/compilerplugins/clang
# Cannot use $(WORKDIR), the plugin should survive even 'make clean', otherwise the rebuilt
# plugin will cause cache misses with ccache.
CLANGOUTDIR=$(BUILDDIR)/compilerplugins/clang
CLANGOBJDIR=$(CLANGOUTDIR)/obj

ifdef LO_CLANG_SHARED_PLUGINS
CLANGCXXFLAGS+=-DLO_CLANG_SHARED_PLUGINS
endif

ifneq ($(CLANGDEBUG),)
ifeq ($(HAVE_GCC_SPLIT_DWARF),TRUE)
CLANGCXXFLAGS+=-gsplit-dwarf
endif
endif

QUIET=$(if $(verbose),,@)

ifneq ($(ENABLE_WERROR),)
ifeq ($(OS),WNT)
CLANGWERROR :=
#TODO: /WX
else
CLANGWERROR := -Werror
endif
endif

compilerplugins: compilerplugins-build

ifdef LO_CLANG_SHARED_PLUGINS
# The shared source, intentionally put first in the list because it takes the longest to build.
CLANGSRCOUTDIR=$(CLANGOUTDIR)/sharedvisitor/sharedvisitor.cxx
CLANGSRC+=$(CLANGSRCOUTDIR)
endif
# The list of source files, generated automatically (all files in clang/, but not subdirs).
CLANGSRCINDIR=$(foreach src,$(wildcard $(CLANGINDIR)/*.cxx), $(notdir $(src)))
CLANGSRC+=$(CLANGSRCINDIR)

# Remember the sources and if they have changed, force plugin relinking.
CLANGSRCCHANGED= \
    $(shell mkdir -p $(CLANGOUTDIR) ; \
            echo $(CLANGSRC) | sort > $(CLANGOUTDIR)/sources-new.txt; \
            if diff $(CLANGOUTDIR)/sources.txt $(CLANGOUTDIR)/sources-new.txt >/dev/null 2>/dev/null; then \
                echo 0; \
            else \
                mv $(CLANGOUTDIR)/sources-new.txt $(CLANGOUTDIR)/sources.txt; \
                echo 1; \
            fi; \
    )
ifeq ($(CLANGSRCCHANGED),1)
.PHONY: CLANGFORCE
CLANGFORCE:
$(CLANGOUTDIR)/plugin$(CLANG_DL_EXT): CLANGFORCE
endif
# Make the .so also explicitly depend on the sources list, to force update in case CLANGSRCCHANGED was e.g. during 'make clean'.
$(CLANGOUTDIR)/plugin$(CLANG_DL_EXT): $(CLANGOUTDIR)/sources.txt
$(CLANGOUTDIR)/sources.txt:
	touch $@

compilerplugins-build: $(CLANGOUTDIR) $(CLANGOBJDIR) $(CLANGOUTDIR)/plugin$(CLANG_DL_EXT)

compilerplugins-clean:
	rm -rf \
        $(CLANGOBJDIR) \
        $(CLANGOUTDIR)/clang-timestamp \
        $(CLANGOUTDIR)/plugin$(CLANG_DL_EXT) \
        $(CLANGOUTDIR)/sharedvisitor/*.plugininfo \
        $(CLANGOUTDIR)/sharedvisitor/clang.pch \
        $(CLANGOUTDIR)/sharedvisitor/sharedvisitor.{cxx,d,o} \
        $(CLANGOUTDIR)/sharedvisitor/{analyzer,generator}{$(CLANG_EXE_EXT),.d,.o} \
        $(CLANGOUTDIR)/sources-new.txt \
        $(CLANGOUTDIR)/sources.txt

$(CLANGOUTDIR):
	mkdir -p $(CLANGOUTDIR)

$(CLANGOBJDIR):
	mkdir -p $(CLANGOBJDIR)

CLANGOBJS=

ifeq ($(OS),WNT)

# clangbuildsrc cxxfile objfile dfile
define clangbuildsrc
$(2): $(1) $(SRCDIR)/compilerplugins/Makefile-clang.mk $(CLANGOUTDIR)/clang-timestamp
	$$(call gb_Output_announce,$(subst $(SRCDIR)/,,$(subst $(BUILDDIR)/,,$(1))),$(true),CXX,3)
	$(QUIET)$(COMPILER_PLUGINS_CXX) $(CLANGCXXFLAGS) $(CLANGWERROR) $(CLANGDEFS) \
        $(CLANGINCLUDES) /I$(BUILDDIR)/config_host /I$(CLANGINDIR) $(1) /MD \
        /c /Fo: $(2)

-include $(3) #TODO

$(CLANGOUTDIR)/plugin$(CLANG_DL_EXT): $(2)
$(CLANGOUTDIR)/plugin$(CLANG_DL_EXT): CLANGOBJS += $(2)
endef

else

# clangbuildsrc cxxfile ofile dfile
define clangbuildsrc
$(2): $(1) $(SRCDIR)/compilerplugins/Makefile-clang.mk $(CLANGOUTDIR)/clang-timestamp
	$$(call gb_Output_announce,$(subst $(SRCDIR)/,,$(subst $(BUILDDIR)/,,$(1))),$(true),CXX,3)
	$(QUIET)$(COMPILER_PLUGINS_CXX) $(CLANGCXXFLAGS) $(CLANGWERROR) $(CLANGDEFS) \
	$(CLANGINCLUDES) -I$(BUILDDIR)/config_host -I$(CLANGINDIR) $(1) \
	-fPIC -c -o $(2) -MMD -MT $(2) -MP -MF $(3)

-include $(3)

$(CLANGOUTDIR)/plugin$(CLANG_DL_EXT): $(2)
$(CLANGOUTDIR)/plugin$(CLANG_DL_EXT): CLANGOBJS += $(2)
endef

endif

$(foreach src, $(CLANGSRCOUTDIR), $(eval $(call clangbuildsrc,$(src),$(src:.cxx=.o),$(src:.cxx=.d))))
$(foreach src, $(CLANGSRCINDIR), $(eval $(call clangbuildsrc,$(CLANGINDIR)/$(src),$(CLANGOBJDIR)/$(src:.cxx=.o),$(CLANGOBJDIR)/$(src:.cxx=.d))))

$(CLANGOUTDIR)/plugin$(CLANG_DL_EXT): $(CLANGOBJS)
	$(call gb_Output_announce,$(subst $(BUILDDIR)/,,$@),$(true),LNK,4)
ifeq ($(OS),WNT)
	$(QUIET)$(COMPILER_PLUGINS_CXX) /LD $(CLANGOBJS) /Fe: $@ $(CLANGLIBDIR)/clang.lib \
        mincore.lib version.lib /link $(COMPILER_PLUGINS_CXX_LINKFLAGS)
else
	$(QUIET)$(COMPILER_PLUGINS_CXX) -shared $(CLANGOBJS) -o $@ \
		$(if $(filter MACOSX,$(OS)),-Wl$(CLANG_COMMA)-flat_namespace \
			-Wl$(CLANG_COMMA)-undefined -Wl$(CLANG_COMMA)suppress)
endif

# Clang most probably doesn't maintain binary compatibility, so rebuild when clang changes
# (either the binary can change if it's a local build, or config_clang.h will change if configure detects
# a new version of a newly installed system clang).
$(CLANGOUTDIR)/clang-timestamp: $(CLANGDIR)/bin/clang$(CLANG_EXE_EXT) $(BUILDDIR)/config_host/config_clang.h
	$(QUIET)touch $@


ifdef LO_CLANG_SHARED_PLUGINS
SHARED_SOURCES := $(shell grep -l "LO_CLANG_SHARED_PLUGINS" $(CLANGINDIR)/*.cxx)
SHARED_SOURCE_INFOS := $(foreach source,$(SHARED_SOURCES),$(patsubst $(CLANGINDIR)/%.cxx,$(CLANGOUTDIR)/sharedvisitor/%.plugininfo,$(source)))

$(CLANGOUTDIR)/sharedvisitor/%.plugininfo: $(CLANGINDIR)/%.cxx \
            $(CLANGOUTDIR)/sharedvisitor/analyzer$(CLANG_EXE_EXT) \
            $(CLANGOUTDIR)/sharedvisitor/clang.pch
	$(call gb_Output_announce,$(subst $(BUILDDIR)/,,$@),$(true),GEN,1)
	$(QUIET)$(ICECREAM_RUN) $(CLANGOUTDIR)/sharedvisitor/analyzer$(CLANG_EXE_EXT) \
        $(COMPILER_PLUGINS_TOOLING_ARGS:%=-arg=%) $< > $@

$(CLANGOUTDIR)/sharedvisitor/sharedvisitor.cxx: $(SHARED_SOURCE_INFOS) $(CLANGOUTDIR)/sharedvisitor/generator$(CLANG_EXE_EXT)
	$(call gb_Output_announce,$(subst $(BUILDDIR)/,,$@),$(true),GEN,1)
	$(QUIET)$(ICECREAM_RUN) $(CLANGOUTDIR)/sharedvisitor/generator$(CLANG_EXE_EXT) \
        $(COMPILER_PLUGINS_TOOLING_ARGS:%=-arg=%) $(SHARED_SOURCE_INFOS) > $@

CLANGTOOLLIBS = -lclangTooling -lclangDriver -lclangFrontend -lclangParse -lclangSema -lclangEdit -lclangAnalysis \
        -lclangAST -lclangLex -lclangSerialization -lclangBasic $(shell $(LLVMCONFIG) --ldflags --libs --system-libs)
# Path to the clang system headers (no idea if there's a better way to get it).
CLANGTOOLDEFS = -DCLANGSYSINCLUDE=$(shell $(LLVMCONFIG) --libdir)/clang/$(shell $(LLVMCONFIG) --version | sed 's/svn//')/include
# -std=c++11 is in line with the default value for COMPILER_PLUGINS_CXX in configure.ac:
CLANGSTDOPTION := $(or $(filter -std=%,$(COMPILER_PLUGINS_CXX)),-std=c++11)
CLANGTOOLDEFS += -DSTDOPTION=\"$(CLANGSTDOPTION)\"
ifneq ($(filter-out MACOSX WNT,$(OS)),)
ifneq ($(CLANGDIR),/usr)
# Help the generator find Clang shared libs, if Clang is built so and installed in a non-standard prefix.
CLANGTOOLLIBS += -Wl,--rpath,$(shell $(LLVMCONFIG) --libdir)
endif
endif

$(CLANGOUTDIR)/sharedvisitor/analyzer$(CLANG_EXE_EXT): $(CLANGINDIR)/sharedvisitor/analyzer.cxx \
        | $(CLANGOUTDIR)/sharedvisitor
	$(call gb_Output_announce,$(subst $(BUILDDIR)/,,$@),$(true),GEN,1)
	$(QUIET)$(COMPILER_PLUGINS_CXX) $(CLANGCXXFLAGS) $(CLANGWERROR) $(CLANGDEFS) $(CLANGTOOLDEFS) $(CLANGINCLUDES) \
        -DCLANGDIR=$(CLANGDIR) -I$(BUILDDIR)/config_host \
        -DLO_CLANG_USE_ANALYZER_PCH=$(LO_CLANG_USE_ANALYZER_PCH) \
        -c $< -o $(CLANGOUTDIR)/sharedvisitor/analyzer.o -MMD -MT $@ -MP \
        -MF $(CLANGOUTDIR)/sharedvisitor/analyzer.d
	$(QUIET)$(COMPILER_PLUGINS_CXX) $(CLANGCXXFLAGS) $(CLANGOUTDIR)/sharedvisitor/analyzer.o \
        -o $@ $(CLANGTOOLLIBS)

$(CLANGOUTDIR)/sharedvisitor/generator$(CLANG_EXE_EXT): $(CLANGINDIR)/sharedvisitor/generator.cxx \
        | $(CLANGOUTDIR)/sharedvisitor
	$(call gb_Output_announce,$(subst $(BUILDDIR)/,,$@),$(true),GEN,1)
	$(QUIET)$(COMPILER_PLUGINS_CXX) $(CLANGCXXFLAGS) $(CLANGWERROR) \
        -c $< -o $(CLANGOUTDIR)/sharedvisitor/generator.o -MMD -MT $@ -MP \
        -MF $(CLANGOUTDIR)/sharedvisitor/generator.d
	$(QUIET)$(COMPILER_PLUGINS_CXX) $(CLANGCXXFLAGS) $(CLANGOUTDIR)/sharedvisitor/generator.o \
        -o $@

$(CLANGOUTDIR)/sharedvisitor/analyzer$(CLANG_EXE_EXT): $(SRCDIR)/compilerplugins/Makefile-clang.mk $(CLANGOUTDIR)/clang-timestamp

$(CLANGOUTDIR)/sharedvisitor/generator$(CLANG_EXE_EXT): $(SRCDIR)/compilerplugins/Makefile-clang.mk

$(CLANGOUTDIR)/sharedvisitor:
	mkdir -p $(CLANGOUTDIR)/sharedvisitor

-include $(CLANGOUTDIR)/sharedvisitor/analyzer.d
-include $(CLANGOUTDIR)/sharedvisitor/generator.d
# TODO WNT version
endif

ifdef LO_CLANG_USE_ANALYZER_PCH

# these are from the invocation in analyzer.cxx
LO_CLANG_ANALYZER_PCH_CXXFLAGS := -I$(BUILDDIR)/config_host -I$(CLANGDIR)/include $(CLANGTOOLDEFS) $(CLANGSTDOPTION) \
    -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS

$(CLANGOUTDIR)/sharedvisitor/clang.pch: $(CLANGINDIR)/sharedvisitor/precompiled_clang.hxx \
        $(CLANGOUTDIR)/clang-timestamp \
        | $(CLANGOUTDIR)/sharedvisitor
	$(call gb_Output_announce,$(subst $(BUILDDIR)/,,$@),$(true),PCH,1)
	$(QUIET)$(CLANGDIR)/bin/clang -x c++-header $(LO_CLANG_ANALYZER_PCH_CXXFLAGS) $< -o $@
else
$(CLANGOUTDIR)/sharedvisitor/clang.pch:
	touch $@
endif

# vim: set noet sw=4 ts=4:
