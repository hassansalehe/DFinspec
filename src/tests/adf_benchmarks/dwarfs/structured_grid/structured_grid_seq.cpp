#include <cstdlib>
#include <cstdio>
#include <getopt.h>
#include <cmath>
#include <cstring>
#include <iostream>

#include "adf.h"

using namespace std;

typedef char mychar;

bool     print_results;
char    *input_file_name;
int      matrix_size = 512;
double   epsilon = 1.0;

#define MAX(A,B) A>B ? A : B


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
	double error = 0.0;

    int i,j;

    while(!converged) {

        for (i = 1; i < size - 1; i ++) {
            for (j = 1; j < size - 1; j ++) {
                //Perform step of computation on current node
                tempGrid[i][j] = 0.25 * (grid[i-1][j] + grid[i+1][j] + grid[i][j-1] + grid[i][j+1]);
                error = MAX(error, fabs(tempGrid[i][j] - grid[i][j]));
		    }
		}

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
		FILE *result_file = fopen("result_seq.txt", "wb");
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
	print_results = false;

	while ((c = getopt (argc, argv, "hf:s:e:r")) != -1)
		switch (c) {
			case 'h':
				printf("\nStructured Grid - ADF benchmark application\n"
				"\n"
				"Usage:\n"
				"   structured_grid_seq [options ...]\n"
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

	printf ("\nStarting sequential Structured Grid.\n");
	printf ("Matrix size: %d\tepsilon: %g\n", matrix_size, epsilon);
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

	Solver solver(matrix_size, epsilon);				// create new Solver (current problem with initial data)

	sequential_init();

	solver.Solve();										// Solve the current problem

	solver.PrintSummery();

	sequential_terminate();

	solver.Finish();									// write results

	return 0;
}



