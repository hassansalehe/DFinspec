#include <cstdio>
#include <getopt.h>
#include <cmath>
#include <cstring>
#include <iostream>

#include "adf.h"

using namespace std;

typedef char mychar;

bool  print_results;
char *input_file_name;
int   num_threads;

#define OPTIMAL

// Size of block for reading from file
#define MAX_BLOCK_SIZE 64 //1 mb


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
	static const int maxArraySize = 1024;
};


Settings::Settings(char *input_file)
{
	dwarfName = new char[maxArraySize];
    inputFile = new char[maxArraySize];
	resultFile = new char[maxArraySize];

	dwarfName[0] = inputFile[0] = resultFile[0] = '\0';

	sprintf(dwarfName, "%s", "CombinatorialLogic");
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
	void Close(unsigned long length, unsigned long resultCount);

	Settings* settings;
private:

};


void Configurator::Close(unsigned long length, unsigned long resultCount)
{
	if (print_results == true) {
		FILE *result = fopen(settings -> resultFile, "wb");

		fprintf(result, "File length (byte) : %lu\n", length);
		fprintf(result, "Result count: %lu\n", resultCount);

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

    unsigned long length;       //Length of file
    unsigned long resultCount;  //Result counter
    FILE 		  *input;

    struct BufferInfo {
    	unsigned int  	len;
    	uint64_t 		buffer[MAX_BLOCK_SIZE];
    };

    /* tokens */
    int readToken;
    BufferInfo *calculateToken;

    // ADF task functions
    void ReadTask();
    void CalculateTask();
    void InitialTask();
};

Solver::Solver(Configurator* configurator) {
	dwarfConfigurator = configurator;
	input = fopen(configurator->settings->inputFile, "rb");		// Opening the input file

	//Get length of file.
	fseek(input, 0, SEEK_END );
	length = ftell(input);
	rewind(input);

	resultCount = 0;
}

Solver::~Solver() {
    if (input)
    	fclose(input);
}




void Solver::ReadTask()
{
	void *intokens[] = {&readToken};
	void *outtokens[] = {&calculateToken, &readToken};
	adf_create_task(1, 1, intokens, [=] (token_t *tokens) -> void
	{
		TRACE_EVENT(12001,121)
		/* TASK START */
		/* recreate context */

		/* redeclare input tokens */
		decltype(readToken) readToken;
		memcpy(&readToken, tokens->value, sizeof(readToken));

		/* redeclare output token data as local variables */
		decltype(calculateToken) calculateToken;

		/* declare local data */
		int exit = 0;	/* the local exit condition flag for use outside of critical section */

		/* UNTIL clause */
		// can't use transaction because feof  is not transaction safe (I could use transaction_relaxed)
//		TRANSACTION_BEGIN(4,RO)
				if (feof(input))
					exit = 1;
//		TRANSACTION_END

		if (exit) {
			adf_task_stop();
			TRACE_EVENT(12001,0)
			return;
		}

		/* TASK BODY */

		STAT_COUNT_EVENT(task_exec, GetThreadID());
		STAT_START_TIMER(task_body, GetThreadID());

#ifndef OPTIMAL
		TRANSACTION_BEGIN(5,RW)
#endif

		//Read block of data from file.
		calculateToken = new BufferInfo();
		calculateToken->len = (unsigned int) fread((void*) calculateToken->buffer, 8, MAX_BLOCK_SIZE, input);

		if (calculateToken->len < MAX_BLOCK_SIZE) {
			*(calculateToken->buffer + calculateToken->len) = *(calculateToken->buffer + calculateToken->len) << 8 * (8 - length % 8);
			calculateToken->len ++;
		}

		readToken++;

#ifndef OPTIMAL
		TRANSACTION_END
#endif

		STAT_STOP_TIMER(task_body, GetThreadID());

		/* TASK END */
		adf_pass_token(outtokens[0], &calculateToken, sizeof(calculateToken));    /* pass tokens */
		adf_pass_token(outtokens[1], &readToken, sizeof(readToken));    /* pass tokens */

		TRACE_EVENT(12001,0)
	});

}


void Solver::CalculateTask()
{
	void *intokens[] = {&calculateToken};
	adf_create_task(num_threads*2, 1, intokens, [=] (token_t *tokens) -> void
	{
		TRACE_EVENT(12002,122)

		/* TASK START */
		/* recreate context */

		/* redeclare input tokens */
		decltype(calculateToken) calculateToken;
		memcpy(&calculateToken, tokens->value, sizeof(calculateToken));

		/* TASK BODY */

		STAT_COUNT_EVENT(task_exec, GetThreadID());
		STAT_START_TIMER(task_body, GetThreadID());

#ifndef OPTIMAL
		TRANSACTION_BEGIN(5,RW)
#endif

	    uint64_t count = 0;
		unsigned int len = calculateToken->len;

		//Loop for length of block.
		while (len --) {

			//This line of code is used for decrease of input file size.
			//Such modification would reduce the input file size,
			//but the algorithm still belongs dwarf.
			//Also this line does not affect the result.
			for (int k = 0; k < 1000000; k ++) {
				//Calculate number of '1' in current uint64 of current block.
				count = *(calculateToken->buffer + len);
				count -= (count >> 1) & 0x5555555555555555;
				count = (count & 0x3333333333333333) + ((count >> 2) & 0x3333333333333333);
				count = (count + (count >> 4)) & 0x0f0f0f0f0f0f0f0f;
			}

#ifdef OPTIMAL
			TRANSACTION_BEGIN(1,RW)
#endif
			resultCount += count * 0x0101010101010101 >> 56;

#ifdef OPTIMAL
			TRANSACTION_END
#endif
		}

		delete calculateToken;

#ifndef OPTIMAL
		TRANSACTION_END
#endif

		STAT_STOP_TIMER(task_body, GetThreadID());
		/* TASK END */

		TRACE_EVENT(12002,0)
	});
}


void Solver::InitialTask()
{
	static int tokenNum = 0;

	void *outtokens[] = {&readToken};
	adf_create_task(1, 0, NULL, [=](token_t *tokens) -> void
	{
		TRACE_EVENT(12003,123)

		/* TASK START */
		/* recreate context */

		/* redeclare output token data as local variables */
		decltype(readToken) readToken;

		/* declare local data */
		int exit = 0;	/* the local exit condition flag for use outside of critical section */

		/* UNTIL clause */
		TRANSACTION_BEGIN(1,RO)
				if (TMREAD(tokenNum) >= 1)
					exit = 1;
		TRANSACTION_END

		if (exit) {
			adf_task_stop();
			TRACE_EVENT(12003,0)
			return;
		}

		/* TASK BODY */
		TRANSACTION_BEGIN(2,RW)
			TMWRITE(tokenNum, TMREAD(tokenNum) + 1);
			readToken = 1;
		TRANSACTION_END

		/* TASK END */
		adf_pass_token(outtokens[0], &readToken, sizeof(readToken));    /* pass tokens */

		TRACE_EVENT(12003,0)
	});
}


// the algorithm counts the number of '1' bits in a bit string.
void Solver::Solve()
{
	ReadTask();
	CalculateTask();
	InitialTask();
}

void Solver::PrintSummery()
{
    printf("File length (byte) : %lu\n", length);
    printf("Result count: %lu\n", resultCount);
}


void Solver::Finish()		// Problem results output
{
	dwarfConfigurator -> Close(length, resultCount);
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
				printf("\nCombinatorial Logic - ADF benchmark application\n"
				"\n"
				"Usage:\n"
				"   combinatorial_logic_adf [options ...]\n"
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

	printf ("\nStarting ADF Combinatorial Logic.\n");
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

