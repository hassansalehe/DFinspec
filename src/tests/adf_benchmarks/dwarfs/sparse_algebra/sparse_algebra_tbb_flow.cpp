#include <cstdlib>
#include <cstdio>
#include <getopt.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <tuple>
#include <iostream>

#include <tbb/flow_graph.h>
#include <tbb/task_scheduler_init.h>

#include "adf.h"

#define OPTIMAL

using namespace tbb;
using namespace tbb::flow;
using namespace std;

#define MIN(a,b) ((a<b) ? a : b)

typedef char mychar;

char *input_file_name;
int   num_threads;

int num_blocks = 12;
int block_size = 64;

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
	sprintf(resultFile, "result_tbb_flow.txt");

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
				if ((M[ii*num_blocks+jj] == NULL))
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
	void Start();
	void Wait();

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

	// graph
	graph g;

	// tasks
	function_node<int> **luTasks;
	function_node<tuple<int,int>> ***fwdTasks;
	function_node<tuple<int,int>> ***bdivTasks;
	function_node<tuple<int,int>> ***bmodTasks;
	function_node<int> ***initialTasks;

	// join nodes
	join_node<tuple<int,int>> ***fwdJoinNodes;
	join_node<tuple<int,int>> ***bdivJoinNodes;
	join_node<tuple<int,int>> ***bmodJoinNodes;


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

	task_scheduler_init init(num_threads);

	// tokens
	luToken = new int[num_blocks];
	fwdToken = new int[num_blocks];
	bdivToken = new int[num_blocks];
	bmodToken = new int*[num_blocks];
	for (int i=0; i<num_blocks; i++)
		bmodToken[i] = new int [num_blocks];

	// tasks
	luTasks = new function_node<int>*[num_blocks];
	bdivTasks = new function_node<tuple<int,int>>**[num_blocks];
	bmodTasks = new function_node<tuple<int,int>>**[num_blocks];
	fwdTasks = new function_node<tuple<int,int>>**[num_blocks];
	initialTasks = new function_node<int>**[num_blocks];
	for (int i = 0; i < num_blocks; i++) {
		bdivTasks[i] = new function_node<tuple<int,int>>*[num_blocks];
		bmodTasks[i] = new function_node<tuple<int,int>>*[num_blocks];
		fwdTasks[i] = new function_node<tuple<int,int>>*[num_blocks];
		initialTasks[i] = new function_node<int>*[num_blocks];
	}

	// join nodes
	fwdJoinNodes = new join_node<tuple<int,int>>**[num_blocks];
	bdivJoinNodes = new join_node<tuple<int,int>>**[num_blocks];
	bmodJoinNodes = new join_node<tuple<int,int>>**[num_blocks];
	for (int i = 0; i < num_blocks; i++) {
		fwdJoinNodes[i] = new join_node<tuple<int,int>>*[num_blocks];
		bdivJoinNodes[i] = new join_node<tuple<int,int>>*[num_blocks];
		bmodJoinNodes[i] = new join_node<tuple<int,int>>*[num_blocks];
	}

	InitSolver();
}

static void delete_array(function_node<int>**a, int size)
{
	for (int i = 0; i < size; i++) {
		if (a[i]) delete a[i];
	}
	delete [] a;
}

template<class T>
static void delete_matrix(T***m, int size)
{
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			if (m[i][j]) delete m[i][j];
		}
		delete [] m[i];
	}
	delete [] m;
}

template static void delete_matrix(function_node<tuple<int,int>>***m, int size);
template static void delete_matrix(function_node<int>***m, int size);
template static void delete_matrix(join_node<tuple<int,int>>***m, int size);

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

	// tasks
	delete_matrix(bdivTasks, num_blocks);
	delete_matrix(bmodTasks, num_blocks);
	delete_matrix(fwdTasks, num_blocks);
	delete_matrix(initialTasks, num_blocks);
	delete_array(luTasks, num_blocks);

	// join nodes
	delete_matrix(fwdJoinNodes, num_blocks);
	delete_matrix(bdivJoinNodes, num_blocks);
	delete_matrix(bmodJoinNodes, num_blocks);
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
			for (int k=0; k<(MIN(ii, jj)); k++)
				if (matrix[ii*num_blocks + k] != NULL && matrix[k*num_blocks + jj] != NULL)
					bmod_count[ii*num_blocks + jj] ++;
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
	for (int kk=0; kk<num_blocks; kk++) {
		luTasks[kk] = new function_node<int> (g, 1,
				[=] (int) -> void
		{
			lu0(matrix[kk*num_blocks+kk]);
			if (kk < num_blocks - 1)	// do not produce luToken for the last block
				for (int i = kk + 1; i < num_blocks; i++) {
					if (matrix[kk*num_blocks + i] != NULL) {
						input_port<0>(*fwdJoinNodes[kk][i]).try_put(0);
					}
					if (matrix[i*num_blocks + kk] != NULL) {
						input_port<0>(*bdivJoinNodes[i][kk]).try_put(0);
					}
				}
		});
	}
}


void Solver::fwdTask()
{
	for (int kk=0; kk<num_blocks; kk++) {
		for (int jj=kk+1; jj<num_blocks; jj++) {

			if (matrix[kk*num_blocks+jj] == NULL)
				continue;

			fwdTasks[kk][jj] = new function_node<tuple<int,int>> (g, 1,
					[=] (tuple<int,int>) -> void
			{
				if (matrix[kk*num_blocks+jj] != NULL)
					fwd(matrix[kk*num_blocks+kk], matrix[kk*num_blocks+jj]);

				for (int i = kk+1; i < num_blocks ; i++) {
					if (bmod_count[i*num_blocks + jj] != 0) {
						input_port<0>(*bmodJoinNodes[i][jj]).try_put(kk*num_blocks+jj);
					}
				}
			});

			fwdJoinNodes[kk][jj] = new join_node<tuple<int,int>>(g);
			make_edge(*fwdJoinNodes[kk][jj], *fwdTasks[kk][jj]);
		}
	}
}




void Solver::bdivTask()
{
	for (int kk=0; kk<num_blocks; kk++) {
		for (int ii=kk+1; ii<num_blocks; ii++) {

			if (matrix[ii*num_blocks+kk] == NULL)
				continue;

			bdivTasks[ii][kk] = new function_node<tuple<int,int>> (g, 1,
					[=] (tuple<int,int>) -> void
			{
				if (matrix[ii*num_blocks+kk] != NULL)
					bdiv (matrix[kk*num_blocks+kk], matrix[ii*num_blocks+kk]);

				for (int i = kk+1; i < num_blocks ; i++) {
					if (bmod_count[ii*num_blocks + i] != 0) {
						input_port<1>(*bmodJoinNodes[ii][i]).try_put(ii*num_blocks+kk);
					}
				}
			});

			bdivJoinNodes[ii][kk] = new join_node<tuple<int,int>>(g);
			make_edge(*bdivJoinNodes[ii][kk], *bdivTasks[ii][kk]);
		}
	}
}



void Solver::bmodTask()
{
	for (int ii=1; ii<num_blocks; ii++) {
		for (int jj= 1; jj<num_blocks; jj++) {

			if (bmod_count[ii*num_blocks + jj] == 0)
				continue;

			bmodTasks[ii][jj] = new function_node<tuple<int,int>> (g, 1,
					[=] (tuple<int,int> inputToken) -> void
			{
				int fwdToken = get<0>(inputToken);
				int bdivToken = get<1>(inputToken);
				if (bmod_count[ii*num_blocks + jj] <= 0) return;

				if (matrix[bdivToken] != NULL && matrix[fwdToken] != NULL) {
					if (matrix[ii*num_blocks+jj]==NULL) {
						matrix[ii*num_blocks+jj] = allocate_clean_block();
					}
					bmod (matrix[bdivToken], matrix[fwdToken], matrix[ii*num_blocks+jj]);
				}

				bmod_count[ii*num_blocks + jj]--;

				if (bmod_count[ii*num_blocks + jj] <= 0) {
					if (ii < jj)
						input_port<1>(*fwdJoinNodes[ii][jj]).try_put(ii*num_blocks+jj);
					else if (ii > jj)
						input_port<1>(*bdivJoinNodes[ii][jj]).try_put(ii*num_blocks+jj);
					else
						luTasks[ii]->try_put(0);
				}
			});

			bmodJoinNodes[ii][jj] = new join_node<tuple<int,int>>(g);
			make_edge(*bmodJoinNodes[ii][jj], *bmodTasks[ii][jj]);
		}
	}
}


void Solver::InitialTask()
{
	for (int ii = 0; ii < num_blocks; ii++) {
		for (int jj = ii+1; jj < num_blocks; jj++) {

			if (matrix[ii*num_blocks + jj] == NULL)
				continue;
			if (bmod_count[ii*num_blocks + jj] > 0)
				continue;

			input_port<1>(*fwdJoinNodes[ii][jj]).try_put(ii*num_blocks+jj);

		}

		for (int jj = ii+1; jj < num_blocks; jj++) {

			if (matrix[jj*num_blocks + ii] == NULL)
				continue;
			if (bmod_count[jj*num_blocks + ii] > 0)
				continue;

			input_port<1>(*bdivJoinNodes[jj][ii]).try_put(jj*num_blocks+ii);
		}

	}

	luTasks[0]->try_put(0);
}


void Solver::Solve()
{
	luTask();
	fwdTask();
	bdivTask();
	bmodTask();

	/* generate initial tokens and start the graph */
	InitialTask();

	/* wait for the graph to execute */
	g.wait_for_all();
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
					"   sparse_algebra_tbb_flow [options ...]\n"
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

	if(num_blocks < 5)
		num_blocks = 5;

	if(block_size < 25)
		block_size = 25;

	printf ("\nStarting TBB_FLOW Sparse Algebra.\n");
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

	task_scheduler_init init(num_threads);

	Configurator dwarfConfigurator(input_file_name);
	Solver solver(&dwarfConfigurator);					// create new Solver (current problem with initial data)

	NonADF_init(num_threads);

	solver.Solve();										// Solve the current problem

	NonADF_terminate();

	solver.Finish();									// write results

	return 0;
}



