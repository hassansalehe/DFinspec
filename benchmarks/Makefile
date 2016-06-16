MAINDIR = $(PWD)
BINDIR=$(MAINDIR)/../bin
INCDIR = $(MAINDIR)/include
INCSTL = $(MAINDIR)/include/stm_stl
INCATOMIC = $(MAINDIR)/include/atomic_ops
SRCDIR = $(MAINDIR)/src
TMMISC = $(MAINDIR)/tmmisc
LIBDIR = $(MAINDIR)/lib
OBJDIR = $(MAINDIR)/obj

# library
# -------------------------------------------
TM = stm
TMLIB = $(LIBDIR)/lib$(TM).a
ADF = adf
ADFLIB = $(LIBDIR)/libadf.a

CXX = clang++
AR = ar

# flags
# -------------------------------------------

CXXFLAGS = -g -c -Wall -O0 -Wno-unused-but-set-variable -I$(SRCDIR) -I$(INCDIR) -I$(INCSTL)
LDFLAGS =

CXXFLAGS_ADF = -std=c++11 -pthread -I$(INCATOMIC) -I$(TMMISC)
LDFLAGS_ADF = -L$(LIBDIR) -litm -ladf -lpthread

LDFLAGS_DWARFS = -lsfftw

# applications
# -------------------------------------------

APPDIR= $(MAINDIR)/apps
DWARFDIR= $(MAINDIR)/dwarfs


APPS_COMMON = 	$(APPDIR)/buffer/buffer         \
		$(APPDIR)/game/game                     \
		$(APPDIR)/lee/lee

ADFAPPS = $(addsuffix _adf, $(APPS_COMMON))

# dwarfs
# -------------------------------------------

DWARFS_COMMON = $(DWARFDIR)/sparse_algebra/sparse_algebra                          \
		$(DWARFDIR)/branch_bound/branch_bound                              \
		$(DWARFDIR)/dense_algebra/dense_algebra                            \
		$(DWARFDIR)/dense_algebra/matrix_multiplication/dense_algebra                            \
		$(DWARFDIR)/finite_state_machine/finite_state_machine              \
		$(DWARFDIR)/graph_models/graph_models                              \
		$(DWARFDIR)/map_reduce/map_reduce                                  \
		$(DWARFDIR)/spectral_methods/spectral_methods                      \
		$(DWARFDIR)/structured_grid/structured_grid                        \
		$(DWARFDIR)/combinatorial_logic/combinatorial_logic                \
		$(DWARFDIR)/../../motivating_bank_example				\
		$(DWARFDIR)/unstructured_grid/unstructured_grid

ADFDWARFS = $(addsuffix _adf, $(DWARFS_COMMON))

LIBOBJS=$(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/adf/%.o, $(wildcard $(SRCDIR)/*.cpp))
TMMISCOBJS=$(patsubst %.cpp,%.o,$(wildcard $(TMMISC)/*.cpp))

.PHONY: adf
all: adf

$(ADFLIB):     $(LIBOBJS)
	$(AR) rcs $@ $^

$(OBJDIR)/adf/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

.cpp.o:
	$(APPNAME)
	$(CXX) -Xclang -load -Xclang $(BINDIR)/libADFInstrumentationPass.so $(CXXFLAGS) $< -o $@

# --------------------------------------------------------------------------------
# Rules to compile library
# --------------------------------------------------------------------------------

DEFS=

ifndef LOCK
  DEFS+= -DADF_STM
endif

ifdef OPTIMAL
  DEFS+= -DOPTIMAL
endif

ifdef GET_STATS
  DEFS+= -DGET_TIMING_STAT
endif

ifdef RESERVE
  DEFS+= -DRESERVE_TASK
endif


ifdef DEBUG
  CXXFLAGS=$(CXXFLAGS) -g
endif

.PHONY: intro_adf
intro_adf:
	@ printf "\nCompiling ADF library\n====================================\n\n";

.PHONY: adf
adf:         adf_lib adf_apps

.PHONY: adf_lib
adf_lib:         adf_lib_test

.PHONY: adf_lib_test
adf_lib_test:    CXXFLAGS+= $(DEFS) -DADF $(CXXFLAGS_ADF)
adf_lib_test:    LDFLAGS+= $(LDFLAGS_ADF)
adf_lib_test:    intro_adf $(ADFLIB)

# --------------------------------------------------------------------------------
# Rules to compile applications
# --------------------------------------------------------------------------------

APPS= $(ADFDWARFS)

$(APPS): APPNAME=@ printf "\n$(subst .o,,$(lastword $(subst /, ,$@)))\n-------------------\n\n"
$(APPS): %:%.o
	$(CXX) -o $@ $< $(BINDIR)/Callbacks.o $(BINDIR)/Logger.o $(LDFLAGS)

.PHONY: adf_apps_intro adf_apps
adf_apps_intro:
	@ printf "\n\nCompiling ADF applications\n=============================\n\n";
adf_apps:   CXXFLAGS += $(DEFS) -DADF $(CXXFLAGS_ADF)
adf_apps:   LDFLAGS+= $(LDFLAGS_ADF) $(LDFLAGS_DWARFS)
adf_apps:   $(TMLIB) $(ADFLIB) adf_apps_intro $(ADFDWARFS) #$(ADFAPPS)

# --------------------------------------------------------------------------------
# Clean
# --------------------------------------------------------------------------------

.PHONY:  clean_libobjs clean_tm clean_apps clean

clean_libobjs:
	@ printf "\nCleaning library object files\n-------------------------------\n\n"
	rm -f $(LIBOBJS)

clean_tm:
	rm -f $(TMMISCOBJS)

clean_apps:
	@ printf "\nCleaning applications\n-------------------------------\n\n"
	rm -f $(APPS)
	rm -f $(shell find $(APPDIR) -name *.o)
	rm -f $(shell find $(DWARFDIR) -name *.o)

clean:  clean_libobjs clean_apps clean_tm
	@ printf "\nCleaning static libraries\n-------------------------------\n\n"
	rm -f $(ADFLIB)
