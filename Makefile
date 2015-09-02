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
