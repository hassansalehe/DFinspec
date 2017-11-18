#include <cstdlib>
#include <cstdio>
#include <getopt.h>
#include <cstring>
#include <iostream>

#include "tbb/task_scheduler_init.h"
#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"

#include "adf.h"
#include "list.h"

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

	sprintf(dwarfName, "%s", "FiniteStateMachine");
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
	sprintf(stringSettings, "%s%s", stringSettings, "\nProfileFile       : ");


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
	Configurator(char *input_file);
	~Configurator();
	void GetContent(mychar **mainContent, long *contentSize, mychar **patternContent, int *stateDimensionCoincidence);
	void WriteSettings();
	void Close(list<long> & positions);

private:
	Settings* settings;
};



Configurator::Configurator(char *input_file)
{
    settings = new Settings(input_file);
}

Configurator::~Configurator()
{
	delete settings;
}

void Configurator :: WriteSettings()
{
    settings -> PrintStringSettings();
}






void Configurator :: GetContent(mychar **mainContent, long *contentSize, mychar **patternContent, int *stateDimensionCoincidence)
{
	int size = 0;
	size_t ret;
	FILE *file = fopen(settings -> inputFile, "rb");		// Opening the input file
	int currentChar = getc(file);
	while (currentChar != 1)								// while the current symbol is not equal to delimeter ((char)'1') read the pattern
    {
		currentChar = getc(file);
		size++;
    }

	*stateDimensionCoincidence = size;
	rewind (file);											// reposition the file pointer to the file beginning

	*patternContent = new mychar[*stateDimensionCoincidence];
	ret = fread (*patternContent, sizeof(char), *stateDimensionCoincidence, file);		// read the pattern content to char array

	fseek (file, 0 , SEEK_END);								// move the file pointer on the end of content file
	*contentSize = ftell (file) - size;		     		    // size of string

	rewind (file);											// reposition the file pointer to the file beginning

	currentChar = getc(file);								// set position to the delimeter
	while (currentChar != 1) {								// while the current symbol is not equal to delimeter ((char)'1') read the pattern
		currentChar = getc(file);
    }

	*mainContent = new mychar[*contentSize];
	ret = fread (*mainContent, sizeof(char), *contentSize, file);			// read the file content to char array

	fclose(file);
}

void Configurator :: Close(list<long> & positions)
{
	if (print_results == true) {
		FILE *result = fopen(settings -> resultFile, "wb");
		if (! positions.empty()) {
			while(! positions.empty()){		// write all the positions values
				fprintf(result, "Position: %ld\r\n", positions.front());
				positions.pop_front();		// traverse the list: print the front and delete.
			}
		}
		else {
			fprintf(result, "%s\r", "Pattern not found");
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
	static const int stateDimensionChars = 65534;				// chars count

	Configurator	 *dwarfConfigurator;
	list<long> 		  positions;								// list for storing of found positions

	int 			**states;									// table of states
    int 			  stateDimensionCoincidence;				// size of pattern
    mychar 			 *patternContent;							// pattern content
	long 			  contentSize;								// size of mainContent string
	mychar 			 *mainContent;								// content to be parsed

	void ConstructStates();										// method for constructing the states
	void StatesProcessing();									// processing of content
};



Solver :: Solver(Configurator* configurator)
{
	dwarfConfigurator = configurator;							// process settings from the command line
	dwarfConfigurator -> WriteSettings();
	dwarfConfigurator -> GetContent(&mainContent, &contentSize, &patternContent, &stateDimensionCoincidence);
}

Solver :: ~Solver() {
	delete[] mainContent;
	for (int i = 0; i < stateDimensionChars; i++)
		delete[] states[i];
	delete[] states;
}

void Solver::ConstructStates()
{
	states = new int*[stateDimensionChars];

	for (int i = 0; i < stateDimensionChars; i++) {
		states[i] = new int[stateDimensionCoincidence + 1];
	}

	for (int i = 0; i < stateDimensionCoincidence + 1; i++) {
		for (int j = 0; j < stateDimensionChars; j++) {
			states[j][i] = 0;
		}
    }

    for (int i = 0; i < stateDimensionCoincidence; i++) {
		states[(int) patternContent[0]][i] = 1;						// all turns from any char to the the first char in pattern are "initial" (=1)
    }

	for (int i = 1; i < stateDimensionCoincidence; i++) {
		int currentState = states[(int) patternContent[i]] [i];
        if (currentState > 0) {										// cycle-in-pattern checker (two or more same characters in a row)
			states[(int) patternContent[currentState]] [i + 1] = currentState + 1;    // if cycle found, add "cycle" state
        }
 		states[(int) patternContent[i]] [i] = i + 1;			// all turns from i-char to the (i+1)-char are "next" (equals to i+1)
    }

	delete[] patternContent;										// pattern won't be used later
}

pthread_spinlock_t m;

void Solver::StatesProcessing()
{
	pthread_spin_init(&m, 0);
	long partSize = max( contentSize / (num_threads * 100 ), (long) 2 * stateDimensionCoincidence);
	parallel_for(blocked_range<size_t>(0,contentSize,partSize), [&](const blocked_range<size_t>& r) {
		int state = 0;
		unsigned long start = r.begin();
		unsigned long end = r.end() + stateDimensionCoincidence;
		if ( end > (unsigned long) contentSize) end = (unsigned long) contentSize;
		for (unsigned long i = start; i < end; ++i) {
			if (mainContent[i] > 0) {
				state = states[(int) mainContent[i]][state];		// change the state
			}

			if (state == stateDimensionCoincidence)	{				// pattern found!

				// TRANSACTION_BEGIN(1,RW)

				// PushBack(i - stateDimensionCoincidence);	// push to list
				pthread_spin_lock(&m);
				positions.push_back(i - stateDimensionCoincidence);	// push to list
				pthread_spin_unlock(&m);

				// TRANSACTION_END

				state = 0;
			}
		}
	});
}



void Solver::Solve()
{
	ConstructStates();								// construct the array of states
	StatesProcessing();							    // traverse the mainContent using states array
}

void Solver::PrintSummery()
{
//	positions.sort();
	if (! positions.empty()) {
		printf("\n\nTotal count of pattern entries: %d \n", (int) positions.size());
	}
	else {
		printf("\n\nPattern not found\n");
	}
}


// Problem results output
void Solver::Finish()
{
	dwarfConfigurator -> Close(positions);
}





/*
====================================================================================
	ParseCommandLine
====================================================================================
*/

void ParseCommandLine(int argc, char **argv)
{
	char c;

	input_file_name = (char*) "smallcontent.txt";
	num_threads = 1;
	print_results = false;

	while ((c = getopt (argc, argv, "hf:n:r")) != -1)
		switch (c) {
			case 'h':
				printf("\nFinite State Machine - ADF benchmark application\n"
				"\n"
				"Usage:\n"
				"   finite_state_machine_tbb [options ...]\n"
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

	printf ("\nStarting TBB Finite State Machine.\n");
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
