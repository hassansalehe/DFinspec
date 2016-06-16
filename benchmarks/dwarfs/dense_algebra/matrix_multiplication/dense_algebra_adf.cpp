#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "adf.h"

typedef struct indexes_s {
  long indexA;
  long indexB;
  long indexC;
} indexes_t;

/* command line parameters */
long BSIZE = 100;
long DIM = 10;
int  num_threads = 20;
int  num_tasks = 32;

/* globals */
long NB, N, NN;
float **A;
float **B;
float **C;

/* tokens */
indexes_t index_token;
long k_token;
void *index_token_addr;

long **c_Token;

#define OPTIMAL

/*
====================================================================================
	convert_to_blocks
====================================================================================
*/
static void convert_to_blocks(float *Alin, float **A)
{
  long i, j;

  for (i = 0; i < N; i++) {
    for (j = 0; j < N; j++) {
      A[i/NB*DIM + j/NB][(i%NB)*NB+j%NB] = Alin[j*N+i];
    }
  }

}

void fill_random(float *Alin)
{
  long i;
  for (i = 0; i < NN; i++) {
    Alin[i]=((float)rand())/((float)RAND_MAX);
  }
}

/*
====================================================================================
	InitMatrix
====================================================================================
*/
void InitMatrix()
{
  // matrix init
  N=BSIZE*DIM;
  NN=N*N;
  NB = BSIZE;

  printf("N = % ld\t NB = %ld\n", N, NB);

  // linear matrix
  float *Alin = (float *) malloc(NN * sizeof(float));
  float *Blin = (float *) malloc(NN * sizeof(float));
  float *Clin = (float *) malloc(NN * sizeof(float));

  // fill the matrix with random values
  long i;
  srand(0);
  fill_random(Alin);
  fill_random(Blin);
  for (i=0; i < NN; i++)
    Clin[i]=0.0;

  A = (float **) malloc(DIM*DIM*sizeof(float *));
  B = (float **) malloc(DIM*DIM*sizeof(float *));
  C = (float **) malloc(DIM*DIM*sizeof(float *));

  for ( i = 0; i < DIM*DIM; i++)
  {
     A[i] = (float *) malloc(BSIZE*BSIZE*sizeof(float));
     B[i] = (float *) malloc(BSIZE*BSIZE*sizeof(float));
     C[i] = (float *) malloc(BSIZE*BSIZE*sizeof(float));
  }
  convert_to_blocks(Alin, A);
  convert_to_blocks(Blin, B);
  convert_to_blocks(Clin, C);

  free(Alin);
  free(Blin);
  free(Clin);

}


/*
====================================================================================
	FreeMatrix
====================================================================================
*/
void FreeMatrix()
{
	long i;
	for ( i = 0; i < DIM*DIM; i++) {
		free(A[i]);
		free(B[i]);
		free(C[i]);
	}
	free(A);
	free(B);
	free(C);
}



/*
====================================================================================
	matmul
====================================================================================
*/
void matmul(float  *A, float *B, float *C)
{
	long i, j, k, I;
	float tmp;
	for (i = 0; i < NB; i++) {
		I=i*NB;
		for (j = 0; j < NB; j++) {
			tmp=C[I+j];
			for (k = 0; k < NB; k++) {
				tmp+=A[I+k]*B[k*NB+j];
			}
			C[I+j]=tmp;
		}
	}
}


void Worker(long i, long j) {
	void *intokens[] = {&c_Token[i][j]};
	void *outtokens[] = {&c_Token[i][j]};
	adf_create_task(1, 1, intokens, [=] (token_t *tokens) -> void
	{
		TRACE_EVENT(13001,131)

		/* TASK START */
		/* recreate context */

		/* redeclare input tokens as local variables */
		std::remove_reference<decltype(c_Token[i][j])>::type c_Token;
		memcpy(&c_Token, tokens->value, sizeof(c_Token));


		/* declare local data */;
		int exit = 0;	/* the local exit condition flag for use outside of critical section */

		/* UNTIL clause */
		TRANSACTION_BEGIN(4,RO)
		if (c_Token >= DIM)
			exit = 1;
		TRANSACTION_END

		if (exit) {
			adf_task_stop();
			TRACE_EVENT(13002,0)
			return;
		}

		STAT_COUNT_EVENT(task_exec, GetThreadID());

		/* TASK BODY */
#ifndef OPTIMAL
		TRANSACTION_BEGIN(1,RW)
#endif
		matmul (A[i*DIM + c_Token], B[c_Token*DIM + j], C[i*DIM + j]);
		c_Token++;
#ifndef OPTIMAL
		TRANSACTION_END
#endif

		adf_pass_token(outtokens[0], &c_Token, sizeof(c_Token));

		TRACE_EVENT(13001,0)
	}, 2, PIN, (i%num_threads) );
}

/*
====================================================================================
	CreateInitialTokens
====================================================================================
*/
void InitialTask ()
{
	int i,j;

	for (i = 0; i < DIM; i++) {
		for (j = 0; j < DIM; j++) {
			void *outtokens[] = {&c_Token[i][j]};
			adf_create_task(1, 0, NULL, [=](token_t *tokens) -> void
			{
				TRACE_EVENT(13002,132)

				/* TASK START */
				/* recreate context */

				/* redeclare tokens as local variables */
				std::remove_reference<decltype(c_Token[i][j])>::type c_Token;

				/* TASK BODY */
#ifndef OPTIMAL
				TRANSACTION_BEGIN(2,RW)
#endif
				c_Token = 0;
#ifndef OPTIMAL
				TRANSACTION_END
#endif

				adf_pass_token(outtokens[0], &c_Token, sizeof(c_Token));

				TRACE_EVENT(13002,0)

				adf_task_stop();
			}, 2, PIN, (i%num_threads) );
		}
	}

}

void Solve() {
	long i,j;

	c_Token = (long **) malloc(DIM*sizeof(long *));
	for (i = 0; i < DIM; i++)
		c_Token[i] = (long *) malloc(DIM * sizeof(long));

	InitialTask();

	for (i = 0; i < DIM; i++)
		for (j = 0; j < DIM; j++)
			Worker(i,j);


}


/*
====================================================================================
	ParseCommandLine
====================================================================================
*/
void ParseCommandLine(int argc, char **argv)
{
	char c;

	while ((c = getopt (argc, argv, "hd:b:n:t:")) != -1)
		switch (c) {
			case 'h':
				printf("\nMatrix multiplication - ADF benchmark application\n"
				"\n"
				"Usage:\n"
				"   matmul_adf [options ...]\n"
				"\n"
				"Options:\n"
				"   -h\n"
				"        Print this help message.\n"
				"   -d <long>\n"
				"        number of blocks in one dimension. (default 10)\n"
				"   -b <long>\n"
				"        Block size. (default 100)\n"
				"   -n <int>\n"
				"        Number of ADF worker threads. (default 1)\n"
				"   -t <int>\n"
				"        Total number of task instances of multiplication task. (default 32)\n"
				"\n"
				);
				exit (0);
			case 'd':
				DIM = atol(optarg);
				break;
			case 'b':
				BSIZE = atol(optarg);
				break;
			case 'n':
				num_threads = atoi(optarg);
				break;
			case 't':
				num_tasks = atoi(optarg);
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

	if (BSIZE < 1) BSIZE = 1;
	if (DIM < 1) DIM = 1;
	if (num_threads < 1) num_threads = 1;

	printf ("\nMatrix mulitplication\nMatrix size is %ldx%ld, devided into %ldx%ld blocks of size %ld.\n",
		DIM*BSIZE, DIM*BSIZE, DIM, DIM, BSIZE);
	printf ("Running with %d threads.\n", num_threads);
	printf ("=====================================================\n\n");
}


/*
====================================================================================
	main
====================================================================================
*/
int main (int argc, char **argv)
{
	ParseCommandLine(argc, argv);

	InitMatrix();

	adf_init(num_threads);
	adf_start();

	Solve();

	adf_start();

	adf_taskwait();
	adf_terminate();

	FreeMatrix();

	return 0;
}
