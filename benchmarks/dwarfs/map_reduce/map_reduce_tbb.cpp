#include <cstdlib>
#include <cstdio>
#include <getopt.h>
#include <cmath>
#include <cstring>
#include <iostream>
#include <list>
#include <map>


#include "tbb/task_scheduler_init.h"
#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"

#include "adf.h"

using namespace tbb;
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
	sprintf(resultFile, "result_tbb.txt");

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
public:
	Solver(Configurator* configurator);
	~Solver();
	void Solve();
	void PrintSummery();
	void Finish();

private:
	Configurator	 *dwarfConfigurator;

	map<string, int>  stringTotal;			 	// final dictionary for counting the entries of chars in the content
	list<string>      stringEntries;			// auxillary list for counting the of chars in the content
	int               contentSize;				// size of *mainContent
	char              *mainContent;				// content for parsing

	string TransformCharFragment(int begin, int end);
	void GetContent();
	void Map();
	void Reduce();
	void Reduce(map<string, int> &partialStringTotal, std::list<string> &partialStringEntries);
	void SumThreadResult(map<string, int> &partialStringTotal);
};

void Solver::Reduce(map<string, int> &partialStringTotal, std::list<string> &partialStringEntries)
{
	partialStringEntries.sort();
    int count = 1;										// local variable for storing the count of each word
	string strCur;
	string strPre = partialStringEntries.front();
	partialStringEntries.pop_front();
	while (!partialStringEntries.empty())				// traverse the list
    {
		strCur = partialStringEntries.front();
		partialStringEntries.pop_front();
		if (strCur == strPre)							// checking the equivalence of adjoining words
		{
			count++;                                    // getting count of the word
        }
		else
        {
			partialStringTotal[strPre] = count;			// add to the final list
            count = 1;									// start the next word's count
    		strPre = strCur;
        }
    }
	partialStringTotal[strPre] = count;					// add to the final list
	partialStringEntries.clear();						// clearing the unused memory (all entrines is already consisted in stringTotal)
}

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
void Solver::Map()
{
	int i = 0;  // iteration counter
	int count = 0; // local variable for storing the length of each word
	while (i < contentSize)
	{
		char c = mainContent[i];
		if ((c > 47 && c < 58) ||
			(c > 96 && c < 123) ||
			(c > 64 && c < 91) ||
			(c == 95) ||
			(c == 45) ||
			(c == 39))
		{
			count++;  // getting the length of a string
		}
		else
		{
			if (count > 0)
			{
				string stringFragment = TransformCharFragment(i - count, i);
				stringEntries.push_back(stringFragment);   // add substring
			}
			string stringFragment = TransformCharFragment(i, i + 1);
			stringEntries.push_back(stringFragment);  // add single-char string
			count = 0;  // start length counting of a new word
		}
		i++;
	}
}


void Solver::Solve()
{
	int partSize = 256*1024; // 256k is the best experimental setting for openmp for various inputs

	parallel_for(	blocked_range<size_t>(0, contentSize, partSize), [&](const blocked_range<size_t>& r) {
		list<string> localStringEntries;
		map<string, int>  localStringTotal;
		int start = r.begin();
		int end = r.end();
		int count = 0;
		int i;
		while ((start != 0) && (mainContent[start] != ' ')) {
			start++;
		}
		while ((end < contentSize ) && (mainContent[end] != ' ')) {
			end++;
		}

		// Map part
		for (i = start; i < end; ++i) {
			char c = mainContent[i];
			if ((c > 47 && c < 58) ||
					(c > 96 && c < 123) ||
					(c > 64 && c < 91) ||
					(c == 95) ||
					(c == 45) ||
					(c == 39))
			{
				count++;  // getting the length of a string
			}
			else
			{
				if (count > 0) {
					string stringFragment = TransformCharFragment(i - count, i);
					localStringEntries.push_back(stringFragment);   // add substring
				}
				string stringFragment = TransformCharFragment(i, i + 1);
				localStringEntries.push_back(stringFragment);  // add single-char string
				count = 0;  // start length counting of a new word
			}
		}

		Reduce(localStringTotal, localStringEntries);
		SumThreadResult(localStringTotal);
	});

}
void Solver::SumThreadResult(map<string, int> &partialStringTotal)
{
	for (map<string, int>::const_iterator it = partialStringTotal.begin(); it != partialStringTotal.end(); it++) {
		string partialKey = it -> first;
		int partialValue = it -> second;
		map<string, int>::iterator ittotal;

#ifdef OPTIMAL
//		TRANSACTION_BEGIN(1,RW)
//		LOCK
		__transaction_relaxed {
#endif
		// updating Dictionary values
		ittotal =  stringTotal.find(partialKey);
		if (ittotal != stringTotal.end()) {
			stringTotal[partialKey] += partialValue;			// the value is already mapped
		}
		else {
			stringTotal[partialKey] = partialValue;				// first occurrence of the value
		}
#ifdef OPTIMAL
//		UNLOCK
		TRANSACTION_END
#endif

	}
	partialStringTotal.clear();					// clearing the unused memory (all data is already stringTotal)
}

//Groups 'stringEntries' elements and count the number of entry of each word
void Solver::Reduce()
{
	stringEntries.sort();
	int count = 1; // local variable for storing the count of each word
	string strCur;
	string strPre = stringEntries.front();
	stringEntries.pop_front();
	while (!stringEntries.empty()) // traverse the list
	{
		strCur = stringEntries.front();
		stringEntries.pop_front();
		if (strCur == strPre) // checking the equivalence of adjoining words
		{
			count++; // getting count of the word
		}
		else
		{
			stringTotal[strPre] = count; // add to the final list
			count = 1; // start the next word's count
		}
		strPre = strCur;
	}
	stringTotal[strPre] = count; // add to the final list
	stringEntries.clear(); // clearing the unused memory (all entrines is already consisted in stringTotal)
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
				"   map_reduce_tbb [options ...]\n"
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


