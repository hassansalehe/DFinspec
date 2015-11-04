#include <cstdio>
#include <getopt.h>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdint.h>
#include <cstdlib>

#include "adf.h"

using namespace std;

typedef char mychar;

bool  print_results;
char *input_file_name;


// Size of block for reading from file
#define MAX_BLOCK_SIZE 1024 * 1024 //1 mb


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
	sprintf(resultFile, "result_seq.txt");

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


// the algorithm counts the number of '1' bits in a bit string.
void Solver::Solve()
{
    uint64_t *buffer = new uint64_t[MAX_BLOCK_SIZE];    	// Working buffer
    uint64_t count = 0;                                   	// Counter
    unsigned int len;                               		// Lenght of block

    //Loop while not End Of File.
    while (!feof(input))
    {
        //Read block of data from file.
        len = (unsigned int) fread((void*) buffer, 8, MAX_BLOCK_SIZE, input);

        if (len < MAX_BLOCK_SIZE)
        {
            *(buffer + len) = *(buffer + len) << 8 * (8 - length % 8);

            len ++;
        }

        //Loop for length of block.
        while (len --)
		{

			//This line of code is used for decrease of input file size.
			//Such modification would reduce the input file size,
			//but the algorithm is still belongs dwarf.
			//Also this line does not affect the result.
			for (int k = 0; k < 1000000; k ++)

			{
				//Calculate number of '1' in current uint64 of current block.
				count = *(buffer + len);

				count -= (count >> 1) & 0x5555555555555555;
				count = (count & 0x3333333333333333) + ((count >> 2) & 0x3333333333333333);
				count = (count + (count >> 4)) & 0x0f0f0f0f0f0f0f0f;
			}

			resultCount += count * 0x0101010101010101 >> 56;
		}
    }

    delete[] buffer;
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

	input_file_name = (char*) "smallcontent.txt"; // NULL;
	print_results = false;

	while ((c = getopt (argc, argv, "hf:r")) != -1)
		switch (c) {
			case 'h':
				printf("\nCombinatorial Logic - ADF benchmark application\n"
				"\n"
				"Usage:\n"
				"   combinatorial_logic_seq [options ...]\n"
				"\n"
				"Options:\n"
				"   -h\n"
				"        Print this help message.\n"
				"   -f <filename>\n"
				"        Input file name.\n"
				"   -r \n"
				"        print results to a result file\n"
				"\n"
				);
				exit (0);
			case 'f':
				input_file_name = optarg;
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

	printf ("\nStarting sequential Combinatorial Logic.\n");
	printf ("Input file %s\n", input_file_name);
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

	sequential_init();

	solver.Solve();										// Solve the current problem

	solver.PrintSummery();

	sequential_terminate();

	solver.Finish();									// write results

	return 0;
}




