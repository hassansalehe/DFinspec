#/////////////////////////////////////////////////////////////////
#//  ADFinspec: a lightweight non-determinism checking 
#//          tool for ADF applications
#//
#//    (c) 2015 - Hassan Salehe Matar & MSRC at Koc University
#//      Copying or using this code by any means whatsoever 
#//      without consent of the owner is strictly prohibited.
#//   
#//   Contact: hmatar-at-ku-dot-edu-dot-tr
#//
#/////////////////////////////////////////////////////////////////

CXX = g++
CXXFLGS = -std=c++11
DEPS = checker.h
OBJ = checker.o main.o
TOOL = ADFinspec

all: $(TOOL)

$(TOOL): $(OBJ)
	$(CXX) -o $@ $^ $(CXXFLGS)

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLGS)

.INTERMEDIATE: $(OBJ)
.PHONY: clean

clean:
	rm -rf *.o  *~ $(TOOL)
