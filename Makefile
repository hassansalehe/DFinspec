#/////////////////////////////////////////////////////////////////
#//  ADFinspec: a lightweight non-determinism checking
#//          tool for ADF applications
#//
#//    (c) 2015, 2016 - Hassan Salehe Matar & MSRC at Koc University
#//      Copying or using this code by any means whatsoever
#//      without consent of the owner is strictly prohibited.
#//
#//   Contact: hmatar-at-ku-dot-edu-dot-tr
#//
#/////////////////////////////////////////////////////////////////

# determine a compiler to use
ERR = $(shell which clang >/dev/null; echo $$?)
ifeq "$(ERR)" "0"
  CXX = clang++
else
  ERR = $(shell which g++ >/dev/null; echo $$?)
  ifeq "$(ERR)" "0"
    CXX = g++
  endif
endif
#CXX = g++
CXXFLGS = -g -O3 -std=c++11
DEPS = DFchecker/validator.h DFchecker/checker.h DFchecker/action.h DFchecker/conflictReport.h DFchecker/defs.h
OBJ = validator.o checker.o main.o
TOOL = ADFinspec

all: $(TOOL)

$(TOOL): $(OBJ)
	$(CXX) -o $@ $^ $(CXXFLGS)

%.o: DFchecker/%.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLGS)

.INTERMEDIATE: $(OBJ)
.PHONY: clean

clean:
	rm -rf *.o  *~ $(TOOL)
