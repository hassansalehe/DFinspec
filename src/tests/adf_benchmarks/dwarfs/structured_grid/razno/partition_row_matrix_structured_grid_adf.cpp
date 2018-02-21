#include <stdio.h>
#include <getopt.h>
#include <math.h>
#include <string>
#include <iostream>

#include "adf.h"

using namespace std;

typedef char mychar;

bool     print_results;
char    *input_file_name;
int      num_threads;
int      num_tasks;
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

	struct SolveToken_s {
		double      **ingrid;
		double      **outgrid;
	};

	Solver();
	~Solver();
	void Solve();
	void PrintSummery();
	void Finish();

    int        size;                // matrix size
    int        num_rows;            // number of rows per task
	long       iterations;			// Iterations till convergence
    double     gerror;				// global error
    double   **grid;          		// Initial matrix.
    double   **tempGrid;      		// Temp matrix.
    double   **result;        		// Result matrix.

    double    *grid_matrix;
    double    *tempGrid_matrix;

    struct SolveToken_s   solveToken;

    void SolveTask();
    void InitialTask();
};


//Init arrays.
Solver :: Solver()
{
    grid = NULL;
    tempGrid = NULL;
    result = NULL;

    size = matrix_size;
    gerror = 0.0;
    iterations = 0;
    num_rows = matrix_size;

    //Init matrices.
    grid_matrix = new double[size*size];
    tempGrid_matrix = new double[size*size];

    grid = new double*[size];
    tempGrid = new double*[size];
    for (int i = 0; i < size; i ++) {
        grid[i] = & grid_matrix[i*size];
        tempGrid[i] = & tempGrid_matrix[i*size];
    }

    result = grid;

    solveToken.ingrid = NULL;
    solveToken.outgrid = NULL;

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

    delete  [] grid;
    delete  [] tempGrid;
}

void Solver::PrintSummery()
{
	printf("\nIterations: %ld\n", iterations);
}


void Solver::Finish()
{
	if (print_results == true) {
		FILE *result_file = fopen("result_adf.txt", "wb");
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

/* order of token assignment :
 * top, left, bottom, right
 */
void Solver::SolveTask() {
	static int task_count = 0;
	int t;

	num_rows = matrix_size / num_tasks;

	for (t = 0; t < num_tasks; t++) {
		void *intokens[] = {&solveToken};
		void *outtokens[] = {&solveToken};
		adf_create_task(1, 1, intokens, [=](token_t *tokens) -> void
		{
			TRACE_EVENT(19003,193)

			/* TASK START */
			/* recreate context */

			/* redeclare input tokens */
			decltype(solveToken) solveToken;
			memcpy(&solveToken, tokens->value, sizeof(solveToken));

			/* declare local data */
			bool generate_output = false;
			bool do_test = false;

			/* TASK BODY */
			STAT_COUNT_EVENT(task_exec, GetThreadID());
			STAT_START_TIMER(task_body, GetThreadID());

#ifndef OPTIMAL
			TRANSACTION_BEGIN(1,RW)
#endif
			double lerror = 0.0;
			int i, j;
			double **in_grid = solveToken.ingrid;
			double **out_grid = solveToken.outgrid;

			for (i = t * num_rows; i < (t+1) * num_rows; i++) {
				if (i == 0 || i == (size - 1))
					continue;
				for (j = 1; j < matrix_size -1; j++) {
					//Perform step of computation on current node
					out_grid[i][j] = 0.25 * (in_grid[i-1][j] + in_grid[i+1][j] + in_grid[i][j-1] + in_grid[i][j+1]);
					lerror = MAX (lerror, fabs(out_grid[i][j] - in_grid[i][j]));
				}
			}

#ifdef OPTIMAL
				TRANSACTION_BEGIN(1,RW)
#endif
				gerror = MAX(gerror, lerror);
				task_count++;
				if (task_count >= num_tasks) {
					task_count = 0;
					do_test = true;
				}

#ifdef OPTIMAL
			TRANSACTION_END
#endif

			if (do_test) {
				iterations++;
				if(gerror > epsilon) {
					generate_output = true;
					gerror = 0.0;
					if ((iterations % 2) == 0) {
						solveToken.ingrid = grid;
						solveToken.outgrid = tempGrid;
					}
					else {
						solveToken.ingrid = tempGrid;
						solveToken.outgrid = grid;
					}
				}
				else
					result = solveToken.outgrid;
			}


#ifndef OPTIMAL
		TRANSACTION_END
#endif

				STAT_STOP_TIMER(task_body, GetThreadID());

				/* TASK END */
				if (generate_output)
					adf_pass_token(outtokens[0], &solveToken, sizeof(solveToken));    /* pass tokens */

				TRACE_EVENT(19003,0)
			}, 2, PIN, t % num_threads);
	}
}


void Solver::InitialTask()
{
	void *outtokens[] = {&solveToken};
	adf_create_task(1, 0, NULL, [=](token_t *tokens) -> void
	{
		TRACE_EVENT(19001,191)

		/* TASK START */
		/* recreate context */

		/* redeclare output token data as local variables */
		decltype(solveToken) solveToken;

		/* TASK BODY */
		TRANSACTION_BEGIN(2,RW)
			solveToken.ingrid = grid;
			solveToken.outgrid = tempGrid;
		TRANSACTION_END

		/* TASK END */
		adf_pass_token(outtokens[0], &solveToken, sizeof(solveToken));    /* pass tokens */

		TRACE_EVENT(19001,0)

		// execute once clause
		adf_task_stop();
	});
}



// Solution of Laplace's equation with the Dirichlet's conditions on the square.
// To obtain solution, iterated Jacobi method is applied. It consists of steps like:
// newgrid[i,j] = (grid[i-1,j] + grid[i+1,j] + grid[i,j - 1] + grid[I,j + 1]) / 4

void Solver::Solve()
{
	SolveTask();
	InitialTask();
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
	num_tasks = 1;
	print_results = false;

	while ((c = getopt (argc, argv, "hf:n:s:e:t:r")) != -1)
		switch (c) {
			case 'h':
				printf("\nStructured Grid - ADF benchmark application\n"
				"\n"
				"Usage:\n"
				"   structured_grid_adf [options ...]\n"
				"\n"
				"Options:\n"
				"   -h\n"
				"        Print this help message.\n"
				"   -f <filename>\n"
				"        Input file name.\n"
				"   -s <int>\n"
				"        Size of matrix.\n"
				"   -t <int>\n"
				"        Number of ADF tasks.\n"
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
			case 't':
				num_tasks = atoi(optarg);
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

	if (num_tasks < num_threads)
		num_tasks = num_threads;

	printf ("\nStarting ADF Structured Grid.\n");
	printf ("Matrix size: %d\tnum_tasks: %d\tepsilon: %g\n", matrix_size, num_tasks, epsilon);
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

	Solver solver;	// create new Solver (current problem with initial data)

	adf_init(num_threads);
	adf_start();

	solver.Solve();										// Solve the current problem

	/* start a dataflow execution */
	adf_go();

	/* wait for the end of execution */
	adf_taskwait();

	solver.PrintSummery();

	adf_terminate();

	solver.Finish();									// write results

	return 0;
}


