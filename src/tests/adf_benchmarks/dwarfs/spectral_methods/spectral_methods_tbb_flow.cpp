#include <cstdlib>
#include <cstdio>
#include <getopt.h>
#include <cstring>
#include <iostream>
#include <sfftw.h>
#include "tbb/task_scheduler_init.h"
#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"

#include "adf.h"


using namespace std;
using namespace tbb;

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
};



Solver :: Solver(Configurator* configurator)
{
	data = NULL;

	dwarfConfigurator = configurator;					// process settings from the command line
	dwarfConfigurator -> WriteSettings();
	dwarfConfigurator -> GetContent(&data, &xPlan);
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


// 3D FFT algorithm
void Solver::Solve()
{
	/// FFT - X
	/// **********************************
	parallel_for(	blocked_range<size_t>(0, num_elements * num_elements, num_elements * num_elements / num_threads),
					[&] (const blocked_range<size_t>& r) {
		for (unsigned int i = r.begin(); i != r.end(); i++) {
			fftw_one(xPlan, data[i], NULL);  // NOTE: xPlan does the FFT in-place
		}
	});


	/// Transpose X -Y
	/// **********************************
	parallel_for(	blocked_range<size_t>(0, num_elements, num_elements / num_threads),
					[&] (const blocked_range<size_t>& r) {
		for (unsigned int z = r.begin(); z != r.end(); z++) {
			for (int y = 0; y < num_elements; y++) {
				for (int x = y + 1; x < num_elements; x++) {
					fftw_complex tmp;
					tmp.re = data[GET_I(y,z)][x].re;
					tmp.im = data[GET_I(y,z)][x].im;
					data[GET_I(y,z)][x].re = data[GET_I(x,z)][y].re;
					data[GET_I(y,z)][x].im = data[GET_I(x,z)][y].im;
					data[GET_I(x,z)][y].re = tmp.re;
					data[GET_I(x,z)][y].im = tmp.im;
				}
			}
		}
	});

	/// FFT - Y
	/// **********************************
	parallel_for(	blocked_range<size_t>(0, num_elements * num_elements, num_elements * num_elements / num_threads),
					[&] (const blocked_range<size_t>& r) {
		for (unsigned int i = r.begin(); i != r.end(); i++) {
			fftw_one(xPlan, data[i], NULL);  // NOTE: xPlan does the FFT in-place
		}
	});


	/// Transpose Z - X
	/// **********************************
	parallel_for(	blocked_range<size_t>(0, num_elements, num_elements / num_threads),
					[&] (const blocked_range<size_t>& r) {
		for (unsigned int y = r.begin(); y != r.end(); y++) {
			for (int z = 0; z < num_elements; z++) {
				for (int x = z + 1; x < num_elements; x++) {
					fftw_complex tmp;
					tmp.re = data[GET_I(y,z)][x].re;
					tmp.im = data[GET_I(y,z)][x].im;
					data[GET_I(y,z)][x].re = data[GET_I(y,x)][z].re;
					data[GET_I(y,z)][x].im = data[GET_I(y,x)][z].im;
					data[GET_I(y,x)][z].re = tmp.re;
					data[GET_I(y,x)][z].im = tmp.im;
				}
			}
		}
	});


	/// FFT - Z
	/// **********************************
	parallel_for(	blocked_range<size_t>(0, num_elements * num_elements, num_elements * num_elements / num_threads),
					[&] (const blocked_range<size_t>& r) {
		for (unsigned int i = r.begin(); i != r.end(); i++) {
			fftw_one(xPlan, data[i], NULL);  // NOTE: xPlan does the FFT in-place
		}
	});

	/* 	At this point Z has the original Y data,
    	X has the original Z data, and Y has the original X data */

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
				"   spectral_methods_tbb [options ...]\n"
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

	printf ("\nStarting TBB Spectral Methods.\n");
	printf ("Matrix dimension is %d.\n", num_elements);
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

	Configurator dwarfConfigurator;
	Solver solver(&dwarfConfigurator);					// create new Solver (current problem with initial data)

	NonADF_init(num_threads);

	task_scheduler_init init(num_threads);

	solver.Solve();										// Solve the current problem

	NonADF_terminate();

	solver.Finish();									// write results

	return 0;
}

