#include <cstdlib>
#include <cstdio>
#include <getopt.h>
#include <cstring>
#include <iostream>

#include <tbb/flow_graph.h>
#include <tbb/task_scheduler_init.h>

#include "adf.h"

using namespace tbb;
using namespace tbb::flow;
using namespace std;

#define OPTIMAL

// Default size for buffer.
#define BUFFER_SIZE 1024

#define MAX(a,b) (a>b) ? a : b;

typedef char mychar;

bool  print_results;
char *input_file_name;
int   num_threads;



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

	sprintf(dwarfName, "%s", "GraphModels");
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

	printf("%s", stringSettings);

	delete[] stringSettings;
}



/*
======================================================================
	Hidden Markov model
======================================================================
*/

class HMM
{
private:
    int numberOfStates;                         // number of states;  Q = {1,2,...,N}
    int numberOfObservationSymbols;             // number of observation symbols; V={1,2,...,M}
public:
    double **stateTransitionMatrix;             // A[1..N][1..N]. a[i][j] is the transition prob
                                                // of going from state i at time t to state j
                                                // at time t+1

    double  **observationProbabilityMatrix;     // B[1..N][1..M] is the  probability
                                                // of observing symbol j in state i

    double *initialStates;                      // initialStates[1..numberOfStates]  is the initial state distribution.

    HMM(int n, int m);
    ~HMM();
    int getNumberOfStates() { return numberOfStates; }
    int getNumberOfObservation() { return numberOfObservationSymbols; }
};


HMM::HMM(int n, int m)
{
    numberOfStates = n;
    numberOfObservationSymbols = m;

    //creation of  state transition and observation matrices
    stateTransitionMatrix = new double*[numberOfStates];
    observationProbabilityMatrix = new double*[numberOfStates];
    for(int i = 0; i < numberOfStates; i++)
    {
        stateTransitionMatrix[i] = new double[numberOfStates];
        observationProbabilityMatrix[i] = new double[numberOfObservationSymbols];
    }

    //initialization
    initialStates = new double[numberOfStates];
    for(int i=0; i < numberOfStates; i++ ) {
        for(int j=0; j < numberOfStates; j++) {
            stateTransitionMatrix[i][j] = 0;
        }
        for(int j=0; j < numberOfObservationSymbols; j++) {
            observationProbabilityMatrix[i][j] = 0;
        }
        initialStates[i] = 0;
    }
};

HMM::~HMM()
{
    for(int i = 0;i < numberOfStates; i++) {
        delete[]stateTransitionMatrix[i];
        delete[]observationProbabilityMatrix[i];
    }
    delete []stateTransitionMatrix;
    delete []observationProbabilityMatrix;
    delete []initialStates;
};


/*
======================================================================
	Viterbi model
======================================================================
*/

class ViterbiModel
{
private:
    double probability;                         // the maximization of probability
public:
    double **delta;                             // is the best score(highest probability) along a single path,at time t
    int **psi;                                  // track of the argument which maximized
    int *observationSequence;                   // the observation sequence [ O ]
    int *sequenceOfState;                       // the sequence of state [ Q ]
    int lengthOfObservationSequence;            // length of the sequence

    ViterbiModel(int t,HMM *hmm);
    ~ViterbiModel();
    int getLengthOfObservationSequence() { return lengthOfObservationSequence; }
    double getProbability() { return probability; }
    void setProbability(double p) { probability = p; }
};


ViterbiModel::ViterbiModel(int t,HMM *hmm)
{
    lengthOfObservationSequence = t;
    //creation of delta and psi work elements
    delta = new double*[lengthOfObservationSequence];
    psi = new int*[lengthOfObservationSequence];
    int numberOfState = hmm->getNumberOfStates();
    for(int i = 0; i < lengthOfObservationSequence; i++) {
        delta[i] = new double[numberOfState];
        psi[i] = new int[numberOfState];
    }

    observationSequence = new int[lengthOfObservationSequence];
    sequenceOfState = new int[lengthOfObservationSequence];

    //initialization
    for(int i = 0; i < lengthOfObservationSequence; i ++) {
        for(int j = 0; j < numberOfState; j++) {
            delta[i][j] = 0;
            psi[i][j] = 0;
        }
        observationSequence[i] = 0;
        sequenceOfState[i] = 0;
    }
};


ViterbiModel::~ViterbiModel()
{
    for(int i = 0;i < lengthOfObservationSequence; i++) {
        delete[]psi[i];
        delete[]delta[i];
    }
    delete []psi;
    delete []delta;
    delete []observationSequence;
    delete []sequenceOfState;
};



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
	void GetContent(HMM **hmm, ViterbiModel **vit);
	void WriteSettings() { settings -> PrintStringSettings(); }
	void Close(ViterbiModel *vit);

private:
	Settings* settings;
};


void Configurator :: GetContent(HMM **hmm, ViterbiModel **vit)
{
	int m,n,t, ret;
	FILE *file = fopen(settings -> inputFile, "rb");		// Opening the input file

    ret = fscanf(file, "M:%d\n", &m);		// number of observations
    ret = fscanf(file, "N:%d\n", &n);		// number of states

    *hmm = new HMM(n,m);

    // transition_probability
    ret = fscanf(file, "A:\n");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
        	ret = fscanf(file, "%lf", &((*hmm)->stateTransitionMatrix[i][j]));
        }
        ret = fscanf(file,"\n");
    }

    // emission_probability
    ret = fscanf(file, "B:\n");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
        	ret = fscanf(file, "%lf", &((*hmm)->observationProbabilityMatrix[i][j]));
        }
        ret = fscanf(file,"\n");
    }

    // start_probability
    ret = fscanf(file, "pi:\n");
    for (int i = 0; i < n; i++) {
    	ret = fscanf(file, "%lf", &((*hmm)->initialStates[i]));
    }

    // observations
    ret = fscanf(file,"\n");
    ret = fscanf(file, "T:%d\n", &t);
    *vit = new ViterbiModel(t, *hmm);
    ret = fscanf(file, "O:\n");
    for(int i = 0; i < t; i++) {
    	ret = fscanf(file, "%d", &((*vit)->observationSequence[i]));
    }

	fclose(file);
    printf("number of states = %d\tnumber of observations = %d\n", n, m);
}

void Configurator :: Close(ViterbiModel *vit)
{
	if (print_results == true) {
		FILE *result = fopen(settings -> resultFile, "wb");

		fprintf(result,  "#Result domain:\r\n");
		int length = vit->getLengthOfObservationSequence();
		for(int i = 0; i < length; i++) {
			fprintf(result,"%d ", vit->sequenceOfState[i]);
		}
		fprintf(result, "\r\n");
		fprintf(result,"Probability: %g \r\n", vit->getProbability() );
		fprintf(result, "#eof");

		fclose(result);
	}
}


/*
======================================================================
	Solver
======================================================================
*/


class Solver
{
public:
	HMM          *hmm;          //Presentation of Hidden Markov model
	ViterbiModel *vit;          //Presentation of Viterbi model

	Solver(Configurator* configurator);
	~Solver();
	void InitSolver();
	void Solve();
	void PrintSummery() {};
	void Finish() { dwarfConfigurator->Close(vit); }

private:
	Configurator	 *dwarfConfigurator;
	int 			  numberOfStates;
	int 			  sequence_step;
	int 			  num_compute_tasks;

	// tokens
	int         	computeToken;
	int 			backtrackToken;

	// tasks
	vector<function_node<int>*> *computeTasks;
	function_node<int> *backTrackTask;
	function_node<int> *initialTask;

	// graph
	graph g;

	void ComputeTask();
	void BackTrackTask();
};


Solver :: Solver(Configurator* configurator)
{
	hmm = NULL;
	vit = NULL;
	dwarfConfigurator = configurator;					// process settings from the command line
	dwarfConfigurator -> WriteSettings();
	dwarfConfigurator -> GetContent(&hmm, &vit);
}


Solver :: ~Solver()
{
	delete hmm;
	delete vit;
	if (computeTasks) {
		for (unsigned int i = 0; i < computeTasks->size(); i++) {
			if ((*computeTasks)[i]) delete (*computeTasks)[i];
		}
		delete computeTasks;
	}
	if (backTrackTask) delete backTrackTask;
	if (initialTask) delete initialTask;
}


void Solver::InitSolver()
{
	/* 1. Initialization  delta and psi */
	numberOfStates = hmm->getNumberOfStates();
	for (int i = 0; i < numberOfStates; i++) {
		vit->delta[0][i] = hmm->initialStates[i] * (hmm->observationProbabilityMatrix[i][vit->observationSequence[0]]);
		vit->psi[0][i] = 0;
	}
	sequence_step = 1;
	num_compute_tasks = 0;
	computeTasks = new vector<function_node<int>*>();
}


void Solver::ComputeTask()
{
	static int states_processed = 0;

	int chunk = numberOfStates/ num_threads;
	if (chunk<1) chunk = 1;

	for (int j = 0; j < numberOfStates; j+=chunk) {

		num_compute_tasks++;

		auto ct = new function_node<int> (g, 1, [=] (int computeToken) -> void
		{
			int backtrackToken = 0;
			bool generate_tokens = false;
			int limit = ((j+chunk) < numberOfStates) ? (j+chunk) : numberOfStates;

			for (int k = j; k < limit; k++) {

				int maxvalind = 0;                        	//temporary storage for max value for psi array
				double maxval = 0.0, val = 0.0;             //temporary storage for max value for delta array
				int t = sequence_step;

				//finding the maximum
				for (int i = 0; i < numberOfStates; i++) {

					val = vit->delta[t-1][i]*(hmm->stateTransitionMatrix[i][k]);

					if (val > maxval) {
						maxval = val;
						maxvalind = i;
					}
				}

				vit->delta[t][k] = maxval*(hmm->observationProbabilityMatrix[k][vit->observationSequence[t]]);
				vit->psi[t][k] = maxvalind;
			}

#ifdef OPTIMAL
			TRANSACTION_BEGIN(5,RW)
#endif
			states_processed += chunk;
			if (states_processed >= numberOfStates) {
				states_processed = 0;
				generate_tokens = true;
				sequence_step++;
				if (sequence_step >= vit->lengthOfObservationSequence)
					backtrackToken = 1;
				else
					computeToken = 1;
			}
#ifdef OPTIMAL
			TRANSACTION_END
#endif

			/* TASK END */
			if  (generate_tokens) {
				if (sequence_step < vit->lengthOfObservationSequence)
					for (unsigned int i = 0; i < computeTasks->size(); i++) {
						(*computeTasks)[i]->try_put(computeToken);
					}
				else
					backTrackTask->try_put(backtrackToken);
			}
		});

		computeTasks->push_back(ct);
	}
}


void Solver::BackTrackTask()
{
	backTrackTask = new function_node<int> (g, 1, [=] (int) -> void
	{
		int lengthOfObsSeq = vit->lengthOfObservationSequence;
		vit->setProbability(0.0);

		vit->sequenceOfState[lengthOfObsSeq - 1] = 0;
		for (int i = 0; i < numberOfStates; i++) {
			if (vit->delta[lengthOfObsSeq - 1][i] > vit->getProbability()) {
				vit->setProbability(vit->delta[lengthOfObsSeq - 1][i]);
				vit->sequenceOfState[lengthOfObsSeq - 1] = i;
			}
		}

		for (int t = lengthOfObsSeq - 2; t >= 0; t--)
			vit->sequenceOfState[t] = vit->psi[t+1][vit->sequenceOfState[t+1]];
	});
}


// Viterbi algorithm for Hidden Markov Model
void Solver::Solve()
{
	InitSolver();

    printf("\nlengthOfObsSeq = %d\n", vit->lengthOfObservationSequence);
    printf("numberOfStates = %d\n",numberOfStates);

	ComputeTask();
	BackTrackTask();

	/* generate initial tokens */
	for (unsigned int i = 0; i < computeTasks->size(); i++)
		(*computeTasks)[i]->try_put(1);

	/* wiat for the graph to finish */
	g.wait_for_all();

}



/*
====================================================================================
	ParseCommandLine
====================================================================================
*/

void ParseCommandLine(int argc, char **argv)
{
	char c;

	input_file_name = (char*) "smallcontent.txt";
	num_threads = 1;
	print_results = false;

	while ((c = getopt (argc, argv, "hf:n:r")) != -1)
		switch (c) {
			case 'h':
				printf("\nGraph Models - ADF benchmark application\n"
				"\n"
				"Usage:\n"
				"   graph_models_tbb_flow [options ...]\n"
				"\n"
				"Options:\n"
				"   -h\n"
				"        Print this help message.\n"
				"   -f <filename>\n"
				"        Input file name.\n"
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

	printf ("\nStarting TBB_FLOW Graph Models.\n");
	printf ("Running with %d threads. Input file %s\n", num_threads, input_file_name);
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

	NonADF_init(num_threads);

	task_scheduler_init init(num_threads);

	solver.Solve();										// Solve the current problem

	NonADF_terminate();

	solver.Finish();									// write results

	return 0;
}



