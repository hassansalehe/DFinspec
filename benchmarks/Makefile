include Makefile.common

HEADERS =                        \
	$(SRCDIR)/buffer.h           \
	$(SRCDIR)/common.h           \
	$(SRCDIR)/debug.h            \
	$(SRCDIR)/threadpool.h       \
	$(SRCDIR)/taskqueue.h        \
	$(SRCDIR)/adf.h              

LIBSRCS=                        \
	$(SRCDIR)/common.cpp        \
	$(SRCDIR)/statistics.cpp    \
	$(SRCDIR)/taskqueue.cpp     \
	$(SRCDIR)/buffer.cpp        \
	$(SRCDIR)/worksteal.cpp     \
	$(SRCDIR)/threadpool.cpp    \
	$(SRCDIR)/adf.cpp           


LIBOBJS_ADF=$(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/adf/%.o, $(wildcard $(SRCDIR)/*.cpp))
LIBOBJS_ADF_DEBUG=$(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/adf_debug/%.o, $(wildcard $(SRCDIR)/*.cpp))
LIBOBJS_ADF_TRACE=$(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/adf_trace/%.o, $(wildcard $(SRCDIR)/*.cpp))
LIBOBJS_OMP= $(OBJDIR)/omp/common.o $(OBJDIR)/omp/statistics.o $(OBJDIR)/omp/adf.o
LIBOBJS_SEQ= $(OBJDIR)/seq/common.o $(OBJDIR)/seq/statistics.o $(OBJDIR)/seq/adf.o
LIBOBJS_SEQ_TRACE= $(LIBOBJS_SEQ)
LIBOBJS= $(LIBOBJS_ADF) $(LIBOBJS_ADF_DEBUG) $(LIBOBJS_ADF_TRACE) $(LIBOBJS_OMP) $(LIBOBJS_SEQ)
TMMISCOBJS=$(patsubst %.cpp,%.o,$(wildcard $(TMMISC)/*.cpp))

.PHONY: adf
all: adf sequential

$(ADFLIB):     $(LIBOBJS_ADF)
	$(AR) rcs $@ $^

$(ADFLIB_DEBUG):  $(LIBOBJS_ADF_DEBUG)
	$(AR) rcs $@ $^

$(ADFLIB_TRACE):  $(LIBOBJS_ADF_TRACE)
	$(AR) rcs $@ $^
	
$(ADFLIB_SEQ_TRACE):  $(LIBOBJS_SEQ_TRACE)
	$(AR) rcs $@ $^

$(ADFLIB_OMP): $(LIBOBJS_OMP)
	$(AR) rcs $@ $^

$(ADFLIB_SEQ): $(LIBOBJS_SEQ)
	$(AR) rcs $@ $^

$(OBJDIR)/adf/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

$(OBJDIR)/adf_debug/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

$(OBJDIR)/adf_trace/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

$(OBJDIR)/omp/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

$(OBJDIR)/seq/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

.cpp.o:
	$(APPNAME)
	$(CXX) $(CXXFLAGS) $< -o $@

%_opt.o: %.cpp
	$(APPNAME)
	$(CXX) $(CXXFLAGS) $< -o $@

%_trace.o: %.cpp
	$(APPNAME)
	$(CXX) $(CXXFLAGS) $< -o $@

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
  CXXFLAGS=$(CXXFLAGS_DEBUG)
endif

.PHONY: intro_adf intro_adf_debug intro_adf_trace intro_omp intro_seq
intro_adf:
	@ printf "\nCompiling ADF library\n====================================\n\n";
intro_adf_debug:
	@ printf "\nCompiling ADF library for debugging\n====================================\n\n";
intro_adf_trace:
	@ printf "\nCompiling ADF library for tracing\n====================================\n\n";
intro_omp:
	@ printf "\n\nCompiling ADF_OPENMP library\n====================================\n\n";
intro_seq:
	@ printf "\n\nCompiling ADF_SEQUENTIAL library\n====================================\n\n";
intro_adf_seq_trace:
	@ printf "\nCompiling ADF library for sequential tracing\n====================================\n\n";

.PHONY: adf
adf:         adf_lib adf_apps #adf_apps_opt

.PHONY: adf_lib
adf_lib:         adf_lib_test adf_lib_debug #adf_lib_trace

.PHONY: adf_lib_test
adf_lib_test:    CXXFLAGS+= $(DEFS) -DADF $(CXXFLAGS_ADF)
adf_lib_test:    LDFLAGS+= $(LDFLAGS_ADF)
adf_lib_test:    intro_adf $(ADFLIB)

.PHONY: adf_lib_debug
adf_lib_debug:   CXXFLAGS = $(CXXFLAGS_DEBUG) $(DEFS) -DADF $(CXXFLAGS_ADF)
adf_lib_debug:   LDFLAGS = $(LDFLAGS_DEBUG) $(LDFLAGS_ADF)
adf_lib_debug:   intro_adf_debug $(ADFLIB_DEBUG)

.PHONY: adf_lib_trace
adf_lib_trace:   CXXFLAGS = $(CXXFLAGS_TRACE) $(DEFS) -DADF $(CXXFLAGS_ADF)
adf_lib_trace:   LDFLAGS = $(LDFLAGS_TRACE) $(LDFLAGS_ADF)
adf_lib_trace:   intro_adf_trace $(ADFLIB_TRACE)

.PHONY: openmp
openmp:      CXXFLAGS+= $(DEFS) -DOPENMP $(CXXFLAGS_OMP)
openmp:      LDFLAGS+= $(LDFLAGS_OMP)
openmp:      intro_omp $(ADFLIB_OMP) omp_apps #omp_apps_opt

.PHONY: tbb
tbb:      tbb_apps

.PHONY: tbb_flow
tbb_flow: tbb_flow_apps

.PHONY: sequential
sequential:  CXXFLAGS+= -DSEQUENTIAL $(CXXFLAGS_SEQ)
sequential:  LDFLAGS+= $(LDFLAGS_SEQ)
sequential:  intro_seq $(ADFLIB_SEQ) seq_apps

.PHONY: adf_lib_seq_trace
adf_lib_seq_trace:   CXXFLAGS = $(CXXFLAGS_TRACE) -DSEQUENTIAL $(CXXFLAGS_SEQ)
adf_lib_seq_trace:   LDFLAGS = $(LDFLAGS_TRACE) $(LDFLAGS_SEQ)
adf_lib_seq_trace:   intro_adf_seq_trace $(ADFLIB_SEQ_TRACE)


.PHONY: trace
trace:   CXXFLAGS = $(CXXFLAGS_TRACE)
trace:   LDFLAGS = $(LDFLAGS_TRACE)
trace:   LDFLAGS_ADF = -L$(LIBDIR) -litm -ladf_trace -lpthread
trace:   adf_lib_seq_trace adf_lib_trace dwarf_trace



# --------------------------------------------------------------------------------
# Rules to compile applications
# --------------------------------------------------------------------------------

DWARFS= $(ADFDWARFS) $(OMPDWARFS) $(TBBDWARFS) $(TBB_FLOW_DWARFS) $(SEQDWARFS) $(ADFDWARFS_TRACE)
APPS= $(DWARFS) #$(ADFAPPS) $(ADFAPPS_OPT) $(OMPAPPS) $(OMPAPPS_OPT) $(SEQAPPS) 

$(APPS): APPNAME=@ printf "\n$(subst .o,,$(lastword $(subst /, ,$@)))\n-------------------\n\n"
$(APPS): %:%.o
	$(CXX) -o $@ $< $(LDFLAGS)

.PHONY: adf_apps_intro adf_apps
adf_apps_intro:
	@ printf "\n\nCompiling ADF applications\n=============================\n\n";
adf_apps:   CXXFLAGS += $(DEFS) -DADF $(CXXFLAGS_ADF)
adf_apps:   LDFLAGS+= $(LDFLAGS_ADF) $(LDFLAGS_DWARFS)
adf_apps:   $(TMLIB) $(ADFLIB) adf_apps_intro $(ADFDWARFS) #$(ADFAPPS)

.PHONY: omp_apps_opt_intro omp_apps_opt
adf_apps_opt_intro:
	@ printf "\n\nCompiling ADF_OPT applications\n=============================\n\n";
adf_apps_opt:   CXXFLAGS += $(DEFS) -DADF $(CXXFLAGS_ADF) -DOPTIMAL
adf_apps_opt:   LDFLAGS+= $(LDFLAGS_ADF) $(LDFLAGS_DWARFS)
adf_apps_opt:   $(TMLIB) $(ADFLIB) adf_apps_opt_intro $(ADFAPPS_OPT)

.PHONY: omp_apps_intro omp_apps
omp_apps_intro:
	@ printf "\n\nCompiling OpenMP applications\n=============================\n\n";
omp_apps:   CXXFLAGS+= $(DEFS) -DOPENMP $(CXXFLAGS_OMP)
omp_apps:   LDFLAGS+= $(LDFLAGS_OMP) $(LDFLAGS_DWARFS)
omp_apps:   $(TMLIB) $(ADFLIB_OMP) omp_apps_intro $(OMPDWARFS) #$(OMPAPPS)

.PHONY: omp_apps_opt_intro omp_apps_opt
omp_apps_opt_intro:
	@ printf "\n\nCompiling OpenMP_OPT applications\n=============================\n\n";
omp_apps_opt:   CXXFLAGS+= $(DEFS) -DOPENMP $(CXXFLAGS_OMP) -DOPTIMAL
omp_apps_opt:   LDFLAGS+= $(LDFLAGS_OMP) $(LDFLAGS_DWARFS)
omp_apps_opt:   $(TMLIB) $(ADFLIB_OMP) omp_apps_opt_intro $(OMPAPPS_OPT)

.PHONY: tbb_apps_intro tbb_apps
tbb_apps_intro:
	@ printf "\n\nCompiling TBB applications\n=============================\n\n";
tbb_apps:   CXXFLAGS+= $(DEFS) -DOPENMP $(CXXFLAGS_TBB)
tbb_apps:   LDFLAGS+= $(LDFLAGS_TBB) $(LDFLAGS_DWARFS)
tbb_apps:   $(TMLIB) $(ADFLIB_OMP) tbb_apps_intro $(TBBDWARFS)

.PHONY: tbb_flow_apps_intro tbb_flow_apps
tbb_flow_apps_intro:
	@ printf "\n\nCompiling TBB Flow Graph applications\n=============================\n\n";
tbb_flow_apps:   CXXFLAGS+= $(DEFS) -DOPENMP $(CXXFLAGS_TBB)
tbb_flow_apps:   LDFLAGS+= $(LDFLAGS_TBB) $(LDFLAGS_DWARFS)
tbb_flow_apps:   $(TMLIB) $(ADFLIB_OMP) tbb_apps_intro $(TBB_FLOW_DWARFS)

.PHONY: seq_apps_intro seq_apps
seq_apps_intro:
	@ printf "\n\nCompiling sequential applications\n=============================\n\n";
seq_apps:   CXXFLAGS+= -DSEQUENTIAL $(CXXFLAGS_SEQ)
seq_apps:   LDFLAGS+= $(LDFLAGS_SEQ) $(LDFLAGS_DWARFS)
seq_apps:   $(ADFLIB_SEQ) seq_apps_intro $(SEQDWARFS) #$(SEQAPPS)

.PHONY: dwarf_trace_intro dwarf_trace
dwarf_trace_intro:
	@ printf "\n\nCompiling DWARF applications for tracing\n=============================\n\n";
dwarf_trace:   CXXFLAGS += $(DEFS) -DADF $(CXXFLAGS_ADF)
dwarf_trace:   LDFLAGS+= $(LDFLAGS_ADF) $(LDFLAGS_DWARFS)
dwarf_trace:   $(ADFLIB_TRACE) dwarf_trace_intro $(ADFDWARFS_TRACE)


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
	rm -f $(ADFLIB) $(ADFLIB_DEBUG) $(ADFLIB_TRACE) $(ADFLIB_OMP) $(ADFLIB_SEQ) $(ADFLIB_SEQ_TRACE)




