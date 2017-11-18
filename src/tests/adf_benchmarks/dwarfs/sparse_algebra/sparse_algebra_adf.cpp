#include <cstdio>
#include <getopt.h>
#include <cstring>
#include <iostream>

#include "adf.h"

using namespace std;

#define MIN(a,b) ((a<b) ? a : b)

#define OPTIMAL

typedef char mychar;

char *input_file_name;
int   num_threads;

int num_blocks = 3;//25;
int block_size = 2;//50;

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

	sprintf(dwarfName, "%s", "SparseAlgebra");
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

	printf("%s\n", stringSettings);

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
	void GetContent(float ***matrix);
	void WriteSettings() { settings -> PrintStringSettings(); }
	void Close();

private:
	Settings* settings;
};



void genmat (float *M[])
{
	int  init_val, i, j, ii, jj, num_null_entries = 0;
	bool null_entry;
	float *p;

	init_val = 1325;

	/* generating the structure */
	for (ii=0; ii < num_blocks; ii++)
	{
		for (jj=0; jj < num_blocks; jj++)
		{
			/* computing null entries */
			null_entry=false;
			if ((ii<jj) && (ii%3 !=0)) null_entry = true;
			if ((ii>jj) && (jj%3 !=0)) null_entry = true;
			if (ii%2==1) null_entry = true;
			if (jj%2==1) null_entry = true;
			if (ii==jj) null_entry = false;
			if (ii==jj-1) null_entry = false;
			if (ii-1 == jj) null_entry = false;
			/* allocating matrix */
			if (null_entry == false){
				M[ii*num_blocks+jj] =  new float[block_size*block_size];
				if (M[ii*num_blocks+jj] == NULL)
				{
					printf("Error: Out of memory\n");
					exit(1);
				}
				/* initializing matrix */
				p = M[ii*num_blocks+jj];
				for (i = 0; i < block_size; i++)
				{
					for (j = 0; j < block_size; j++)
					{
						init_val = (3125 * init_val) % 65536;
						(*p) = (float)((init_val - 32768.0) / 16384.0);
						p++;
					}
				}
			}
			else
			{
				num_null_entries++;
				M[ii*num_blocks+jj] = NULL;
			}
		}
	}
	printf("Number of null entries : %d\n", num_null_entries);
}


void Configurator :: GetContent(float ***matrix)
{
	*matrix = new float*[num_blocks*num_blocks];
	genmat(*matrix);
}

void Configurator :: Close()
{
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
	void InitSolver();
	void Solve();
	void Finish();

private:
	Configurator	 *dwarfConfigurator;
	float 			**matrix;

	float *allocate_clean_block();
	void lu0(float *diag);
	void bdiv(float *diag, float *row);
	void bmod(float *row, float *col, float *inner);
	void fwd(float *diag, float *col);

	// tokens
	int 	*luToken;		// vector of tokens lu[num_blocks]
	int 	*fwdToken;		// vector of tokens fwd[num_blocks]
	int 	*bdivToken;		// vector of tokens bdiv[num_blocks]
	int 	**bmodToken;	// matrix of tokens bmod[num_blocks, num_blocks]

//	static int 	*count;		// number of executions of each bmodTask

	// adf exit condition variable for bmod tasks
//	int 	*count;

	/* lu task creates lu token which triggers all fwd tasks in the corresponding row
	 * and all bdiv tasks in the corresponding column.
	 * fwd task creates fwd token for the corresponding row i.
	 * bdvi task creates bdiv token for the corresponding column j.
	 * bmod[i,j] task waits for fwd token from row i and bdiv token from column j.
	 * It can be triggered from 0 to num_blocks-1 times depending on its position in the matrix.
	 * bmod task creates bmod token that enables either lu or fwd and bdiv tasks.
	 * The initial tokens are bdiv[0,1..num_blocks] and bdiv[1..num_blocks, 0] for fwd and bdiv tasks
	 * in the first row and column of the block matrix, and bmod[0,0] for the lu[0] tasks.
	 */

	void luTask();
	void fwdTask();
	void bdivTask();
	void bmodTask();
	void InitialTask();
};


int 	*bmod_count;


Solver::Solver(Configurator* configurator)
{
	matrix = NULL;

	dwarfConfigurator = configurator;							// process settings from the command line
	dwarfConfigurator -> WriteSettings();
	dwarfConfigurator -> GetContent(&matrix);

	// tokens
	luToken = new int[num_blocks];
	fwdToken = new int[num_blocks];
	bdivToken = new int[num_blocks];
	bmodToken = new int*[num_blocks];
	for (int i=0; i<num_blocks; i++)
		bmodToken[i] = new int [num_blocks];

	InitSolver();
}

Solver::~Solver()
{
	delete [] matrix;

	delete [] luToken;
	delete [] fwdToken;
	delete [] bdivToken;
	for (int i=0; i<num_blocks; i++)
		delete [] bmodToken[i];
	delete [] bmodToken;

	delete [] bmod_count;
}


void Solver :: InitSolver()
{
	int ii, jj, kk;

	// Allocate currently NULL blocks for bmodTask
	for (kk=0; kk<num_blocks; kk++)
	{
		for (ii=kk+1; ii<num_blocks; ii++)
			if (matrix[ii*num_blocks+kk] != NULL)
				for (jj=kk+1; jj<num_blocks; jj++)
					if (matrix[kk*num_blocks+jj] != NULL)
					{
						if (matrix[ii*num_blocks+jj]==NULL)
							matrix[ii*num_blocks+jj] = allocate_clean_block();
					}
	}

	/* Calculate how many times each bmodTask needs to execute - this is optimization */
	bmod_count = new int[num_blocks*num_blocks];
	for (int i=1; i<num_blocks*num_blocks; i++)
		bmod_count [i] = 0;

	for (int ii=1; ii<num_blocks; ii++)
	{
		for (int jj= 1; jj<num_blocks; jj++)
		{
			for (int k=0; k<(MIN(ii, jj)); k++) {
					if (ii==2 && jj==2) {
						printf("ii*num_blocks + k    = %d\tmatrix = %p\n", ii*num_blocks + k, matrix[ii*num_blocks + k]);
						printf("k*num_blocks + jj    = %d\tmatrix = %p\n", k*num_blocks + jj, matrix[k*num_blocks + jj]);
					}
				if (matrix[ii*num_blocks + k] != NULL && matrix[k*num_blocks + jj] != NULL)
					bmod_count[ii*num_blocks + jj] ++;
			}
		}
	}
}

/***********************************************************************
 * allocate_clean_block:
 **********************************************************************/
float * Solver::allocate_clean_block()
{
	int i,j;
	float *p, *q;

	p = new float [block_size*block_size];
	q=p;
	if (p!=NULL){
		for (i = 0; i < block_size; i++)
			for (j = 0; j < block_size; j++){(*p)=0.0; p++;}

	}
	else {
		printf("Error: Out of memory\n");
	}
	return (q);
}
/***********************************************************************
 * lu0:
 **********************************************************************/
void Solver::lu0(float *diag)
{
	int i, j, k;

	for (k=0; k<block_size; k++)
		for (i=k+1; i<block_size; i++)
		{
			diag[i*block_size+k] = diag[i*block_size+k] / diag[k*block_size+k];
			for (j=k+1; j<block_size; j++)
				diag[i*block_size+j] = diag[i*block_size+j] - diag[i*block_size+k] * diag[k*block_size+j];
		}
}

/***********************************************************************
 * bdiv:
 **********************************************************************/
void Solver::bdiv(float *diag, float *row)
{
	int i, j, k;
	for (i=0; i<block_size; i++)
		for (k=0; k<block_size; k++)
		{
			row[i*block_size+k] = row[i*block_size+k] / diag[k*block_size+k];
			for (j=k+1; j<block_size; j++)
				row[i*block_size+j] = row[i*block_size+j] - row[i*block_size+k]*diag[k*block_size+j];
		}
}
/***********************************************************************
 * bmod:
 **********************************************************************/
void Solver::bmod(float *row, float *col, float *inner)
{
	int i, j, k;
	for (i=0; i<block_size; i++)
		for (j=0; j<block_size; j++)
			for (k=0; k<block_size; k++)
				inner[i*block_size+j] = inner[i*block_size+j] - row[i*block_size+k]*col[k*block_size+j];
}
/***********************************************************************
 * fwd:
 **********************************************************************/
void Solver::fwd(float *diag, float *col)
{
	int i, j, k;
	for (j=0; j<block_size; j++)
		for (k=0; k<block_size; k++)
			for (i=k+1; i<block_size; i++)
				col[i*block_size+j] = col[i*block_size+j] - diag[i*block_size+k]*col[k*block_size+j];
}



void Solver::luTask()
{
	for (int kk=0; kk<num_blocks; kk++)
	{
		void *intokens[] = {&bmodToken[kk][kk]};
		void *outtokens[] = {&luToken[kk]};
		adf_create_task(1, 1, intokens, [=] (token_t *tokens) -> void
		{
			TRACE_EVENT(16002,162)

			/* TASK START */
			/* recreate context */

			/* redeclare input tokens */
			std::remove_reference<decltype(bmodToken[kk][kk])>::type bmodToken;
			memcpy(&bmodToken, tokens->value, sizeof(bmodToken));

			/* redeclare input tokens */
			std::remove_reference<decltype(luToken[kk])>::type luToken;

			/* TASK BODY */

			STAT_COUNT_EVENT(task_exec, GetThreadID());
			STAT_START_TIMER(task_body, GetThreadID());

#ifndef OPTIMAL
			//~ CHECKSWITCHPOINT
			TRANSACTION_BEGIN(1,RW)
#endif
			printf("lu %d\n", kk);
			
			lu0(matrix[kk*num_blocks+kk]);
			luToken = kk;

#ifndef OPTIMAL
			TRANSACTION_END
			//~ CHECKSWITCHPOINT
#endif

			STAT_STOP_TIMER(task_body, GetThreadID());

			/* TASK END */
			if (kk < num_blocks - 1)	// do not produce luToken for the last block
				adf_pass_token(outtokens[0], &luToken, sizeof(luToken));    /* pass tokens */

			TRACE_EVENT(16002,0)

			/* execute once clause */
			adf_task_stop();
		});
	}
}





void Solver::fwdTask()
{
	for (int kk=0; kk<num_blocks; kk++)
	{
		for (int jj=kk+1; jj<num_blocks; jj++)
		{
			if (matrix[kk*num_blocks+jj] == NULL)
				continue;

			void *intokens[] = {&luToken[kk], &bmodToken[kk][jj]};
			void *outtokens[] = {&fwdToken[jj]};
			adf_create_task(1, 2, intokens, [=] (token_t *tokens) -> void
			{
				TRACE_EVENT(16003,163)

				/* TASK START */
				/* recreate context */

				/* redeclare input tokens */
				std::remove_reference<decltype(luToken[kk])>::type luToken;
				memcpy(&luToken, tokens->value, sizeof(int));
				tokens = tokens->next_token;
				std::remove_reference<decltype(bmodToken[kk][jj])>::type bmodToken;
				memcpy(&bmodToken, tokens->value, sizeof(int));

				/* redeclare input tokens */
				std::remove_reference<decltype(fwdToken[kk])>::type fwdToken;

				/* TASK BODY */

				STAT_COUNT_EVENT(task_exec, GetThreadID());
				STAT_START_TIMER(task_body, GetThreadID());

#ifndef OPTIMAL
				//~ CHECKSWITCHPOINT
				TRANSACTION_BEGIN(2,RW)
#endif
				printf("fwd [%d, %d]\n", kk, jj);
				
				if (matrix[kk*num_blocks+jj] != NULL)
					fwd(matrix[kk*num_blocks+kk], matrix[kk*num_blocks+jj]);
				fwdToken = kk*num_blocks+jj;

#ifndef OPTIMAL
				TRANSACTION_END
				//~ CHECKSWITCHPOINT
#endif

				STAT_STOP_TIMER(task_body, GetThreadID());

				/* TASK END */
				adf_pass_token(outtokens[0], &fwdToken, sizeof(fwdToken));    /* pass tokens */

				/* execute once clause */
				adf_task_stop();

				TRACE_EVENT(16003,0)
			});
		}
	}
}




void Solver::bdivTask()
{
	for (int kk=0; kk<num_blocks; kk++)
	{
		for (int ii=kk+1; ii<num_blocks; ii++)
		{
			if (matrix[ii*num_blocks+kk] == NULL)
				continue;

			void *intokens[] = {&luToken[kk], &bmodToken[ii][kk]};
			void *outtokens[] = {&bdivToken[ii]};
			adf_create_task(1, 2, intokens, [=] (token_t *tokens) -> void
			{
				TRACE_EVENT(16004,164)

				/* TASK START */
				/* recreate context */

				/* redeclare input tokens */
				std::remove_reference<decltype(luToken[kk])>::type luToken;
				memcpy(&luToken, tokens->value, sizeof(luToken));
				tokens = tokens->next_token;
				std::remove_reference<decltype(bmodToken[ii][kk])>::type bmodToken;
				memcpy(&bmodToken, tokens->value, sizeof(bmodToken));


				/* redeclare input tokens */
				std::remove_reference<decltype(bdivToken[kk])>::type bdivToken;

				/* TASK BODY */

				STAT_COUNT_EVENT(task_exec, GetThreadID());
				STAT_START_TIMER(task_body, GetThreadID());

#ifndef OPTIMAL
				//~ CHECKSWITCHPOINT
				TRANSACTION_BEGIN(3,RW)
#endif
				printf("bdiv [%d, %d]\n", ii, kk);
				
				if (matrix[ii*num_blocks+kk] != NULL)
					bdiv (matrix[kk*num_blocks+kk], matrix[ii*num_blocks+kk]);
				bdivToken = ii*num_blocks+kk;

#ifndef OPTIMAL
				TRANSACTION_END
				//~ CHECKSWITCHPOINT
#endif

				STAT_STOP_TIMER(task_body, GetThreadID());

				/* TASK END */
				adf_pass_token(outtokens[0], &bdivToken, sizeof(bdivToken));    /* pass tokens */

				/* execute once clause */
				adf_task_stop();

				TRACE_EVENT(16004,0)
			});
		}
	}
}



void Solver::bmodTask()
{
	/* Create bmod tasks */
	for (int ii=1; ii<num_blocks; ii++)
	{
		for (int jj= 1; jj<num_blocks; jj++)
		{
			if (bmod_count[ii*num_blocks + jj] == 0)
				continue;

			void *intokens[] = {&fwdToken[jj], &bdivToken[ii]};
			void *outtokens[] = {&bmodToken[ii][jj]};
			adf_create_task(1, 2, intokens, [=] (token_t *tokens) -> void
			{
				TRACE_EVENT(16005,165)

				/* TASK START */
				/* recreate context */

				/* redeclare input tokens */
				std::remove_reference<decltype(fwdToken[jj])>::type fwdToken;
				memcpy(&fwdToken, tokens->value, sizeof(fwdToken));
				tokens = tokens->next_token;
				std::remove_reference<decltype(bdivToken[ii])>::type bdivToken;
				memcpy(&bdivToken, tokens->value, sizeof(bdivToken));

				/* redeclare input tokens */
				std::remove_reference<decltype(bmodToken[ii][jj])>::type bmodToken;

				/* declare local data */
				int exit = 0;	/* the local exit condition flag for use outside of critical section */

				/* UNTIL clause */
				//~ CHECKSWITCHPOINT
				TRANSACTION_BEGIN(4,RO)
						if (bmod_count[ii*num_blocks + jj] <= 0)
							exit = 1;
				TRANSACTION_END
				//~ CHECKSWITCHPOINT

				if (exit) {
					adf_task_stop();
					TRACE_EVENT(16005,0)
					return;
				}

				/* TASK BODY */

				STAT_COUNT_EVENT(task_exec, GetThreadID());
				STAT_START_TIMER(task_body, GetThreadID());

#ifndef OPTIMAL
				//~ CHECKSWITCHPOINT
				TRANSACTION_BEGIN(5,RW)
#endif
				printf("bmod [%d, %d]\n", ii, jj);
				
					if (ii==2 && jj==2) {
						printf("************************************************\n");
						printf("fwdToken    = %d\tmatrix = %p\n", fwdToken, matrix[fwdToken]);
						printf("bdivToken    = %d\tmatrix = %p\n", bdivToken, matrix[bdivToken]);
					}
				
				if (matrix[bdivToken] != NULL && matrix[fwdToken] != NULL) {
					if (matrix[ii*num_blocks+jj]==NULL) {
						matrix[ii*num_blocks+jj] = allocate_clean_block();
					}
					bmod (matrix[bdivToken], matrix[fwdToken], matrix[ii*num_blocks+jj]);
				}

				bmod_count[ii*num_blocks + jj] --;
				if (bmod_count[ii*num_blocks + jj] <= 0)
					bmodToken = ii*num_blocks+jj;

#ifndef OPTIMAL
				TRANSACTION_END
				//~ CHECKSWITCHPOINT
#endif

				STAT_STOP_TIMER(task_body, GetThreadID());

				/* TASK END */
				if (bmod_count[ii*num_blocks + jj] <= 0) {
					adf_pass_token(outtokens[0], &bmodToken, sizeof(bmodToken));    /* pass tokens */
				}

				TRACE_EVENT(16005,0)
			});
		}
	}
}


void Solver::InitialTask()
{
	// Create bmod[0,0..num_blocks] tokens
	// ======================================
	for (int ii = 0; ii < num_blocks; ii++)
	{
		for (int jj = 0; jj < num_blocks; jj++)
		{
			if (matrix[ii*num_blocks + jj] == NULL)
				continue;

			if (bmod_count[ii*num_blocks + jj] > 0)
				continue;

			void *outtokens[] = {&bmodToken[ii][jj]};
			adf_create_task(1, 0, NULL, [=](token_t *tokens) -> void
			{
				TRACE_EVENT(16001,161)
				
				/* TASK START */
				/* recreate context */

				/* redeclare output token data as local variables */
				std::remove_reference<decltype(bmodToken[0][jj])>::type bmodToken;
				
				/* TASK BODY */

				STAT_COUNT_EVENT(task_exec, GetThreadID());
				STAT_START_TIMER(task_body, GetThreadID());
				
				//~ CHECKSWITCHPOINT
				TRANSACTION_BEGIN(8,RW)
				bmodToken = ii*num_blocks + jj;
				TRANSACTION_END
				//~ CHECKSWITCHPOINT
				
				STAT_STOP_TIMER(task_body, GetThreadID());

				/* TASK END */
				adf_pass_token(outtokens[0], &bmodToken, sizeof(bmodToken));    /* pass tokens */
				
				
				/* execute once clause */
				adf_task_stop();

				TRACE_EVENT(16001,0)
			});
		}
	}

}


void Solver::Solve()
{
	luTask();
	fwdTask();
	bdivTask();
	bmodTask();
	printf("InitialTask\n");
	InitialTask();
	printf("End InitialTask\n");	
}


// Problem results output
void Solver::Finish()
{
	dwarfConfigurator -> Close();
}



/*
====================================================================================
	ParseCommandLine
====================================================================================
 */

void ParseCommandLine(int argc, char **argv)
{
	char c;

	input_file_name = (char*) "no file - input generated";
	num_threads = 1;

	while ((c = getopt (argc, argv, "hf:b:s:n:")) != -1)
		switch (c) {
		case 'h':
			printf("\nSparse Algebra - ADF benchmark application\n"
					"\n"
					"Usage:\n"
					"   sparse_algebra_adf [options ...]\n"
					"\n"
					"Options:\n"
					"   -h\n"
					"        Print this help message.\n"
					"   -f <filename>\n"
					"        Input file name.\n"
					"   -b <num_blocks>\n"
					"        Number of blocks in the row.\n"
					"   -s <block_size>\n"
					"        Size of the block.\n"
					"   -n <long>\n"
					"        Number of worker threads. (default 1)\n"
					"\n"
			);
			exit (0);
		case 'f':
			input_file_name = optarg;
			break;
		case 'b':
			num_blocks = atoi(optarg);
			break;
		case 's':
			block_size = atoi(optarg);
			break;
		case 'n':
			num_threads = strtol(optarg, NULL, 10);
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

	if(num_blocks < 4)
		num_blocks = 3;//4;

	if(block_size < 16)
		block_size = 2;//16;

	printf ("\nStarting ADF Sparse Algebra.\n");
	printf ("Input file %s\n", input_file_name);
	printf ("Matrix with %d x %d blocks of size %d\n", num_blocks, num_blocks, block_size);
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
	printf("Solve\n");
	solver.Solve();										// Solve the current problem
	printf("End Solve\n");
	
	printf("ADF_START\n");
	/* start a dataflow execution */
	adf_start();
	printf("End ADF_START\n");
	
	/* wait for the end of execution */
	adf_taskwait();

	adf_terminate();

	solver.Finish();									// write results

	return 0;
}



