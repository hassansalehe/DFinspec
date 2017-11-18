#include <stdlib.h>
#include <stdio.h>
#include <math.h>


void generate_probabilities(double *arr, int size) {
	double sum = 0.0;
	for (int i=0; i<size; i++) {
		arr[i] = rand();
		sum += arr[i];
	}
	for (int i=0; i<size; i++) {
		arr[i] = arr[i] / sum;
	}
}

int main() {
	int num_states = 2400;
	int num_observations = 1000;
	int seq_lenght = 200;
	long seq;

	double transition[num_states];
	double emission[num_observations];
	double initial[num_states];

	FILE *file = fopen("input.txt", "wb");

	fprintf(file, "M:%d\n", num_observations);
	fprintf(file, "N:%d\n", num_states);
	fprintf(file, "A:\n");
	for (int i = 0; i < num_states; i++) {
		generate_probabilities(transition, num_states);
		for (int j=0; j<num_states; j++)
			fprintf(file, "%.4lf ", transition[j]);
		fprintf(file, "\n");
	}

	fprintf(file, "B:\n");
	for (int i = 0; i < num_states; i++) {
		generate_probabilities(emission, num_observations);
		for (int j=0; j<num_observations; j++)
			fprintf(file, "%.4lf ", emission[j]);
		fprintf(file, "\n");
	}

	fprintf(file, "pi:\n");
	generate_probabilities(initial, num_states);
	for (int j=0; j<num_states; j++)
		fprintf(file, "%.4lf ", initial[j]);
	fprintf(file, "\n");

	fprintf(file, "T:%d\n", seq_lenght);

	fprintf(file, "O:\n");
	for (int j=0; j<seq_lenght; j++) {
		seq = lround(rand() % seq_lenght);
		fprintf(file, "%ld ", seq);
	}
	fprintf(file, "\n");
}

