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

	struct Borders_s {
		double *top;
		double *bottom;
		double *right;
		double *left;
	};

	typedef struct Borders_s Borders;

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
    double   **result;        		// Result matrix.
    double  ***block;               // blocks of matrix
    double  ***tmp_block;           // blocks of matrix
    double    *grid_matrix;


    Borders  **solveToken;
    int        iterateToken;
    int        generateToken;
    Error      errorToken;

    void SolveTask();
    void GenerateTask();
    void InitialTask();
    void PrintInput();
};


void Solver :: PrintInput() {
	printf("\n");
    for (int i = 0; i < size; i ++) {
        for (int j = 0; j < size; j ++) {
        	printf("%g ", grid[i][j]);
        }
        printf("\n");
    }
    printf("\n");


    for (int i = 0; i < num_blocks; i++)
        for (int j = 0; j < num_blocks; j++)
        {
            for (int k = 0; k < block_size; k++) {
            	for (int m = 0; m < block_size; m++) {
            		printf("%g ", block[i][j][k*block_size + m]);
            	}
            	printf("\n");
            }

            printf("\n");
            printf("\n");
        }

    printf("\n");
    exit(0);
}


//Init arrays.
Solver :: Solver()
{
    grid = NULL;
    result = NULL;

    size = matrix_size;
    gerror = 0.0;
    iterations = 0;
    block_size = size / num_blocks;

    //Init matrices.
    grid_matrix = new double[size*size];

    grid = new double*[size];
    for (int j = 0; j < size; j ++) {
        grid[j] = & grid_matrix[j*size];
    }

    result = grid;
    int val;

    for (int i = 0; i < size; i ++) {
        for (int j = 0; j < size; j ++) {
            val = rand()%1000;
            grid[i][j] = (double) val;
        }
    }



    block = new double**[num_blocks];
    tmp_block = new double**[num_blocks];
    for (int j = 0; j < num_blocks; j ++) {
    	block[j] = new double*[num_blocks];
    	tmp_block[j] = new double*[num_blocks];
    }


    for (int i = 0; i < num_blocks; i++)
        for (int j = 0; j < num_blocks; j++)
        {
            block[i][j] = new double[block_size*block_size];
            tmp_block[i][j] = new double[block_size*block_size];
            for (int k = 0; k < block_size; k++)
            	for (int m = 0; m < block_size; m++) {
            		block[i][j][k*block_size + m] = grid[i*block_size + k][j*block_size + m];
            		tmp_block[i][j][k*block_size + m] = grid[i*block_size + k][j*block_size + m];
            	}
        }


    solveToken = new Borders*[num_blocks];
    for(int i = 0; i < num_blocks; i++)
    	solveToken[i] = new Borders[num_blocks];

}

//Dispose arrays
Solver :: ~Solver()
{
	delete [] grid_matrix;
    delete [] grid;
    for (int i = 0; i < num_blocks; i++) {
        for (int j = 0; j < num_blocks; j++)
        {
            delete [] block[i][j];
            delete [] tmp_block[i][j];
        }
		delete [] block[i];
		delete [] tmp_block[i];
    }

    delete [] block;
    delete [] tmp_block;
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

			std::vector<Borders*> addr;
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
				std::remove_reference<decltype(solveToken[bi-1][bj])>::type solveToken_top;
				std::remove_reference<decltype(solveToken[bi][bj-1])>::type solveToken_left;
				std::remove_reference<decltype(solveToken[bi+1][bj])>::type solveToken_bottom;
				std::remove_reference<decltype(solveToken[bi][bj+1])>::type solveToken_right;

				if (bi > 0 ) {
					memcpy(&solveToken_top, tokens->value, sizeof(solveToken_top));
					tokens = tokens->next_token;
				}
				if (bj > 0) {
					memcpy(&solveToken_left, tokens->value, sizeof(solveToken_left));
					tokens = tokens->next_token;
				}
				if (bi < (num_blocks-1)) {
					memcpy(&solveToken_bottom, tokens->value, sizeof(solveToken_bottom));
					tokens = tokens->next_token;
				}
				if (bj < (num_blocks-1)) {
					memcpy(&solveToken_right, tokens->value, sizeof(solveToken_right));
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
				double left, right, top, bottom;
				double *cell = NULL;

				double *in_block, *out_block;

				if ((block_iteration[bi*num_blocks + bj] % 2) == 0) {
					in_block = block[bi][bj];
					out_block = tmp_block[bi][bj];
				}
				else {
					in_block = tmp_block[bi][bj];
					out_block = block[bi][bj];
				}

				block_iteration[bi*num_blocks + bj] ++;
				if (block_iteration[bi*num_blocks + bj] % num_iterations != 0) {

					solveToken_self.top = NULL;
					solveToken_self.bottom = NULL;
					solveToken_self.right = NULL;
					solveToken_self.left = NULL;

					double *my_left = new double[block_size];
					double *my_right = new double[block_size];

					for (i = 0; i < block_size; i++) {
						if ((bi*block_size + i) == 0 || (bi*block_size + i) == (size-1))
							continue;
						double *this_top = (i==0) ?  solveToken_top.bottom : &in_block[(i-1)*block_size];
						double *this_bottom = (i==block_size-1) ?  solveToken_bottom.top : &in_block[(i+1)*block_size];

						for (j = 0; j < block_size; j++) {
							if ((bj*block_size + j) == 0 || (bj*block_size + j) == (size-1))
								continue;

							left=(j==0 ? solveToken_left.right[i] : in_block[i*block_size + j-1]);
							right=(j==(block_size-1) ? solveToken_right.left[i] : in_block[i*block_size + j+1]);

							//Perform step of computation on current node
							cell = &out_block[i*block_size + j];
							*cell = 0.25 * (this_top[j] + this_bottom[j] + left + right);  //(top + bottom + left + right);

							if (j == 0 || j == (num_blocks-1)) {
								if (j == 0) my_left[i] = *cell;
								if (j == (num_blocks-1)) my_right[i] = *cell;
							}
						}
					}

					if (bi > 0) {
						solveToken_self.top = new double[block_size];
						memcpy((void*) solveToken_self.top, (void*) &out_block[0], (size_t) block_size*sizeof(double));
					}
					if (bi < (num_blocks-1)) {
						solveToken_self.bottom = new double[block_size];
						memcpy((void*) solveToken_self.bottom, (void*) &out_block[block_size*(block_size-1)], (size_t) block_size*sizeof(double));
					}
					if (bj > 0)
						solveToken_self.left = my_left;
					else
						delete [] my_left;
					if (bj < (num_blocks-1))
						solveToken_self.right = my_right;
					else
						delete [] my_right;

					generate_solveToken = true;
				}
				else {
					for (i = 0; i < block_size; i++) {
						if ((bi*block_size + i) == 0 || (bi*block_size + i) == (size-1))
							continue;
						for (j = 0; j < block_size; j++) {
							if ((bj*block_size + j) == 0 || (bj*block_size + j) == (size-1))
								continue;

							left=(j==0 ? solveToken_left.right[i] : in_block[i*block_size + j-1]);
							top=(i==0 ? solveToken_top.bottom[j] : in_block[(i-1)*block_size + j]);
							right=(j==(block_size-1) ? solveToken_right.left[i] : in_block[i*block_size + j+1]);
							bottom=(i==(block_size-1) ? solveToken_bottom.top[j] : in_block[(i+1)*block_size + j]);

							//Perform step of computation on current node
							cell = &out_block[i*block_size + j];
							*cell = 0.25 * (top + bottom + left + right);
							lerror = MAX (lerror, fabs(*cell - in_block[i*block_size + j]));
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
					//std::cout << "Iterations = " << iterations << " error = " << gerror << " lerror[" << bi << ", " << bj << "] =" << lerror << std::endl;

					if (do_test) {
						iterations+= num_iterations;
						//std::cout << "Iterations = " << iterations << " error = " << gerror << std::endl;
						if(gerror > epsilon) {
							create_generateToken = true;
							generateToken = iterations;
							gerror = 0.0;
						}
						else
							result = grid;
					}
				}

				if (bj < (num_blocks-1)) delete [] solveToken_right.left;
				if (bj > 0) delete [] solveToken_left.right;
				if (bi > 0) delete [] solveToken_top.bottom;
				if (bi < (num_blocks-1)) delete [] solveToken_bottom.top;



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
				solveToken.top = bi==0 ?  NULL : new double[block_size];
				solveToken.bottom = bi==(num_blocks-1) ? NULL : new double[block_size];
				solveToken.right = bj==(num_blocks-1) ? NULL : new double[block_size];
				solveToken.left = bj==0 ? NULL : new double[block_size];

				double *curr_block;
				if ((generateToken % 2) == 0)
					curr_block = block[bi][bj];
				else
					curr_block = tmp_block[bi][bj];

				int i;

				for (i = 0; i < block_size; i++) {
					if (solveToken.top != NULL) solveToken.top[i] = curr_block[i];
					if (solveToken.bottom != NULL) solveToken.bottom[i] = curr_block[(block_size-1)*block_size + i];
					if (solveToken.left != NULL) solveToken.left[i] = curr_block[i*block_size];
					if (solveToken.right != NULL) solveToken.right[i] = curr_block[i*block_size + block_size - 1];
				}

				//std::cout << "Generate [" << bi << ", " << bj << "]" << std::endl;

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

	while ((c = getopt (argc, argv, "hf:n:s:e:b:i:r")) != -1)
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

	solver.Finish();

	return 0;
}


