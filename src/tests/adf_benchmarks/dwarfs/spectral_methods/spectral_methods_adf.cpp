#include <cstdio>
#include <getopt.h>
#include <cstring>
#include <iostream>
#include <sfftw.h>

#include "adf.h"

using namespace std;

typedef char mychar;

int   num_threads;
int   num_elements = 256;

#define GET_I(y,z)     ( ((z)*(num_elements)) + (y) )

#define OPTIMAL

/*
======================================================================
	Settings
======================================================================
*/

class Settings
{
public:
    char* dwarfName;

	Settings();
	~Settings();
	void PrintStringSettings();

private:
	static const int maxArraySize = 5000;
};


Settings::Settings()
{
	dwarfName = new char[maxArraySize];
	dwarfName[0] = '\0';
	sprintf(dwarfName, "%s", "SpectralMethods");
}

Settings::~Settings()
{
	delete[] dwarfName;
}

void Settings::PrintStringSettings()
{
	char* stringSettings = new char[maxArraySize];
	stringSettings[0] = '\0';

	sprintf(stringSettings, "%s%s", stringSettings, "Kernel settings summary: ");
	sprintf(stringSettings, "%s%s", stringSettings, "\nDwarf name        : ");
	sprintf(stringSettings, "%s%s", stringSettings, dwarfName);

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
	Configurator() { settings = new Settings(); }
	~Configurator() { delete settings; }
	void GetContent(fftw_complex*** data, fftw_plan *xPlan);
	void WriteSettings() { settings -> PrintStringSettings(); }
	void Close(fftw_complex** data) {};

private:
	Settings* settings;
};



void Configurator :: GetContent(fftw_complex*** data, fftw_plan *xPlan)
{
	  // Initialize rand
	  srand(0);

	  // Create the 3D grid of values
	  // Create a 2D array of pointers (data[y][z]) and then allocate the x dimension for each
	  *data = new fftw_complex*[num_elements * num_elements];
	  for (int i = 0; i < (num_elements * num_elements); i++) {
		  (*data)[i] = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * num_elements);
		  for (int j = 0; j < num_elements; j++) {
			  (*data)[i][j].re = ((j % num_elements == 0) ? (1.0f) : (0.0f));  // Initialize data here
			  (*data)[i][j].im = 0.0f;  // Initialize data here
		  }
	  }

	  // Create the plan for doing a 1D FFT along the X-Dimension
	  *xPlan = fftw_create_plan(num_elements, FFTW_FORWARD, FFTW_IN_PLACE | FFTW_ESTIMATE);
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
	void Finish();

	fftw_complex  **data;		// 2D array of pointers (data[y][z])
	fftw_plan       xPlan;		// the plan for doing a 1D FFT along the X-Dimension

private:
	Configurator	 *dwarfConfigurator;

	int num_tasks;

	struct TaskIndexes {
		int startIndex;
		int endIndex;
	};

	// tokens
	int 		fftxToken;
	int			transposeXYToken;
	int			transposeZXToken;
	int			barrierToken;


	// ADF tasks
	void fftxTask();
	void transposeXYTask();
	void transposeZXTask();
	void barrierTask();
	void InitialTask();
};



Solver :: Solver(Configurator* configurator)
{
	data = NULL;

	dwarfConfigurator = configurator;					// process settings from the command line
	dwarfConfigurator -> WriteSettings();
	dwarfConfigurator -> GetContent(&data, &xPlan);

	num_tasks = 8*num_threads;
}


Solver :: ~Solver()
{
	  // destroy the plan
	  fftw_destroy_plan(xPlan);

	  // Free the memory used by the data
	  for (int i = 0; i < (num_elements * num_elements); i++)
	    if (data[i] != NULL) { fftw_free(data[i]); data[i] = NULL; }
	  delete [] data;
}




void Solver::fftxTask()
{
	for (int i = 0; i < num_tasks; i++) {
		void *intokens[] = {&fftxToken};
		void *outtokens[] = {&barrierToken};
		adf_create_task(1, 1, intokens, [=] (token_t *tokens) -> void
		{
			TRACE_EVENT(17002,172)

			/* TASK START */
			/* recreate context */

			/* redeclare input tokens */
			std::remove_reference<decltype(fftxToken)>::type fftxToken;
			memcpy(&fftxToken, tokens->value, sizeof(fftxToken));

			/* redeclare output tokens */
			std::remove_reference<decltype(barrierToken)>::type barrierToken;

			/* TASK BODY */

			STAT_COUNT_EVENT(task_exec, GetThreadID());
			STAT_START_TIMER(task_body, GetThreadID());

#ifndef OPTIMAL
			TRANSACTION_BEGIN(5,RW)
#endif

			int startIndex = (num_elements * num_elements * i    ) / num_tasks;
			int endIndex   = (num_elements * num_elements * (i+1)) / num_tasks - 1;
			for (int i = startIndex; i <= endIndex; i++)
				fftw_one(xPlan, data[i], NULL);  // NOTE: xPlan does the FFT in-place

			barrierToken = i;

#ifndef OPTIMAL
			TRANSACTION_END
#endif

			STAT_STOP_TIMER(task_body, GetThreadID());

			/* TASK END */
			adf_pass_token(outtokens[0], &barrierToken, sizeof(barrierToken));    /* pass tokens */

			TRACE_EVENT(17002,0)
		});
	}
}

void Solver::transposeXYTask()
{
	for (int i = 0; i < num_tasks; i++) {
		void *intokens[] = {&transposeXYToken};
		void *outtokens[] = {&barrierToken};
		adf_create_task(1, 1, intokens, [=] (token_t *tokens) -> void
		{
			TRACE_EVENT(17003,173)

			/* TASK START */
			/* recreate context */

			/* redeclare input tokens */
			std::remove_reference<decltype(transposeXYToken)>::type transposeXYToken;
			memcpy(&transposeXYToken, tokens->value, sizeof(transposeXYToken));

			/* redeclare output tokens */
			std::remove_reference<decltype(barrierToken)>::type barrierToken;

			/* TASK BODY */

			STAT_COUNT_EVENT(task_exec, GetThreadID());
			STAT_START_TIMER(task_body, GetThreadID());

#ifndef OPTIMAL
			TRANSACTION_BEGIN(5,RW)
#endif

			for (int z = i; z < num_elements; z+=num_tasks)
				for (int y = 0; y < num_elements; y++)
					for (int x = y + 1; x < num_elements; x++) {
						fftw_complex tmp;
						tmp.re = data[GET_I(y,z)][x].re;
						tmp.im = data[GET_I(y,z)][x].im;
						data[GET_I(y,z)][x].re = data[GET_I(x,z)][y].re;
						data[GET_I(y,z)][x].im = data[GET_I(x,z)][y].im;
						data[GET_I(x,z)][y].re = tmp.re;
						data[GET_I(x,z)][y].im = tmp.im;
					}

			barrierToken = i;

#ifndef OPTIMAL
			TRANSACTION_END
#endif

			STAT_STOP_TIMER(task_body, GetThreadID());

			/* TASK END */
			adf_pass_token(outtokens[0], &barrierToken, sizeof(barrierToken));    /* pass tokens */

			TRACE_EVENT(17003,0)
		});
	}
}


void Solver::transposeZXTask()
{
	for (int i = 0; i < num_tasks; i++) {
		void *intokens[] = {&transposeZXToken};
		void *outtokens[] = {&barrierToken};
		adf_create_task(1, 1, intokens, [=] (token_t *tokens) -> void
		{
			TRACE_EVENT(17004,174)

			/* TASK START */
			/* recreate context */

			/* redeclare input tokens */
			std::remove_reference<decltype(transposeZXToken)>::type transposeZXToken;
			memcpy(&transposeZXToken, tokens->value, sizeof(transposeZXToken));

			/* redeclare output tokens */
			std::remove_reference<decltype(barrierToken)>::type barrierToken;

			/* TASK BODY */

			STAT_COUNT_EVENT(task_exec, GetThreadID());
			STAT_START_TIMER(task_body, GetThreadID());

#ifndef OPTIMAL
			TRANSACTION_BEGIN(5,RW)
#endif

			for (int y = i; y < num_elements; y+=num_tasks)
				for (int z = 0; z < num_elements; z++)
					for (int x = z + 1; x < num_elements; x++) {
						fftw_complex tmp;
						tmp.re = data[GET_I(y,z)][x].re;
						tmp.im = data[GET_I(y,z)][x].im;
						data[GET_I(y,z)][x].re = data[GET_I(y,x)][z].re;
						data[GET_I(y,z)][x].im = data[GET_I(y,x)][z].im;
						data[GET_I(y,x)][z].re = tmp.re;
						data[GET_I(y,x)][z].im = tmp.im;
					}

			barrierToken = i;

#ifndef OPTIMAL
			TRANSACTION_END
#endif

			STAT_STOP_TIMER(task_body, GetThreadID());

			/* TASK END */
			adf_pass_token(outtokens[0], &barrierToken, sizeof(barrierToken));    /* pass tokens */

			TRACE_EVENT(17004,0)
		});
	}
}


void Solver::barrierTask()
{
	static int barrier_entry = 0;
	static int task_count = 0;

	void *intokens[] = {&barrierToken};
	void *outtokens[] = {&transposeXYToken, &transposeZXToken, &fftxToken};
	adf_create_task(1, 1, intokens, [=] (token_t *tokens) -> void
	{
		TRACE_EVENT(17005,175)

		/* TASK START */
		/* recreate context */

		/* redeclare input tokens */
		std::remove_reference<decltype(barrierToken)>::type barrierToken;
		memcpy(&barrierToken, tokens->value, sizeof(barrierToken));

		/* redeclare output tokens */
		std::remove_reference<decltype(fftxToken)>::type fftxToken;
		std::remove_reference<decltype(transposeXYToken)>::type transposeXYToken;
		std::remove_reference<decltype(transposeZXToken)>::type transposeZXToken;

		/* declare local data */
		bool generate_output = false;

		/* TASK BODY */

		STAT_COUNT_EVENT(task_exec, GetThreadID());
		STAT_START_TIMER(task_body, GetThreadID());

#ifndef OPTIMAL
		TRANSACTION_BEGIN(5,RW)
#endif

		task_count++;
		if(task_count >= num_tasks) {
			task_count = 0;
			generate_output = true;

			barrier_entry++;
			if (barrier_entry == 1)
				transposeXYToken = 1;
			else if (barrier_entry == 3)
				transposeZXToken = 1;
			else if ((barrier_entry == 2) || (barrier_entry == 4))
				fftxToken = 1;
		}

#ifndef OPTIMAL
		TRANSACTION_END
#endif

		STAT_STOP_TIMER(task_body, GetThreadID());

		/* TASK END */

		if (generate_output) {
			if (barrier_entry == 1)
				adf_pass_token(outtokens[0], &transposeXYToken, sizeof(transposeXYToken));    /* pass tokens */
			else if (barrier_entry == 3)
				adf_pass_token(outtokens[1], &transposeZXToken, sizeof(transposeZXToken));    /* pass tokens */
			else if ((barrier_entry == 2) || (barrier_entry == 4))
				adf_pass_token(outtokens[2], &fftxToken, sizeof(fftxToken));    /* pass tokens */
		}

		TRACE_EVENT(17005,0)
	});

}



void Solver::InitialTask()
{
	void *outtokens[] = {&fftxToken};
	adf_create_task(1, 0, NULL, [=](token_t *tokens) -> void
	{
		TRACE_EVENT(17001,171)

		/* TASK START */
		/* recreate context */

		/* redeclare output token data as local variables */
		std::remove_reference<decltype(fftxToken)>::type fftxToken;

		/* TASK BODY */
		TRANSACTION_BEGIN(2,RW)
			fftxToken = 1;
		TRANSACTION_END

		/* TASK END */
		adf_pass_token(outtokens[0], &fftxToken, sizeof(fftxToken));    /* pass tokens */

		TRACE_EVENT(17001,0)

		/* execute_once clause */
		adf_task_stop();
	});
}


// 3D FFT algorithm
void Solver::Solve()
{
	fftxTask();
	transposeXYTask();
	transposeZXTask();
	barrierTask();
	InitialTask();
}


void Solver::Finish()
{
	dwarfConfigurator -> Close(data);
}


/*
====================================================================================
	ParseCommandLine
====================================================================================
*/

void ParseCommandLine(int argc, char **argv)
{
	char c;

	num_threads = 1;

	while ((c = getopt (argc, argv, "he:n:")) != -1)
		switch (c) {
			case 'h':
				printf("\nSpectral Methods - ADF benchmark application\n"
				"\n"
				"Usage:\n"
				"   spectral_methods_adf [options ...]\n"
				"\n"
				"Options:\n"
				"   -h\n"
				"        Print this help message.\n"
				"   -e <int>\n"
				"        Number of matrix elements (default 256)\n"
				"   -n <long>\n"
				"        Number of worker threads. (default 1)\n"
				"\n"
				);
				exit (0);
			case 'e':
				num_elements = atoi(optarg);
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

	if (num_elements < 100)
		num_elements = 100;

	printf ("\nStarting ADF Spectral Methods.\n");
	printf ("Matrix dimension is %d.\n", num_elements);
	printf ("Running with %d threads.\n", num_threads);;
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

	Configurator dwarfConfigurator;
	Solver solver(&dwarfConfigurator);					// create new Solver (current problem with initial data)

	adf_init(num_threads);

	solver.Solve();										// Solve the current problem

	/* start a dataflow execution */
	adf_start();

	/* wait for the end of execution */
	adf_taskwait();

	adf_terminate();

	solver.Finish();									// write results

	return 0;
}


