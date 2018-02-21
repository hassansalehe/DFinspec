#include <stdio.h>
#include <getopt.h>
#include <math.h>
#include <string>
#include <iostream>
#include <vector>

#include "adf.h"

using namespace std;

typedef char mychar;

bool     print_results;
char    *input_file_name;
int      num_threads;
int      matrix_size = 512;
double   epsilon = 1.0;
int      num_blocks;
int      num_iterations;


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

	struct Error {
		int iteration;
		double error;
	};

	Solver();
	~Solver();
	void Solve();
	void PrintSummery();
	void Finish();

    int        size;                // matrix size
    int        block_size;          // size of the block
	long       iterations;			// Iterations till convergence
    double     gerror;				// global error
    double   **grid;          		// Initial matrix.
    double   **tempGrid;      		// Temp matrix.
    double   **result;        		// Result matrix.
    double   **block;               // blocks of matrix

    double    *grid_matrix;
    double    *tempGrid_matrix;

    int      **solveToken;
    int        iterateToken;
    int        generateToken;
    Error      errorToken;

    void SolveTask();
    void GenerateTask();
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
    block_size = size / num_blocks;

    //Init matrices.
    grid_matrix = new double[size*size];
    tempGrid_matrix = new double[size*size];

    grid = new double*[size];
    tempGrid = new double*[size];
    for (int j = 0; j < size; j ++) {
        grid[j] = & grid_matrix[j*size];
        tempGrid[j] = & tempGrid_matrix[j*size];
    }

    result = grid;

    int val;

    for (int i = 0; i < size; i ++) {
        for (int j = 0; j < size; j ++) {
            val = rand()%1000;
            grid[i][j] = (double) val;
            tempGrid[i][j]  = (double) val;
        }
    }

    solveToken = new int*[num_blocks];
    for(int i = 0; i < num_blocks; i++)
    	solveToken[i] = new int[num_blocks];

}

//Dispose arrays
Solver :: ~Solver()
{
	delete [] grid_matrix;
	delete [] tempGrid_matrix;

    delete [] grid;
    delete [] tempGrid;
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
	int bi, bj, p;
	static int *block_iteration = new int[num_blocks * num_blocks]; // per block iteration counter
	static int block_count = 0;

	for (bi = 0; bi < num_blocks; bi++) {
		for (bj = 0; bj < num_blocks; bj++) {

			block_iteration[bi*num_blocks + bj] = 0;

			std::vector<int*> addr;
			if (bi > 0 )
				addr.push_back(&(solveToken[bi-1][bj]));
			if (bj > 0)
				addr.push_back(&(solveToken[bi][bj-1]));
			if (bi < (num_blocks-1))
				addr.push_back(&(solveToken[bi+1][bj]));
			if (bj < (num_blocks-1))
				addr.push_back(&(solveToken[bi][bj+1]));

			p = addr.size();
			void *intokens[p];
			for (uint it = 0; it < addr.size(); it++)
				intokens[it] = addr[it];
			void *outtokens [] = {&generateToken, &(solveToken[bi][bj])};

			adf_create_task(1, p, intokens, [=](token_t *tokens) -> void
			{
				TRACE_EVENT(19003,193)

				/* TASK START */
				/* recreate context */

				/* redeclare input tokens */
				std::remove_reference<decltype(solveToken[bi-1][bj])>::type _solveToken_1;
				std::remove_reference<decltype(solveToken[bi][bj-1])>::type _solveToken_2;
				std::remove_reference<decltype(solveToken[bi+1][bj])>::type _solveToken_3;
				std::remove_reference<decltype(solveToken[bi][bj+1])>::type _solveToken_4;

				if (bi > 0 ) {
					memcpy(&_solveToken_1, tokens->value, sizeof(_solveToken_1));
					tokens = tokens->next_token;
				}
				if (bj > 0) {
					memcpy(&_solveToken_2, tokens->value, sizeof(_solveToken_2));
					tokens = tokens->next_token;
				}
				if (bi < (num_blocks-1)) {
					memcpy(&_solveToken_3, tokens->value, sizeof(_solveToken_3));
					tokens = tokens->next_token;
				}
				if (bj < (num_blocks-1)) {
					memcpy(&_solveToken_4, tokens->value, sizeof(_solveToken_4));
					tokens = tokens->next_token;
				}


				/* redeclare output tokens */
				std::remove_reference<decltype(generateToken)>::type generateToken;
				std::remove_reference<decltype(solveToken[bi][bj])>::type solveToken_self;

				/* declare local data */
				bool create_generateToken = false;
				bool generate_solveToken = false;
				bool do_test = false;

				/* TASK BODY */
				STAT_COUNT_EVENT(task_exec, GetThreadID());
				STAT_START_TIMER(task_body, GetThreadID());

#ifndef OPTIMAL
				TRANSACTION_BEGIN(1,RW)
#endif
				double lerror = 0.0;
				int i, j;
				double **in_grid, **out_grid;

				if ((block_iteration[bi*num_blocks + bj] % 2) == 1) {
					in_grid = grid;
					out_grid = tempGrid;
				}
				else {
					in_grid = tempGrid;
					out_grid = grid;
				}

				block_iteration[bi*num_blocks + bj] ++;
				if (block_iteration[bi*num_blocks + bj] % num_iterations != 0) {
					for (i = bi*block_size; i < (bi+1)*block_size; i ++) {
						if (i == 0 || i == (size - 1))
							continue;
						for (j = bj*block_size; j < (bj+1)*block_size; j ++) {
							if ((j == 0) || (j == (size-1)))
								continue;
							//Perform step of computation on current node
							out_grid[i][j] = 0.25 * (in_grid[i-1][j] + in_grid[i+1][j] + in_grid[i][j-1] + in_grid[i][j+1]);
						}
					}
					solveToken_self = block_iteration[bi*num_blocks + bj];
					generate_solveToken = true;
				}
				else {
					for (i = bi*block_size; i < (bi+1)*block_size; i ++) {
						if (i == 0 || i == (size - 1))
							continue;
						for (j = bj*block_size; j < (bj+1)*block_size; j ++) {
							if ((j == 0) || (j == (size-1)))
								continue;
							//Perform step of computation on current node
							out_grid[i][j] = 0.25 * (in_grid[i-1][j] + in_grid[i+1][j] + in_grid[i][j-1] + in_grid[i][j+1]);
							lerror = MAX (lerror, fabs(out_grid[i][j] - in_grid[i][j]));
						}
					}
#ifdef OPTIMAL
					TRANSACTION_BEGIN(1,RW)
#endif
					gerror = MAX(gerror, lerror);
					block_count++;
					if (block_count >= num_blocks * num_blocks) {
						block_count = 0;
						do_test = true;
					}

#ifdef OPTIMAL
					TRANSACTION_END
#endif
					if (do_test) {
						iterations+= num_iterations;
						if(gerror > epsilon) {
							create_generateToken = true;
							generateToken = iterations;
							gerror = 0.0;
						}
						else
							result = out_grid;
					}
				}



#ifndef OPTIMAL
		TRANSACTION_END
#endif

				STAT_STOP_TIMER(task_body, GetThreadID());

				/* TASK END */
				if (create_generateToken)
					adf_pass_token(outtokens[0], &generateToken, sizeof(generateToken));    /* pass tokens */
				if (generate_solveToken)
					adf_pass_token(outtokens[1], &solveToken_self, sizeof(solveToken_self));    /* pass tokens */

				TRACE_EVENT(19003,0)
			}, 2 , PIN, bi%num_threads);

		}
	}
}



void Solver::GenerateTask() {
	int bi, bj;

	for (bi = 0; bi < num_blocks; bi++) {
		for (bj = 0; bj < num_blocks; bj++) {
			void *intokens[] = {&generateToken};
			void *outtokens[] = {&solveToken[bi][bj]};
			adf_create_task(1, 1, intokens, [=](token_t *tokens) -> void
			{
				TRACE_EVENT(19002,192)

				/* TASK START */
				/* recreate context */

				/* redeclare input token data as local variables */
				std::remove_reference<decltype(generateToken)>::type generateToken;
				memcpy(&generateToken, tokens->value, sizeof(generateToken));

				/* redeclare output token data as local variables */
				std::remove_reference<decltype(solveToken[bi][bj])>::type solveToken;

				/* TASK BODY */
				solveToken = generateToken;

				/* TASK END */
				adf_pass_token(outtokens[0], &solveToken, sizeof(solveToken));    /* pass tokens */

				TRACE_EVENT(19002,0)
			}, 2, PIN, bi%num_threads);

		}
	}
}


void Solver::InitialTask()
{
	void *outtokens[] = {&generateToken};
	adf_create_task(1, 0, NULL, [=](token_t *tokens) -> void
	{
		TRACE_EVENT(19001,191)

		/* TASK START */
		/* recreate context */

		/* redeclare output token data as local variables */
		std::remove_reference<decltype(generateToken)>::type generateToken;

		/* TASK BODY */
		generateToken = 0;

		/* TASK END */
		adf_pass_token(outtokens[0], &generateToken, sizeof(generateToken));    /* pass tokens */

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
	GenerateTask();
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
	num_blocks = 16;
	num_iterations = 50;
	print_results = false;

	while ((c = getopt (argc, argv, "hf:n:s:e:b:i:")) != -1)
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
				"   -b <int>\n"
				"        Number of blocks in one dimension.\n"
				"   -i <int>\n"
				"        Number of iterations between error check.\n"
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
			case 'b':
				num_blocks = atoi(optarg);
				break;
			case 'i':
				num_iterations = atoi(optarg);
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

	printf ("\nStarting ADF Structured Grid.\n");
	printf ("Matrix size: %d\tnum_blocks: %d\tepsilon: %g\n", matrix_size, num_blocks, epsilon);
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

	return 0;
}


