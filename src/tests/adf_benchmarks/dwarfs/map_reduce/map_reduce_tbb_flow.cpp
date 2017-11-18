#include <cstdlib>
#include <cstdio>
#include <getopt.h>
#include <cmath>
#include <cstring>
#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <pthread.h>
#include <memory>
#include "tbb/task_scheduler_init.h"
#include "tbb/flow_graph.h"

#include "adf.h"

using namespace tbb;
using namespace tbb::flow;
using namespace std;

typedef char mychar;

bool  print_results;
char *input_file_name;
int   num_threads;

#define OPTIMAL


/*
======================================================================
	Settings
======================================================================
*/

class Settings
{
public:
    char* dwarfName;
    char* inputFile;
	char* resultFile;

	Settings(char *input_file);
	~Settings();
	void PrintStringSettings();

private:
	static const int maxArraySize = 5000;
};


Settings::Settings(char *input_file)
{
	dwarfName = new char[maxArraySize];
    inputFile = new char[maxArraySize];
	resultFile = new char[maxArraySize];

	dwarfName[0] = inputFile[0] = resultFile[0] = '\0';

	sprintf(dwarfName, "%s", "MapReduce");
	sprintf(inputFile, "%s", input_file);
	sprintf(resultFile, "result_tbb_flow.txt");

}

Settings::~Settings()
{
	delete[] dwarfName;
	delete[] inputFile;
	delete[] resultFile;
}

void Settings::PrintStringSettings()
{
	char* stringSettings = new char[maxArraySize];
	stringSettings[0] = '\0';

	sprintf(stringSettings, "%s%s", stringSettings, "Kernel settings summary: ");
	sprintf(stringSettings, "%s%s", stringSettings, "\nDwarf name        : ");
	sprintf(stringSettings, "%s%s", stringSettings, dwarfName);
	sprintf(stringSettings, "%s%s", stringSettings, "\nInputFile         : ");
	sprintf(stringSettings, "%s%s", stringSettings, inputFile);
	sprintf(stringSettings, "%s%s", stringSettings, "\nResultFile        : ");
	sprintf(stringSettings, "%s%s", stringSettings, resultFile);

	printf("%s", stringSettings);

	delete[] stringSettings;
}


/*
======================================================================
	Configurator
======================================================================
*/

class Configurator
{
public:
	Configurator(char *input_file) { settings = new Settings(input_file); }
	~Configurator() { delete settings; }
	void GetContent(char **mainContent, int *contentSize);
	void WriteSettings() { settings -> PrintStringSettings(); }
	void Close(map<string, int> & stringTotal);

private:
	Settings* settings;
};

void Configurator :: GetContent(char **mainContent, int *contentSize)
{
	int ret;
	FILE *file = fopen(settings -> inputFile, "rb");				// Opening the input file
	fseek (file, 0 , SEEK_END);										// move the file pointer on the end of content file
	*contentSize = ftell (file);									// size of all file

	rewind (file);													// reposition the file pointer to the file beginning
	*mainContent = new char[*contentSize];							// dynamic memory memory allocation for [contentSize] char array
	ret = fread (*mainContent, sizeof(char), *contentSize, file);	// read the file content to char array

	fclose(file);
}

void Configurator::Close(map<string, int> & stringTotal)
{
	if (print_results == true) {
		FILE *result = fopen(settings -> resultFile, "wb");

		for (map<string, int>::const_iterator it = stringTotal.begin(); it != stringTotal.end(); ++it)
		{												// traverse all the map
			string key = it -> first;					// get all the keys
			int value = it -> second;					// get all the values
			fprintf(result, "%s", key.c_str());			// print to the outputfile
			fprintf(result, " %i\r\n", value);
		}
		fclose(result);
	}
}


/*
======================================================================
	Solver
======================================================================
*/


class Solver
{
	friend class MapNodeBody;
	friend class ReduceNodeBody;
	friend class SumNodeBody;

public:
	Solver(Configurator* configurator);
	~Solver();
	void Solve();
	void PrintSummery();
	void Finish();

private:
	Configurator	 *dwarfConfigurator;

protected:
	map<string, int>  stringTotal;			 	// final dictionary for counting the entries of chars in the content
	list<string>      stringEntries;			// auxillary list for counting the of chars in the content
	int               contentSize;				// size of *mainContent
	char              *mainContent;				// content for parsing

	string TransformCharFragment(int begin, int end);
	void GetContent();
	void Map(shared_ptr<list<string>> partialStringEntries, int start, int end);
	void Reduce(shared_ptr<list<string>> partialMap, shared_ptr<map<string, int>> partialReduction);
};

Solver::Solver(Configurator* configurator)
{
	mainContent = NULL;
	contentSize = 0;

	dwarfConfigurator = configurator;					// process settings from the command line
	dwarfConfigurator -> WriteSettings();
	dwarfConfigurator -> GetContent(&mainContent, &contentSize);
}

Solver::~Solver()
{
	delete[] mainContent;
}


string Solver::TransformCharFragment(int begin, int end)
{
	int length = end - begin;
	char* currentWord = new char[length];									// dynamic memory allocation for new char array
	for (int j = 0; j < length; j++) {
		currentWord[j] = mainContent[begin + j];							// filling the array
	}
	string transform(reinterpret_cast<const char*>(currentWord), length);	// convert to string
	delete[] currentWord;													// free memory from char array
	return transform;
}


// Traverses the string 'content'. Collect each word as substring of 'content'. Change the 'stringEntries' value
void Solver::Map(shared_ptr<list<string>> partialStringEntries, int start, int end)
{
	while ((start != 0) && (mainContent[start] != ' ')) {
		start++;
	}
	while ((end < contentSize ) && (mainContent[end] != ' ')) {
		end++;
	}
	int i = start;									// iteration counter
	int count = 0;									// local variable for storing the length of each word
	while (i < end)
	{
		char c = mainContent[i];
		if ((c > 47 && c < 58)  ||
			(c > 96 && c < 123) ||
			(c > 64 && c < 91)  ||
			 c == 95			||
			 c == 45			||
			 c == 39)
		{
			count++;															// getting the length of a string
		}
		else
		{
			if (count > 0)
			{
				string stringFragment = TransformCharFragment(i - count, i);
				partialStringEntries->push_back(stringFragment);					// add substring
			}
			string stringFragment = TransformCharFragment(i, i + 1);
			partialStringEntries->push_back(stringFragment);						// add single-char string
			count = 0;															// start length counting of a new word
		}
		i++;
	}
}


//Groups 'stringEntries' elements and count the number of entry of each word
void Solver::Reduce(shared_ptr<list<string>> partialMap, shared_ptr<map<string, int>> partialReduction)
{
	partialMap->sort();
	int count = 1;										// local variable for storing the count of each word
	string strCur;
	string strPre = partialMap->front();
	partialMap->pop_front();
	while (!partialMap->empty())				// traverse the list
	{
		strCur = partialMap->front();
		partialMap->pop_front();
		if (strCur == strPre)							// checking the equivalence of adjoining words
		{
			count++;                                    // getting count of the word
		}
		else
		{
			(*partialReduction)[strPre] = count;			// add to the final list
			count = 1;									// start the next word's count
			strPre = strCur;
		}
	}
	(*partialReduction)[strPre] = count;					// add to the final list
	partialMap->clear();						// clearing the unused memory (all entrines is already consisted in stringTotal)
}


struct MapNodeBody {
	Solver *solver;
	MapNodeBody(Solver *s_) : solver(s_) {}

	shared_ptr<list<string>> operator() (tuple<int,int> in) {
		int start = get<0>(in);
		int end = get<1>(in);
		shared_ptr<list<string>> partialMap(new list<string>());

		solver->Map(partialMap, start, end);
		return partialMap;
	}
};


struct ReduceNodeBody {
	Solver *solver;
	ReduceNodeBody(Solver *s_) : solver(s_) {}

	shared_ptr<map<string, int>> operator() ( shared_ptr<list<string>> partialMap) {
		shared_ptr<map<string, int>> partialReduction(new map<string, int>);
		solver->Reduce(partialMap, partialReduction);
		return partialReduction;
	}
};


struct SumNodeBody {
	Solver *solver;
	SumNodeBody(Solver *s_) : solver(s_) {}

	void operator() ( shared_ptr<map<string, int>> partialReduction) {
		map<string, int>::iterator ittotal;
		for (map<string, int>::const_iterator it = partialReduction->begin(); it != partialReduction->end(); it++) {
			string key = it -> first;
			int value = it -> second;

			// updating Dictionary values
			ittotal =  solver->stringTotal.find(key);
			if (ittotal != solver->stringTotal.end()) {
				solver->stringTotal[key] += value;			// the value is already mapped
			}
			else {
				solver->stringTotal[key] = value;				// first occurrence of the value
			}
		}
	}
};

void Solver::Solve()
{
	graph g;
	function_node<tuple<int,int>, shared_ptr<list<string>>> MapNode (g, unlimited, MapNodeBody(this));
	function_node<shared_ptr<list<string>>, shared_ptr<map<string, int>>> ReduceNode(g, unlimited, ReduceNodeBody(this));
	function_node<shared_ptr<map<string, int>> > SumNode (g, 1, SumNodeBody(this));

	make_edge(MapNode, ReduceNode);
	make_edge(ReduceNode, SumNode);

	// This is the best experimental setting for various inputs
	int partSize = 1 << (9 + (int) log2(contentSize)/3);
	int start = 0;
	int end = partSize;
	while (start < contentSize) {
		MapNode.try_put(tuple<int, int>(start, end));
		start = end;
		end += partSize;
		if (end > contentSize)
			end = contentSize;
	}

	g.wait_for_all();
}


void Solver::PrintSummery()
{
	printf("\nNumber of pairs : %ld\n", (long) stringTotal.size());
}


void Solver::Finish()	// Problem results output
{
	dwarfConfigurator->Close(stringTotal);
	stringTotal.clear();			// clearing the unused memory (all data is already printed)
}




/*
====================================================================================
	ParseCommandLine
====================================================================================
*/

void ParseCommandLine(int argc, char **argv)
{
	char c;

	input_file_name = (char*) "largecontent.txt";
	num_threads = 1;
	print_results = false;

	while ((c = getopt (argc, argv, "hf:n:r")) != -1)
		switch (c) {
			case 'h':
				printf("\nMap-Reduce - ADF benchmark application\n"
				"\n"
				"Usage:\n"
				"   map_reduce_tbb_flow [options ...]\n"
				"\n"
				"Options:\n"
				"   -h\n"
				"        Print this help message.\n"
				"   -f <filename>\n"
				"        Input file name.\n"
				"   -n <long>\n"
				"        Number of worker threads. (default 1)\n"
				"   -r \n"
				"        print results to a result file\n"
				"\n"
				);
				exit (0);
			case 'f':
				input_file_name = optarg;
				break;
			case 'n':
				num_threads = strtol(optarg, NULL, 10);
				break;
			case 'r':
				print_results = true;
				break;
			case '?':
				if (optopt == 'c')
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				exit(1);
			default:
				exit(1);
		}

	if(input_file_name == NULL) {
		printf("missing input file!\n");
		exit(1);
	}

	printf ("\nStarting TBB Map-Reduce.\n");
	printf ("Running with %d threads. Input file %s\n", num_threads, input_file_name);
	printf ("=====================================================\n\n");
}



/*
======================================================================
	main
======================================================================
*/


int main(int argc, char** argv)
{
	ParseCommandLine(argc, argv);

	Configurator dwarfConfigurator(input_file_name);
	Solver solver(&dwarfConfigurator);					// create new Solver (current problem with initial data)

 	NonADF_init(num_threads);

	task_scheduler_init init(num_threads);

	solver.Solve();										// Solve the current problem

	solver.PrintSummery();

 	NonADF_terminate();

	solver.Finish();									// write results

	return 0;
}
