#include <cstdio>
#include <getopt.h>
#include <cmath>
#include <cstring>
#include <iostream>
#include <list>
#include <map>

#include "adf.h"

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
	sprintf(resultFile, "result_adf.txt");

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

	struct MapBounds {
		int start;
		int end;
	};

	// tokens
	MapBounds 			mapToken;
	list<string> 		*reduceToken;
	map<string, int>	*sumToken;

	// tasks
	void MapTask();
	void ReduceTask();
	void SumTask();
	void InitialTask();
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


void Solver::InitialTask()
{
	static int start = 0;
	// This is the best experimental setting for ADF for various inputs
	static int partSize = 1 << (9 + (int) log2(contentSize)/3);

	void *outtokens[] = {&mapToken};
	adf_create_task(1, 0, NULL, [=](token_t *tokens) -> void
	{
		/* TASK START */
		/* recreate context */

		/* redeclare output token data as local variables */
		std::remove_reference<decltype(mapToken)>::type mapToken;

		/* declare local data */
		int exit = 0;	/* the local exit condition flag for use outside of critical section */

		/* UNTIL clause */
		TRANSACTION_BEGIN(1,RO)
				if (start >= contentSize)
					exit = 1;
		TRANSACTION_END

		if (exit) {
			adf_task_stop();
			return;
		}

		TRACE_EVENT(18001,181)
		TRACE_EVENT(18002,182)

		/* TASK BODY */
#ifndef OPTIMAL
		TRANSACTION_BEGIN(4,RW)
#endif

			int end = start + partSize;
			if (end > contentSize) {
				end = contentSize;
			}
			else {
				while (mainContent[end] != ' ')  {						// search of middle-word spaces
					end++;
				}
				end++;
			}

			mapToken.start = start;
			mapToken.end = end;
			start = end;

#ifndef OPTIMAL
		TRANSACTION_END
#endif

		TRACE_EVENT(18002,0)

		/* TASK END */
		adf_pass_token(outtokens[0], &mapToken, sizeof(mapToken));    /* pass tokens */

		TRACE_EVENT(18001,0)
	}, 2, PIN, 0);

}


void Solver::MapTask()
{
	void *intokens[] = {&mapToken};
	void *outtokens[] = {&reduceToken};
	adf_create_task(num_threads, 1, intokens, [=] (token_t *tokens) -> void
	{
		TRACE_EVENT(18003,183)

		/* TASK START */
		/* recreate context */

		/* redeclare input tokens */
		std::remove_reference<decltype(mapToken)>::type mapToken;
		memcpy(&mapToken, tokens->value, sizeof(mapToken));

		/* redeclare output token data as local variables */
		std::remove_reference<decltype(reduceToken)>::type reduceToken;

		/* TASK BODY */

		STAT_COUNT_EVENT(task_exec, GetThreadID());
		STAT_START_TIMER(task_body, GetThreadID());

		TRACE_EVENT(18004,184)

#ifndef OPTIMAL
		TRANSACTION_BEGIN(5,RW)
#endif

		list<string> *partialStringEntries = new list<string>();
		int i = mapToken.start;									// iteration counter
	    int count = 0;									// local variable for storing the length of each word
	    while (i < mapToken.end)
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

	    reduceToken = partialStringEntries;

#ifndef OPTIMAL
		TRANSACTION_END
#endif

		TRACE_EVENT(18004,0)

		STAT_STOP_TIMER(task_body, GetThreadID());

		/* TASK END */
		adf_pass_token(outtokens[0], &reduceToken, sizeof(reduceToken));    /* pass tokens */

		TRACE_EVENT(18003,0)
	});
}


void Solver::ReduceTask()
{
	void *intokens[] = {&reduceToken};
	void *outtokens[] = {&sumToken};
	adf_create_task(num_threads, 1, intokens, [=] (token_t *tokens) -> void
	{
		TRACE_EVENT(18005,185)

		/* TASK START */
		/* recreate context */

		/* redeclare input tokens */
		std::remove_reference<decltype(reduceToken)>::type reduceToken;
		memcpy(&reduceToken, tokens->value, sizeof(reduceToken));

		/* redeclare output token data as local variables */
		std::remove_reference<decltype(sumToken)>::type sumToken;

		/* TASK BODY */

		STAT_COUNT_EVENT(task_exec, GetThreadID());
		STAT_START_TIMER(task_body, GetThreadID());

		TRACE_EVENT(18006,186)

#ifndef OPTIMAL
		TRANSACTION_BEGIN(5,RW)
#endif

		//printf("Thread %d: Reduce\n",GetThreadID() );

		sumToken = new map<string, int>();
		map<string, int> &partialStringTotal = *sumToken;
		list<string> *partialStringEntries = reduceToken;
		partialStringEntries->sort();

	    int count = 1;										// local variable for storing the count of each word
		string strCur;
		string strPre = partialStringEntries->front();
		partialStringEntries->pop_front();
		while (!partialStringEntries->empty())				// traverse the list
	    {
			strCur = partialStringEntries->front();
			partialStringEntries->pop_front();
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
		delete partialStringEntries;

#ifndef OPTIMAL
		TRANSACTION_END
#endif

		TRACE_EVENT(18006,0)

		STAT_STOP_TIMER(task_body, GetThreadID());

		/* TASK END */
		adf_pass_token(outtokens[0], &sumToken, sizeof(sumToken));    /* pass tokens */

		TRACE_EVENT(18005,0)
	});
}


void Solver::SumTask()
{
	void *intokens[] = {&sumToken};
	adf_create_task(1, 1, intokens, [=] (token_t *tokens) -> void
	{
		TRACE_EVENT(18007,187)

		/* TASK START */
		/* recreate context */

		/* redeclare input tokens */
		std::remove_reference<decltype(sumToken)>::type sumToken;
		memcpy(&sumToken, tokens->value, sizeof(sumToken));

		/* TASK BODY */

		STAT_COUNT_EVENT(task_exec, GetThreadID());
		STAT_START_TIMER(task_body, GetThreadID());

		TRACE_EVENT(18008,188)

#ifndef OPTIMAL
		TRANSACTION_BEGIN(5,RW)
#endif

		map<string, int> &partialStringTotal = *sumToken;
		for (map<string, int>::const_iterator it = partialStringTotal.begin(); it != partialStringTotal.end(); it++) {
			string partialKey = it -> first;
			int partialValue = it -> second;
			map<string, int>::iterator ittotal;

			// updating Dictionary values
			ittotal =  stringTotal.find(partialKey);
			if (ittotal != stringTotal.end()) {
				stringTotal[partialKey] += partialValue;			// the value is already mapped
			}
			else {
				stringTotal[partialKey] = partialValue;				// first occurrence of the value
			}
		}
		sumToken->clear();
		delete sumToken;

#ifndef OPTIMAL
		TRANSACTION_END
#endif

		TRACE_EVENT(18008,0)

		STAT_STOP_TIMER(task_body, GetThreadID());

		/* TASK END */

		TRACE_EVENT(18007,0)
	});
}


void Solver::Solve()
{
	printf("\ncontentSize = %d\n", contentSize);

	MapTask();
	ReduceTask();
	SumTask();
	InitialTask();
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

	//HASSAN input_file_name = (char*) "largecontent.txt";
	input_file_name = (char*) "mediumcontent.txt";
	num_threads = 1;
	print_results = false;

	while ((c = getopt (argc, argv, "hf:n:r")) != -1)
		switch (c) {
			case 'h':
				printf("\nMap-Reduce - ADF benchmark application\n"
				"\n"
				"Usage:\n"
				"   map_reduce_adf [options ...]\n"
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

	printf ("\nStarting ADF Map-Reduce.\n");
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
	Solver solver(&dwarfConfigurator);					// create new Solver (current problem with initial data

	adf_init(num_threads);

	solver.Solve();										// Solve the current problem

	/* start a dataflow execution */
	adf_start();

	/* wait for the end of execution */
	adf_taskwait();

	solver.PrintSummery();

	adf_terminate();

	solver.Finish();									// write results

	return 0;
}

