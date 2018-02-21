#include <cstdio>
#include <getopt.h>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <set>

#include "adf.h"

using namespace std;

typedef char mychar;

bool  print_results;
char *input_file_name;
int   num_threads;

#define OPTIMAL

// Default size for buffer.
#define BUFFER_SIZE 1024

#define MAX(A,B) A>B ? A : B

//Structure for vertices
typedef set<unsigned int> SetOfVertex;

//Structure for neighbors of vertices
typedef map<unsigned int, SetOfVertex *> Neighborhood;

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

	sprintf(dwarfName, "%s", "UnstructuredGrid");
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

	static void getNextLine(char* str, FILE* file);
	void GetContent(unsigned int *numVertex, unsigned int *numTriangles, double *epsilon,
			double **value, double **tempValue, unsigned int **numNeighbors,
			unsigned int ***neighborhood, unsigned int ***triangles);
	void WriteSettings() { settings -> PrintStringSettings(); }
	void Close(long iterations, unsigned int numVertex, double *resultValue);

private:
	Settings* settings;
};


void Configurator :: getNextLine(char* str, FILE* file)
{
	short count = 0;
	char ch = ' ';

    while (!feof(file) && (ch == ' ' || ch == '\n' || ch == '\r')) {
        ch = (char)getc(file);
	}

    str[count] = ch;
	while (!feof(file) && ch != '\r' && ch != '\n' && ch != ' ') {
		str[++count] = (ch = (char)getc(file));
	}

	str[count] = '\0';
}


void Configurator :: GetContent(unsigned int *numVertex, unsigned int *numTriangles, double *epsilon,
								double **value, double **tempValue, unsigned int **numNeighbors,
								unsigned int ***neighborhood, unsigned int ***triangles)
{
	FILE *file = fopen(settings -> inputFile, "rb");		// Opening the input file
    char *str = new char[BUFFER_SIZE];

    //Get epsilon.
    getNextLine(str, file);
    *epsilon = atof(str);

    //Get power of graph.
    getNextLine(str, file);
    *numVertex = atoi(str);

    //Get number of vertices.
    getNextLine(str, file);
    *numTriangles = atoi(str);

    unsigned int i,j;

    //Init arrays.
    *value = new double[*numVertex];
    *tempValue = new double[*numVertex];

    (*triangles)[0] = new unsigned int[*numTriangles];
    (*triangles)[1] = new unsigned int[*numTriangles];
    (*triangles)[2] = new unsigned int[*numTriangles];

    *neighborhood = new unsigned int*[*numVertex];
    *numNeighbors = new unsigned int[*numVertex];

    i = j = 0;
    double val;

    //Get values of graph.
    while(!feof(file) && j < 1) {
        getNextLine(str, file);
        val = atof(str);
        (*value)[i] = val;
        (*tempValue)[i] = val;

        i ++;
        if (i >= *numVertex) {
            i = 0;
            j ++;
        }
    }

    i = j = 0;

    //Get triangles.
    while(!feof(file) && j < 3) {
        getNextLine(str, file);
        (*triangles)[j][i] = atoi(str);

        i ++;
        if (i >= *numTriangles) {
            i = 0;
            j ++;
        }
    }

    delete[] str;


	fclose(file);
}

void Configurator :: Close(long iterations, unsigned int numVertex, double *resultValue)
{
	if (print_results == true) {
		FILE *result = fopen(settings -> resultFile, "wb");
		fprintf(result, "#Iterations: %ld\n", iterations);
		fprintf(result, "#Result vector:\r\n");

		for (unsigned int j = 0; j < numVertex; j ++) {
			fprintf(result, "%0.8f ", resultValue[j]);
		}
		fprintf(result, "\r\n");
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

	long           iterations;			// Number of iterations until covnergence
    unsigned int   numVertex;     		// Power of graph
    unsigned int   numTriangles;  		// Number of triangles in grid

    double         epsilon;             // Epsilon

    double        *value;              	// Values of verteces
    double        *tempValue;          	// Temp array
    double        *resultValue;        	// Reference to result array

    unsigned int  *numNeighbors; 		// Number of neighbors
    unsigned int **neighborhood;		// Array for neighbors
    unsigned int **triangles;   		// Verteces of triangles

	unsigned int   num_solve_tasks;

private:
	Configurator	 *dwarfConfigurator;

	static inline void compute(unsigned int *vertices, unsigned int num, double *input, double *output);
	void constructNeighborhood();

	// tokens
	int 	solveToken;

	// tasks
	void SolveTask();
	void InitialTask();
};


Solver :: Solver(Configurator* configurator)
{
	iterations = 0;
    resultValue = 0;
    neighborhood = 0;
    numNeighbors = 0;
    value = 0;
    tempValue = 0;
    triangles = new unsigned int*[3];

    numVertex = 0;
    numTriangles = 0;

    num_solve_tasks = 0;

	dwarfConfigurator = configurator;					// process settings from the command line
	dwarfConfigurator -> WriteSettings();
	dwarfConfigurator -> GetContent(&numVertex, &numTriangles, &epsilon, &value, &tempValue,
									&numNeighbors, &neighborhood, &triangles);

	constructNeighborhood();
}

Solver :: ~Solver()
{
    delete triangles[0];
    delete triangles[1];
    delete triangles[2];

    for( unsigned int i = 0; i < numVertex; i ++)
    {
	    delete neighborhood[i];
    }

    delete numNeighbors;
    delete neighborhood;
    delete value;
    delete tempValue;
    delete triangles;
}


//Init & fill neighborhood array
void Solver::constructNeighborhood()
{
    //Init map
    Neighborhood *nhood = new Neighborhood();
    for (unsigned int i = 0; i < numVertex; i ++) {
        nhood->insert(make_pair( i, new SetOfVertex() ));
    }

    //Insert vertices into map
    SetOfVertex* sov;
    for (unsigned int j = 0; j < numTriangles; j ++) {
    	sov = (SetOfVertex*) (nhood->find(triangles[0][j])->second);
    	sov->insert(triangles[1][j]);
    	sov->insert(triangles[2][j]);

    	sov = (SetOfVertex*) (nhood->find(triangles[1][j])->second);
        sov->insert(triangles[0][j]);
        sov->insert(triangles[2][j]);

        sov = (SetOfVertex*) (nhood->find(triangles[2][j])->second);
        sov->insert(triangles[0][j]);
        sov->insert(triangles[1][j]);
    }

    //Copy from map to array
    for (unsigned int i = 0; i < numVertex; i ++) {
        SetOfVertex *vSet = (SetOfVertex*)(nhood->find(i)->second);

        numNeighbors[i] = (unsigned int) vSet->size();
        neighborhood[i] = new unsigned int[numNeighbors[i]];

        int j = 0;
        SetOfVertex::iterator it;
        for(  it = vSet->begin(); it != vSet->end(); it++ ) {
            neighborhood[i][j++] = *it;
	    }
    }

    //Dispose map
    Neighborhood::iterator iter;
    for( iter = nhood->begin(); iter != nhood->end(); iter++ ) {
	    delete iter->second;
    }
}


//Arithmetic mean of the neighbors' vertices
void Solver::compute(unsigned int *vertices, unsigned int num, double *input, double *output)
{
        *output = 0;
        for(unsigned int i = 0; i < num; i ++) {
            *output += input[vertices[i]];
	    }

        (num != 0) ? *output /= num : *output = 0;
}

// We have chosen two-dimensional iterative triangle grid.
// Function used on each vertex of the grid is arithmetic mean of the neighbors' vertices.
// The test of convergence performed is the comparison of difference of the same vertices on latest iterations to epsilon.


void Solver::SolveTask()
{
	static double gerror = 0;
	static unsigned int solve_count = 0;

	num_solve_tasks = num_threads;
	unsigned int chunkSize = numVertex / num_solve_tasks;

	if (chunkSize * num_solve_tasks < numVertex)
			chunkSize++;

	printf ("chunkSize = %d\tnum_solve_tasks = %d\n", chunkSize, num_solve_tasks);

	for (unsigned int  k = 0; k < numVertex; k += chunkSize)
	{
		void *intokens[] = {&solveToken};
		void *outtokens[] = {&solveToken};
		adf_create_task(1, 1, intokens, [=](token_t *tokens) -> void
		{
			TRACE_EVENT(20002,202)

			/* TASK START */
			/* recreate context */

			/* redeclare input tokens */
			std::remove_reference<decltype(solveToken)>::type solveToken;
			memcpy(&solveToken, tokens->value, sizeof(solveToken));

			/* declare local data */
			bool do_output = false;
			bool do_test = false;

			/* TASK BODY */
			STAT_COUNT_EVENT(task_exec, GetThreadID());
			STAT_START_TIMER(task_body, GetThreadID());

#ifndef OPTIMAL
			TRANSACTION_BEGIN(1,RW)
#endif
			double lerror = 0.0;
			unsigned int bound = ((k + chunkSize) > numVertex) ? numVertex : (k + chunkSize);

			//Loop for number of vertices in the chunk
			for (unsigned int i = k; i < bound; i ++) {
				compute(neighborhood[i], numNeighbors[i], value ,  tempValue + i);
				lerror = MAX (lerror, fabs(tempValue[i] - value[i]));
			}
#ifdef OPTIMAL
			TRANSACTION_BEGIN(1,RW)
#endif
			gerror = MAX(gerror, lerror);

			solve_count++;
			if (solve_count == num_solve_tasks) {
				solve_count = 0;
				do_test = true;
			}

#ifdef OPTIMAL
			TRANSACTION_END
#endif

			if (do_test) {

				double * t = value;
				value = tempValue;
				tempValue = t;

				if(gerror > epsilon) {
					solveToken = 1;
					do_output = true;
				}
				else
					resultValue = value;

				gerror = 0.0;
				iterations++;
			}

#ifndef OPTIMAL
			TRANSACTION_END
#endif

			STAT_STOP_TIMER(task_body, GetThreadID());

			/* TASK END */
			if (do_output)
				adf_pass_token(outtokens[0], &solveToken, sizeof(solveToken));    /* pass tokens */

			TRACE_EVENT(20002,0)
		}, 2, PIN, (k/chunkSize)%num_threads);
	}
}


void Solver::InitialTask()
{
	void *outtokens[] = {&solveToken};
	adf_create_task(1, 0, NULL, [=](token_t *tokens) -> void
	{
		TRACE_EVENT(20001,201)

		/* TASK START */
		/* recreate context */

		/* redeclare output token data as local variables */
		std::remove_reference<decltype(solveToken)>::type solveToken;

		/* TASK BODY */
		TRANSACTION_BEGIN(2,RW)
			solveToken = 1;
		TRANSACTION_END

		/* TASK END */
		adf_pass_token(outtokens[0], &solveToken, sizeof(solveToken));    /* pass tokens */

		TRACE_EVENT(20001,0)

		// execute once clause
		adf_task_stop();
	});
}


// Task based
void Solver::Solve()
{
	printf("\nnumVertex = %u\n", numVertex);

	SolveTask();
	InitialTask();
}


void Solver::PrintSummery()
{
	printf("\n\nIterations: %ld\n", iterations);
}


void Solver::Finish()
{
	dwarfConfigurator -> Close(iterations, numVertex, resultValue);
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
	input_file_name = (char*) "smallcontent.txt";
	num_threads = 1;
	print_results = false;

	while ((c = getopt (argc, argv, "hf:n:r")) != -1)
		switch (c) {
			case 'h':
				printf("\nUnstructured Grid - ADF benchmark application\n"
				"\n"
				"Usage:\n"
				"   unstructured_grid_adf [options ...]\n"
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

	printf ("\nStarting ADF Finite Unstructured Grid.\n");
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



