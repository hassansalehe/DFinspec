#include <cstdlib>
#include <cstdio>
#include <getopt.h>
#include <cmath>
#include <cstring>
#include <iostream>

#include "tbb/task_scheduler_init.h"
#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"

#include "adf.h"

using namespace std;
using namespace tbb;

typedef char mychar;

bool     print_results;
char    *input_file_name;
int      num_threads;
int      matrix_size = 512;
double   epsilon = 1.0;


#define MAX(A,B) A>B ? A : B

#define OPTIMAL



/*
======================================================================
	Solver
======================================================================
*/


class Solver
{
public:
	Solver(int _size, double _epsilon);
	~Solver();
	void Solve();
	void PrintSummery();
	void Finish();

    int        size;                // matrix size
	long       iterations;			// Iterations till convergence
    double     epsilon;         	// Epsilon.
    double   **grid;          		// Initial matrix.
    double   **tempGrid;      		// Temp matrix.
    double   **result;        		// Result matrix.

    double    *grid_matrix;
    double    *tempGrid_matrix;

};


//Init arrays.
Solver :: Solver(int _size, double _epsilon)
{
    grid = NULL;
    tempGrid = NULL;
    result = NULL;

    size = _size;
    epsilon = _epsilon;
    iterations = 0;

    //Init matrices.
    grid_matrix = new double[size*size];
    tempGrid_matrix = new double[size*size];

    grid = new double*[size];
    tempGrid = new double*[size];
    for (int j = 0; j < size; j ++) {
        grid[j] = & grid_matrix[j*size];
        tempGrid[j] = & tempGrid_matrix[j*size];
    }

    int val;

    for (int i = 0; i < size; i ++) {
        for (int j = 0; j < size; j ++) {
            val = rand()%1000;
            grid[i][j] = (double) val;
            tempGrid[i][j]  = (double) val;
        }
    }
}

//Dispose arrays
Solver :: ~Solver()
{
	delete [] grid_matrix;
	delete [] tempGrid_matrix;

    delete [] grid;
    delete [] tempGrid;
}

// Solution of Laplace's equation with the Dirichlet's conditions on the square.
// To obtain solution, iterated Jacobi method is applied. It consists of steps like:
// newgrid[i,j] = (grid[i-1,j] + grid[i+1,j] + grid[i,j - 1] + grid[I,j + 1]) / 4
void Solver::Solve()
{
	bool converged = false;
	static double error = 0.0;
	static affinity_partitioner ap;

	while(!converged) {

		parallel_for(	blocked_range<size_t>(1, size - 1, size / num_threads),
						[&] (const blocked_range<size_t>& r) {
			double lerror = 0.0;
			for (unsigned int i = r.begin(); i != r.end(); i++) {
				for (int j = 1; j < size - 1; j ++) {
					//Perform step of computation on current node
					tempGrid[i][j] = 0.25 * (grid[i-1][j] + grid[i+1][j] + grid[i][j-1] + grid[i][j+1]);
					lerror = MAX(lerror, fabs(tempGrid[i][j] - grid[i][j]));
				}
			}


			TRANSACTION_BEGIN(1,RW)
				error = MAX(error, lerror);
			TRANSACTION_END

		}, ap);

		if(error <= epsilon)
			converged = true;
		error = 0.0;
		iterations++;
		double ** t = grid;
		grid = tempGrid;
		tempGrid = t;
	}

	result = grid;
}


void Solver::PrintSummery()
{
	printf("\nIterations: %ld\n", iterations);
}


void Solver::Finish()
{
	if (print_results == true) {
		FILE *result_file = fopen("result_tbb.txt", "wb");
		fprintf(result_file, "#Iterations: %ld #Result domain:\r\n", iterations);
		for (int i = 0; i < size; i ++)
		{
			for (int j = 0; j < size; j ++)
			{
				fprintf(result_file, "%0.8f ", result[i][j]);
			}
			fprintf(result_file, "\r\n");
		}
		fclose(result_file);
	}
}


/*
====================================================================================
	ParseCommandLine
====================================================================================
*/

void ParseCommandLine(int argc, char **argv)
{
	char c;

	input_file_name = (char*) "mediumcontent.txt";
	num_threads = 1;
	print_results = false;

	while ((c = getopt (argc, argv, "hf:n:s:e:r")) != -1)
		switch (c) {
			case 'h':
				printf("\nStructured Grid - ADF benchmark application\n"
				"\n"
				"Usage:\n"
				"   structured_grid_tbb [options ...]\n"
				"\n"
				"Options:\n"
				"   -h\n"
				"        Print this help message.\n"
				"   -f <filename>\n"
				"        Input file name.\n"
				"   -s <int>\n"
				"        Size of matrix.\n"
				"   -e <double>\n"
				"        Epsilon value.\n"
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
			case 's':
				matrix_size = atoi(optarg);
				break;
			case 'e':
				epsilon = atof(optarg);
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

	printf ("\nStarting TBB Structured Grid.\n");
	printf ("Matrix size: %d\tepsilon: %g\n", matrix_size, epsilon);
	printf ("Running with %d threads.\n", num_threads);
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

	Solver solver(matrix_size, epsilon);				// create new Solver (current problem with initial data

	NonADF_init(num_threads);

task_scheduler_init init(num_threads);

	solver.Solve();										// Solve the current problem

	solver.PrintSummery();

	NonADF_terminate();

	solver.Finish();									// write results

	return 0;
}


