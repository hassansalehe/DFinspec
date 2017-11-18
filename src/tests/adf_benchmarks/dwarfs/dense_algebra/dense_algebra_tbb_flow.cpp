#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <getopt.h>
#include <tuple>
#include <iostream>

#include <tbb/flow_graph.h>
#include <tbb/task_scheduler_init.h>

#include "adf.h"

#define OPTIMAL

using namespace tbb;
using namespace tbb::flow;
using namespace std;

#define MIN(a,b) (((a)<=(b)) ? a : b)
#define MAX(a,b) (((a)>=(b)) ? a : b)

typedef float real;
typedef float doublereal;
typedef long integer;
typedef bool logical;


float  ***A;
float    *Alin;
long      MatrixSize;

/* command line parameters */
long      BlockSize;
long      NumBlocks;
int       num_threads = 1;



// Forward declarations
long dlarnv_ (long *idist, long *iseed, long *n, float *x);
int  sgemm_  (char *transa, char *transb, integer *m, integer *n, integer *k, real *alpha,
		      real *a, integer *lda, real *b, integer *ldb, real *beta, real *c__, integer *ldc);
int  strsm_  (char *side, char *uplo, char *transa, char *diag, integer *m, integer *n,
		      real *alpha, real *a, integer *lda, real *b, integer *ldb);
int  ssyrk_  (char *uplo, char *trans, integer *n, integer *k, real *alpha, real *a,
		      integer *lda, real *beta, real *c__, integer *ldc);
int  spotf2_ (char *uplo, integer *n, real *a, integer *lda, integer *info);


/*
====================================================================================
	LAPACK wrappers
====================================================================================
*/

void spotrfBlock(float *A,long BSize)
{
	long INFO;

	spotf2_((char*) "L",              /* UPLO     */
			&BSize,                   /* n        */
			A, &BSize,                /* A, LDA   */
			&INFO);                   /* INFO     */

}

void sgemmBlock(float  *A, float *B, float *C, long BSize)
{
	float ALPHA=-1.0, BETA=1.0;

	sgemm_( (char*) "Transpose",      /* transA   */
			(char*) "NoTranspose",    /* transB   */
			&BSize, &BSize, &BSize,   /* m, n, k  */
			&ALPHA,                   /* ALPHA    */
			A, &BSize,                /* A, LDA   */
			B, &BSize,                /* B, LDB   */
			&BETA,                    /* BETA     */
			C, &BSize);               /* C, LDC   */
}

void strsmBlock(float *T, float *B, long BSize)
{
	float ALPHA=1.0;

	strsm_ ((char*) "Right",          /* SIDE     */
			(char*) "L",              /* UPLO     */
			(char*) "Transpose",      /* transA   */
			(char*) "N",              /* DIAG     */
			&BSize, &BSize,           /* M, N     */
			&ALPHA,                   /* ALPHA    */
			T, &BSize,                /* A, LDA   */
			B, &BSize);               /* B, LDB   */
}

void ssyrkBlock( float *A, float *C, long BSize)
{
	float ALPHA=-1.0, BETA=1.0;

	ssyrk_ ((char*) "L",              /* UPLO     */
			(char*) "Transpose",      /* transA   */
			&BSize, &BSize,           /* M, N     */
			&ALPHA,                   /* ALPHA    */
			A, &BSize,                /* A, LDA   */
			&BETA,                    /* BETA     */
			C, &BSize);               /* C, LDC   */
}




/*
====================================================================================
	Implementation
====================================================================================
*/


void PrintMatrix(long size, float *A) {
	printf("\n");
	for(long i = 0; i < size; i++) {
		for(long j = 0; j < size; j++) {
			printf("%g ", A[i * size + j]);
		}
		printf("\n");
	}
	printf("\n\n");
}




void InitMatrix()
{
	long ISEED[4] = {0,0,0,1};
	long IONE=1;

	// matrix init
	MatrixSize = BlockSize * NumBlocks;
	long NN = MatrixSize * MatrixSize;

	// linear matrix
	Alin = (float *) malloc(NN * sizeof(float));

	// fill the matrix with random values
	dlarnv_(&IONE, ISEED, &NN, Alin);

	// make it positive definite
	for(long i = 0; i < MatrixSize; i++) {
		for(long j = i+1; j < MatrixSize; j++) {
			Alin[i * MatrixSize + j] = Alin[j * MatrixSize + i];
		}
	}

	for(long i = 0; i < MatrixSize; i++) {
		Alin[i * MatrixSize + i] += MatrixSize;
	}

	// blocked matrix
	A = (float***) malloc(NumBlocks * sizeof(float **));
	for (long i = 0; i < NumBlocks; i++)
		A[i] = (float**) malloc(NumBlocks * sizeof(float *));

	for (long i = 0; i < NumBlocks; i++) {
		for (long j = 0; j < NumBlocks; j++) {
			A[i][j] = (float *) malloc(BlockSize * BlockSize * sizeof(float));
		}
	}

	for (long i = 0; i < MatrixSize; i++) {
		for (long j = 0; j < MatrixSize; j++) {
			A[i/BlockSize][j/BlockSize][(i%BlockSize) * BlockSize + j%BlockSize] = Alin[i*MatrixSize + j];
		}
	}
}


void FreeMatrix() {
	for (long i = 0; i < NumBlocks; i++) {
		for (long j = 0; j < NumBlocks; j++) {
			free(A[i][j]);
		}
	}

	for (long i = 0; i < NumBlocks; i++)
		free(A[i]);

	free(A);
	free(Alin);

}

/*
====================================================================================
	TBB flow graph nodes
====================================================================================
*/

/* tasks */
function_node<long> **spotrfTasks;
function_node<long> **ssyrkTasks;
function_node<tuple<long,long>> ***strsmTasks;
function_node<tuple<long,long>> ***sgemmTasks;

/* join nodes */
join_node<tuple<long,long>, queueing> ***strsmJoinNodes;
join_node<tuple<long,long>, queueing> ***sgemmJoinNodes;



void DeleteGraph() {
	for (long i = 0; i < NumBlocks; i++) {
		for (long j = 0; j < NumBlocks; j++) {
			delete strsmTasks[i][j];
			delete sgemmTasks[i][j];
			delete strsmJoinNodes[i][j];
			delete sgemmJoinNodes[i][j];
		}
		delete spotrfTasks[i];
		delete ssyrkTasks[i];
		delete [] strsmTasks[i];
		delete [] sgemmTasks[i];
		delete [] strsmJoinNodes[i];
		delete [] sgemmJoinNodes[i];
	}

    delete [] spotrfTasks;
    delete [] ssyrkTasks;
    delete [] strsmTasks;
    delete [] sgemmTasks;
    delete [] strsmJoinNodes;
    delete [] sgemmJoinNodes;
}



void spotrfTask(graph &g) {
	for (long j=0; j<NumBlocks; j++) {
		spotrfTasks[j] = new function_node<long> (g, 1, [=] (long) -> void
		{
			/* Cholesky Factorization of A[j,j]  */
			spotrfBlock( A[j][j], BlockSize);
			if (j < NumBlocks - 1)	// do not produce spotrfToken for the last block
				for (long i = j + 1; i < NumBlocks; i++) {
					input_port<0>(*strsmJoinNodes[i][j]).try_put(j);
				}
		});
	}
}


void ssyrkTask(graph &g) {
	for (long j=0; j<NumBlocks; j++) {
		ssyrkTasks[j] = new function_node<long> (g, 1, [=] (long inputToken) -> void
		{
			long i = inputToken;

			/*  A[j,j] = A[j,j] - A[j,i] * (A[j,i])^t  */
			ssyrkBlock( A[j][i], A[j][j], BlockSize);

			if (i == j-1)
				spotrfTasks[j]->try_put(j);
		});
	}
}


void strsmTask(graph &g) {
	for (long j = 0; j < NumBlocks; j++) {
		for (long i = j+1; i < NumBlocks; i++) {
			strsmTasks[i][j] = new function_node<tuple<long,long>> (g, 1, [=] (tuple<long,long>) -> void
			{
				/*  A[i,j] <- A[i,j] = X * (A[j,j])^t  */
				strsmBlock( A[j][j], A[i][j], BlockSize);

				ssyrkTasks[i]->try_put(j);
				for (long jj = j+1; jj < i; jj++) {
					input_port<0>(*sgemmJoinNodes[i][jj]).try_put(j);
				}
				if (i != NumBlocks - 1) {
					for (long jj = j+1; jj < i+1; jj++) {
						input_port<1>(*sgemmJoinNodes[i+1][jj]).try_put(j);
					}
				}
			});

			strsmJoinNodes[i][j] = new join_node<tuple<long,long>, queueing>(g);
			make_edge(*strsmJoinNodes[i][j], *strsmTasks[i][j]);
		}
	}
}


void sgemmTask(graph &g) {
	for (long j = 1; j < NumBlocks; j++) {
		for (long i = j+1; i < NumBlocks; i++) {
			sgemmTasks[i][j] = new function_node<tuple<long,long>> (g, 1, [=] (tuple<long,long> inputToken) -> void
			{
				long k = get<0>(inputToken);

				/*  A[i,j] = A[i,j] - A[i,k] * (A[j,k])^t  */
				sgemmBlock( A[i][k], A[j][k], A[i][j], BlockSize);

				if (k == j -1)
					input_port<1>(*strsmJoinNodes[i][j]).try_put(j);
			});

			sgemmJoinNodes[i][j] = new join_node<tuple<long,long>, queueing>(g);
			make_edge(*sgemmJoinNodes[i][j], *sgemmTasks[i][j]);
		}
	}
}




void Solve() {

	graph g;

	spotrfTasks = new function_node<long>*[NumBlocks];
	ssyrkTasks = new function_node<long>*[NumBlocks];
	strsmTasks = new function_node<tuple<long,long>>**[NumBlocks];
	sgemmTasks = new function_node<tuple<long,long>>**[NumBlocks];
	strsmJoinNodes = new join_node<tuple<long,long>, queueing>**[NumBlocks];
	sgemmJoinNodes = new join_node<tuple<long,long>, queueing>**[NumBlocks];

	for (long i = 0; i < NumBlocks; i++) {
		strsmTasks[i] = new function_node<tuple<long,long>>*[NumBlocks];
		sgemmTasks[i] = new function_node<tuple<long,long>>*[NumBlocks];
		strsmJoinNodes[i] = new join_node<tuple<long,long>, queueing>*[NumBlocks];
		sgemmJoinNodes[i] = new join_node<tuple<long,long>, queueing>*[NumBlocks];
	}

	spotrfTask(g);
	strsmTask(g);
	sgemmTask(g);
	ssyrkTask(g);



	/* create initial tokens */
	for (long i = 1; i < NumBlocks; i++)
		input_port<1>(*strsmJoinNodes[i][0]).try_put(0);
	spotrfTasks[0]->try_put(0);

	/* wait for the graph to finish the execution */
	g.wait_for_all();

	DeleteGraph();
}



/*
====================================================================================
	ParseCommandLine
====================================================================================
*/
void ParseCommandLine(int argc, char **argv)
{
	char c;

	while ((c = getopt (argc, argv, "hd:b:n:")) != -1)
		switch (c) {
			case 'h':
				printf("\nCholesky Decomposition - ADF benchmark application\n"
				"\n"
				"Usage:\n"
				"   cholseky_tbb_flow [options ...]\n"
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
				"\n"
				);
				exit (0);
			case 'd':
				NumBlocks = atol(optarg);
				break;
			case 'b':
				BlockSize = atol(optarg);
				break;
			case 'n':
				num_threads = atoi(optarg);
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

	if (BlockSize < 10) BlockSize = 10;
	if (NumBlocks < 1) NumBlocks = 1;
	if (num_threads < 1) num_threads = 1;

	printf ("\nCholesky Decomposition\nMatrix size is %ldx%ld, devided into %ldx%ld blocks of size %ld.\n",
			NumBlocks*BlockSize, NumBlocks*BlockSize, NumBlocks, NumBlocks, BlockSize);
	printf ("Running with %d threads.\n", num_threads);
	printf ("=====================================================\n\n");
}


/*
====================================================================================
	MAIN
====================================================================================
*/

int
main(int argc, char *argv[])
{	
	ParseCommandLine(argc, argv);

	tbb::task_scheduler_init init(num_threads);

	InitMatrix();

	NonADF_init(num_threads);

	Solve();

	NonADF_terminate();

	FreeMatrix();

	return 0;
}




















/*
====================================================================================
	LAPACK function for pseudo-random number generation
====================================================================================
*/


int dlaruv_(long *iseed, long *n, float *x)
{
	/* Initialized data */

	static long mm[512]	/* was [128][4] */ = { 494,2637,255,2008,1253,
			3344,4084,1739,3143,3468,688,1657,1238,3166,1292,3422,1270,2016,
			154,2862,697,1706,491,931,1444,444,3577,3944,2184,1661,3482,657,
			3023,3618,1267,1828,164,3798,3087,2400,2870,3876,1905,1593,1797,
			1234,3460,328,2861,1950,617,2070,3331,769,1558,2412,2800,189,287,
			2045,1227,2838,209,2770,3654,3993,192,2253,3491,2889,2857,2094,
			1818,688,1407,634,3231,815,3524,1914,516,164,303,2144,3480,119,
			3357,837,2826,2332,2089,3780,1700,3712,150,2000,3375,1621,3090,
			3765,1149,3146,33,3082,2741,359,3316,1749,185,2784,2202,2199,1364,
			1244,2020,3160,2785,2772,1217,1822,1245,2252,3904,2774,997,2573,
			1148,545,322,789,1440,752,2859,123,1848,643,2405,2638,2344,46,
			3814,913,3649,339,3808,822,2832,3078,3633,2970,637,2249,2081,4019,
			1478,242,481,2075,4058,622,3376,812,234,641,4005,1122,3135,2640,
			2302,40,1832,2247,2034,2637,1287,1691,496,1597,2394,2584,1843,336,
			1472,2407,433,2096,1761,2810,566,442,41,1238,1086,603,840,3168,
			1499,1084,3438,2408,1589,2391,288,26,512,1456,171,1677,2657,2270,
			2587,2961,1970,1817,676,1410,3723,2803,3185,184,663,499,3784,1631,
			1925,3912,1398,1349,1441,2224,2411,1907,3192,2786,382,37,759,2948,
			1862,3802,2423,2051,2295,1332,1832,2405,3638,3661,327,3660,716,
			1842,3987,1368,1848,2366,2508,3754,1766,3572,2893,307,1297,3966,
			758,2598,3406,2922,1038,2934,2091,2451,1580,1958,2055,1507,1078,
			3273,17,854,2916,3971,2889,3831,2621,1541,893,736,3992,787,2125,
			2364,2460,257,1574,3912,1216,3248,3401,2124,2762,149,2245,166,466,
			4018,1399,190,2879,153,2320,18,712,2159,2318,2091,3443,1510,449,
			1956,2201,3137,3399,1321,2271,3667,2703,629,2365,2431,1113,3922,
			2554,184,2099,3228,4012,1921,3452,3901,572,3309,3171,817,3039,
			1696,1256,3715,2077,3019,1497,1101,717,51,981,1978,1813,3881,76,
			3846,3694,1682,124,1660,3997,479,1141,886,3514,1301,3604,1888,
			1836,1990,2058,692,1194,20,3285,2046,2107,3508,3525,3801,2549,
			1145,2253,305,3301,1065,3133,2913,3285,1241,1197,3729,2501,1673,
			541,2753,949,2361,1165,4081,2725,3305,3069,3617,3733,409,2157,
			1361,3973,1865,2525,1409,3445,3577,77,3761,2149,1449,3005,225,85,
			3673,3117,3089,1349,2057,413,65,1845,697,3085,3441,1573,3689,2941,
			929,533,2841,4077,721,2821,2249,2397,2817,245,1913,1997,3121,997,
			1833,2877,1633,981,2009,941,2449,197,2441,285,1473,2741,3129,909,
			2801,421,4073,2813,2337,1429,1177,1901,81,1669,2633,2269,129,1141,
			249,3917,2481,3941,2217,2749,3041,1877,345,2861,1809,3141,2825,
			157,2881,3637,1465,2829,2161,3365,361,2685,3745,2325,3609,3821,
			3537,517,3017,2141,1537 };

	long jota;
	static long ind, i1, i2, i3, i4, it1, it2, it3, it4;

    /* Parameter adjustments */
	--iseed;
	--x;

	/* Function Body */

	i1 = iseed[1];
	i2 = iseed[2];
	i3 = iseed[3];
	i4 = iseed[4];

	jota = MIN(*n,128);
	for (ind = 1; ind <= jota; ++ind) {

L20:

		/* Multiply the seed by i-th power of the multiplier modulo 2**48 */

		it4 = i4 * mm[ind + 383];
		it3 = it4 / 4096;
		it4 -= it3 << 12;
		it3 = it3 + i3 * mm[ind + 383] + i4 * mm[ind + 255];
		it2 = it3 / 4096;
		it3 -= it2 << 12;
		it2 = it2 + i2 * mm[ind + 383] + i3 * mm[ind + 255] + i4 * mm[ind +
		                                                              127];
		it1 = it2 / 4096;
		it2 -= it1 << 12;
		it1 = it1 + i1 * mm[ind + 383] + i2 * mm[ind + 255] + i3 * mm[ind +
		                                                              127] + i4 * mm[ind - 1];
		it1 %= 4096;

		/* Convert 48-bit integer to a real number in the interval (0,1) */

		x[ind] = ((float) it1 + ((float) it2 + ((float) it3 + (
				float) it4 * 2.44140625e-4) * 2.44140625e-4) *
				2.44140625e-4) * 2.44140625e-4;

		if (x[ind] == 1.) {
			/*           If a real number has n bits of precision, and the first
             n bits of the 48-bit integer above happen to be all 1 (which
             will occur about once every 2**n calls), then X( I ) will
             be rounded to exactly 1.0.
             Since X( I ) is not supposed to return exactly 0.0 or 1.0,
             the statistically correct thing to do in this situation is
             simply to iterate again.
             N.B. the case X( I ) = 0.0 should not be possible. */
			i1 += 2;
			i2 += 2;
			i3 += 2;
			i4 += 2;
			goto L20;
		}
	}

	/* Return final value of seed */

	iseed[1] = it1;
	iseed[2] = it2;
	iseed[3] = it3;
	iseed[4] = it4;

	return 0;
} /* dlaruv_ */



long dlarnv_(long *idist, long *iseed, long *n, float *x)
{
	/* Local variables */
	long alpha, beta, gama;
	static long val;
	static float u[128];
	static long il, iv, il2;

	/* Parameter adjustments */
	--x;
	--iseed;

	/* Function Body */
	alpha = *n;
	for (iv = 1; iv <= alpha; iv += 64) {
		/* Computing MIN */
		beta = 64, gama = *n - iv + 1;
		il = MIN(beta,gama);
		if (*idist == 3) {
			il2 = il << 1;
		} else {
			il2 = il;
		}

		/* Call DLARUV to generate IL2 numbers from a uniform (0,1) distribution (IL2 <= LV) */
		dlaruv_(&iseed[1], &il2, u);

		if (*idist == 1) {
			/*           Copy generated numbers */
			beta = il;
			for (val = 1; val <= beta; ++val) {
				x[iv + val - 1] = u[val - 1];
			}
		} else if (*idist == 2) {
			/*           Convert generated numbers to uniform (-1,1) distribution */
			beta = il;
			for (val = 1; val <= beta; ++val) {
				x[iv + val - 1] = u[val - 1] * 2. - 1.;
			}
		} else if (*idist == 3) {
			/*           Convert generated numbers to normal (0,1) distribution */
			beta = il;
			for (val = 1; val <= beta; ++val) {
				x[iv + val - 1] = sqrt(log(u[(val << 1) - 2]) * -2.) *
						cos(u[(val << 1) - 1] * 6.2831853071795864769252867663);
			}
		}
	}

	return 0;
} /* dlarnv_ */






/*
====================================================================================
	LAPACK & BLAS Functions
====================================================================================
*/


logical lsame_(char *a, char*b) {
	return (toupper(*a) == toupper(*b));
}

void xerbla_(char *funcName, integer *val) {
	printf("Error in function %s\tvalue = %d", funcName, (int) *val);
}

/*  Purpose
    =======
    SSYRK  performs one of the symmetric rank k operations
       C := alpha*A*A' + beta*C,
    or
       C := alpha*A'*A + beta*C,
    where  alpha and beta  are scalars, C is an  n by n  symmetric matrix
    and  A  is an  n by k  matrix in the first case and a  k by n  matrix
    in the second case.
 */

int ssyrk_(char *uplo, char *trans, integer *n, integer *k, real *alpha, real *a, integer *lda, real *beta, real *c__, integer *ldc)
{
	/* System generated locals */
	integer a_dim1, a_offset, c_dim1, c_offset, i__1, i__2, i__3;
	/* Local variables */
	static integer i__, j, l, info;
	static real temp;
	static integer nrowa;
	static logical upper;


	a_dim1 = *lda;
	a_offset = 1 + a_dim1;
	a -= a_offset;
	c_dim1 = *ldc;
	c_offset = 1 + c_dim1;
	c__ -= c_offset;

	/* Function Body */
	if (lsame_(trans, (char *) "N")) {
		nrowa = *n;
	} else {
		nrowa = *k;
	}
	upper = lsame_(uplo, (char *) "U");
	info = 0;
	if (! upper && ! lsame_(uplo, (char *) "L")) {
		info = 1;
	} else if (! lsame_(trans, (char *) "N") && ! lsame_(trans, (char *) "T") && ! lsame_(trans, (char *) "C")) {
		info = 2;
	} else if (*n < 0) {
		info = 3;
	} else if (*k < 0) {
		info = 4;
	} else if (*lda < MAX(1,nrowa)) {
		info = 7;
	} else if (*ldc < MAX(1,*n)) {
		info = 10;
	}
	if (info != 0) {
		xerbla_((char *) "SSYRK ", &info);
		return 0;
	}
	/*     Quick return if possible. */
	if (*n == 0 || ((*alpha == 0.f || *k == 0) && *beta == 1.f)) {
		return 0;
	}
	/*     And when  alpha.eq.zero. */
	if (*alpha == 0.f) {
		if (upper) {
			if (*beta == 0.f) {
				i__1 = *n;
				for (j = 1; j <= i__1; ++j) {
					i__2 = j;
					for (i__ = 1; i__ <= i__2; ++i__) {
						c__[i__ + j * c_dim1] = 0.f;
					}
				}
			} else {
				i__1 = *n;
				for (j = 1; j <= i__1; ++j) {
					i__2 = j;
					for (i__ = 1; i__ <= i__2; ++i__) {
						c__[i__ + j * c_dim1] = *beta * c__[i__ + j * c_dim1];
					}
				}
			}
		} else {
			if (*beta == 0.f) {
				i__1 = *n;
				for (j = 1; j <= i__1; ++j) {
					i__2 = *n;
					for (i__ = j; i__ <= i__2; ++i__) {
						c__[i__ + j * c_dim1] = 0.f;
					}
				}
			} else {
				i__1 = *n;
				for (j = 1; j <= i__1; ++j) {
					i__2 = *n;
					for (i__ = j; i__ <= i__2; ++i__) {
						c__[i__ + j * c_dim1] = *beta * c__[i__ + j * c_dim1];
					}
				}
			}
		}
		return 0;
	}
	/*     Start the operations. */
	if (lsame_(trans, (char *) "N")) {
		/*        Form  C := alpha*A*A' + beta*C. */
		if (upper) {
			i__1 = *n;
			for (j = 1; j <= i__1; ++j) {
				if (*beta == 0.f) {
					i__2 = j;
					for (i__ = 1; i__ <= i__2; ++i__) {
						c__[i__ + j * c_dim1] = 0.f;
					}
				} else if (*beta != 1.f) {
					i__2 = j;
					for (i__ = 1; i__ <= i__2; ++i__) {
						c__[i__ + j * c_dim1] = *beta * c__[i__ + j * c_dim1];
					}
				}
				i__2 = *k;
				for (l = 1; l <= i__2; ++l) {
					if (a[j + l * a_dim1] != 0.f) {
						temp = *alpha * a[j + l * a_dim1];
						i__3 = j;
						for (i__ = 1; i__ <= i__3; ++i__) {
							c__[i__ + j * c_dim1] += temp * a[i__ + l * a_dim1];

						}
					}
					/* L120: */
				}
				/* L130: */
			}
		} else {
			i__1 = *n;
			for (j = 1; j <= i__1; ++j) {
				if (*beta == 0.f) {
					i__2 = *n;
					for (i__ = j; i__ <= i__2; ++i__) {
						c__[i__ + j * c_dim1] = 0.f;
						/* L140: */
					}
				} else if (*beta != 1.f) {
					i__2 = *n;
					for (i__ = j; i__ <= i__2; ++i__) {
						c__[i__ + j * c_dim1] = *beta * c__[i__ + j * c_dim1];
						/* L150: */
					}
				}
				i__2 = *k;
				for (l = 1; l <= i__2; ++l) {
					if (a[j + l * a_dim1] != 0.f) {
						temp = *alpha * a[j + l * a_dim1];
						i__3 = *n;
						for (i__ = j; i__ <= i__3; ++i__) {
							c__[i__ + j * c_dim1] += temp * a[i__ + l * a_dim1];
						}
					}
				}
			}
		}
	} else {
		/*        Form  C := alpha*A'*A + beta*C. */
		if (upper) {
			i__1 = *n;
			for (j = 1; j <= i__1; ++j) {
				i__2 = j;
				for (i__ = 1; i__ <= i__2; ++i__) {
					temp = 0.f;
					i__3 = *k;
					for (l = 1; l <= i__3; ++l) {
						temp += a[l + i__ * a_dim1] * a[l + j * a_dim1];
					}
					if (*beta == 0.f) {
						c__[i__ + j * c_dim1] = *alpha * temp;
					} else {
						c__[i__ + j * c_dim1] = *alpha * temp + *beta * c__[i__ + j * c_dim1];
					}
				}
			}
		} else {
			i__1 = *n;
			for (j = 1; j <= i__1; ++j) {
				i__2 = *n;
				for (i__ = j; i__ <= i__2; ++i__) {
					temp = 0.f;
					i__3 = *k;
					for (l = 1; l <= i__3; ++l) {
						temp += a[l + i__ * a_dim1] * a[l + j * a_dim1];
					}
					if (*beta == 0.f) {
						c__[i__ + j * c_dim1] = *alpha * temp;
					} else {
						c__[i__ + j * c_dim1] = *alpha * temp + *beta * c__[i__ + j * c_dim1];
					}
				}
			}
		}
	}
	return 0;
} /* ssyrk_ */







/*  Purpose
	=======
	STRSM  solves one of the matrix equations
	   op( A )*X = alpha*B,   or   X*op( A ) = alpha*B,
	where alpha is a scalar, X and B are m by n matrices, A is a unit, or
	non-unit,  upper or lower triangular matrix  and  op( A )  is one  of
	   op( A ) = A   or   op( A ) = A'.
	The matrix X is overwritten on B.
*/

int strsm_(char *side, char *uplo, char *transa, char *diag, integer *m, integer *n, real *alpha, real *a, integer *lda, real *b, integer *ldb)
{
	/* System generated locals */
	integer a_dim1, a_offset, b_dim1, b_offset, i__1, i__2, i__3;
	/* Local variables */
	static integer i__, j, k, info;
	static real temp;
	static logical lside;
	static integer nrowa;
	static logical upper;
	static logical nounit;

	/* Parameter adjustment */
	a_dim1 = *lda;
	a_offset = 1 + a_dim1;
	a -= a_offset;
	b_dim1 = *ldb;
	b_offset = 1 + b_dim1;
	b -= b_offset;

	/* Function Body */
	lside = lsame_(side, (char *) "L");
	if (lside) {
		nrowa = *m;
	} else {
		nrowa = *n;
	}
	nounit = lsame_(diag, (char *) "N");
	upper = lsame_(uplo, (char *) "U");
	info = 0;
	if (! lside && ! lsame_(side, (char *) "R")) {
		info = 1;
	} else if (! upper && ! lsame_(uplo, (char *) "L")) {
		info = 2;
	} else if (! lsame_(transa, (char *) "N") && ! lsame_(transa, (char *) "T") && ! lsame_(transa, (char *) "C")) {
		info = 3;
	} else if (! lsame_(diag, (char *) "U") && ! lsame_(diag, (char *) "N")) {
		info = 4;
	} else if (*m < 0) {
		info = 5;
	} else if (*n < 0) {
		info = 6;
	} else if (*lda < MAX(1,nrowa)) {
		info = 9;
	} else if (*ldb < MAX(1,*m)) {
		info = 11;
	}
	if (info != 0) {
		xerbla_((char *) "STRSM ", &info);
		return 0;
	}
	/*     Quick return if possible. */
	if (*n == 0) {
		return 0;
	}
	/*     And when  alpha.eq.zero. */
	if (*alpha == 0.f) {
		i__1 = *n;
		for (j = 1; j <= i__1; ++j) {
			i__2 = *m;
			for (i__ = 1; i__ <= i__2; ++i__) {
				b[i__ + j * b_dim1] = 0.f;
			}
		}
		return 0;
	}
	/*     Start the operations. */
	if (lside) {
		if (lsame_(transa, (char *) "N")) {
			/*           Form  B := alpha*inv( A )*B. */
			if (upper) {
				i__1 = *n;
				for (j = 1; j <= i__1; ++j) {
					if (*alpha != 1.f) {
						i__2 = *m;
						for (i__ = 1; i__ <= i__2; ++i__) {
							b[i__ + j * b_dim1] = *alpha * b[i__ + j * b_dim1];
						}
					}
					for (k = *m; k >= 1; --k) {
						if (b[k + j * b_dim1] != 0.f) {
							if (nounit) {
								b[k + j * b_dim1] /= a[k + k * a_dim1];
							}
							i__2 = k - 1;
							for (i__ = 1; i__ <= i__2; ++i__) {
								b[i__ + j * b_dim1] -= b[k + j * b_dim1] * a[i__ + k * a_dim1];
							}
						}
					}
				}
			} else {
				i__1 = *n;
				for (j = 1; j <= i__1; ++j) {
					if (*alpha != 1.f) {
						i__2 = *m;
						for (i__ = 1; i__ <= i__2; ++i__) {
							b[i__ + j * b_dim1] = *alpha * b[i__ + j * b_dim1];
						}
					}
					i__2 = *m;
					for (k = 1; k <= i__2; ++k) {
						if (b[k + j * b_dim1] != 0.f) {
							if (nounit) {
								b[k + j * b_dim1] /= a[k + k * a_dim1];
							}
							i__3 = *m;
							for (i__ = k + 1; i__ <= i__3; ++i__) {
								b[i__ + j * b_dim1] -= b[k + j * b_dim1] * a[i__ + k * a_dim1];
							}
						}
					}
				}
			}
		} else {
			/*           Form  B := alpha*inv( A' )*B. */
			if (upper) {
				i__1 = *n;
				for (j = 1; j <= i__1; ++j) {
					i__2 = *m;
					for (i__ = 1; i__ <= i__2; ++i__) {
						temp = *alpha * b[i__ + j * b_dim1];
						i__3 = i__ - 1;
						for (k = 1; k <= i__3; ++k) {
							temp -= a[k + i__ * a_dim1] * b[k + j * b_dim1];
						}
						if (nounit) {
							temp /= a[i__ + i__ * a_dim1];
						}
						b[i__ + j * b_dim1] = temp;
					}
				}
			} else {
				i__1 = *n;
				for (j = 1; j <= i__1; ++j) {
					for (i__ = *m; i__ >= 1; --i__) {
						temp = *alpha * b[i__ + j * b_dim1];
						i__2 = *m;
						for (k = i__ + 1; k <= i__2; ++k) {
							temp -= a[k + i__ * a_dim1] * b[k + j * b_dim1];
						}
						if (nounit) {
							temp /= a[i__ + i__ * a_dim1];
						}
						b[i__ + j * b_dim1] = temp;
					}
				}
			}
		}
	} else {
		if (lsame_(transa, (char *) "N")) {
			/*           Form  B := alpha*B*inv( A ). */
			if (upper) {
				i__1 = *n;
				for (j = 1; j <= i__1; ++j) {
					if (*alpha != 1.f) {
						i__2 = *m;
						for (i__ = 1; i__ <= i__2; ++i__) {
							b[i__ + j * b_dim1] = *alpha * b[i__ + j * b_dim1];
						}
					}
					i__2 = j - 1;
					for (k = 1; k <= i__2; ++k) {
						if (a[k + j * a_dim1] != 0.f) {
							i__3 = *m;
							for (i__ = 1; i__ <= i__3; ++i__) {
								b[i__ + j * b_dim1] -= a[k + j * a_dim1] * b[i__ + k * b_dim1];
							}
						}
					}
					if (nounit) {
						temp = 1.f / a[j + j * a_dim1];
						i__2 = *m;
						for (i__ = 1; i__ <= i__2; ++i__) {
							b[i__ + j * b_dim1] = temp * b[i__ + j * b_dim1];
						}
					}
				}
			} else {
				for (j = *n; j >= 1; --j) {
					if (*alpha != 1.f) {
						i__1 = *m;
						for (i__ = 1; i__ <= i__1; ++i__) {
							b[i__ + j * b_dim1] = *alpha * b[i__ + j * b_dim1];
						}
					}
					i__1 = *n;
					for (k = j + 1; k <= i__1; ++k) {
						if (a[k + j * a_dim1] != 0.f) {
							i__2 = *m;
							for (i__ = 1; i__ <= i__2; ++i__) {
								b[i__ + j * b_dim1] -= a[k + j * a_dim1] * b[i__ + k * b_dim1];
							}
						}
					}
					if (nounit) {
						temp = 1.f / a[j + j * a_dim1];
						i__1 = *m;
						for (i__ = 1; i__ <= i__1; ++i__) {
							b[i__ + j * b_dim1] = temp * b[i__ + j * b_dim1];
						}
					}
				}
			}
		} else {
			/*           Form  B := alpha*B*inv( A' ). */
			if (upper) {
				for (k = *n; k >= 1; --k) {
					if (nounit) {
						temp = 1.f / a[k + k * a_dim1];
						i__1 = *m;
						for (i__ = 1; i__ <= i__1; ++i__) {
							b[i__ + k * b_dim1] = temp * b[i__ + k * b_dim1];
						}
					}
					i__1 = k - 1;
					for (j = 1; j <= i__1; ++j) {
						if (a[j + k * a_dim1] != 0.f) {
							temp = a[j + k * a_dim1];
							i__2 = *m;
							for (i__ = 1; i__ <= i__2; ++i__) {
								b[i__ + j * b_dim1] -= temp * b[i__ + k *b_dim1];
							}
						}
					}
					if (*alpha != 1.f) {
						i__1 = *m;
						for (i__ = 1; i__ <= i__1; ++i__) {
							b[i__ + k * b_dim1] = *alpha * b[i__ + k * b_dim1];
						}
					}
				}
			} else {
				i__1 = *n;
				for (k = 1; k <= i__1; ++k) {
					if (nounit) {
						temp = 1.f / a[k + k * a_dim1];
						i__2 = *m;
						for (i__ = 1; i__ <= i__2; ++i__) {
							b[i__ + k * b_dim1] = temp * b[i__ + k * b_dim1];
						}
					}
					i__2 = *n;
					for (j = k + 1; j <= i__2; ++j) {
						if (a[j + k * a_dim1] != 0.f) {
							temp = a[j + k * a_dim1];
							i__3 = *m;
							for (i__ = 1; i__ <= i__3; ++i__) {
								b[i__ + j * b_dim1] -= temp * b[i__ + k *b_dim1];
							}
						}
					}
					if (*alpha != 1.f) {
						i__2 = *m;
						for (i__ = 1; i__ <= i__2; ++i__) {
							b[i__ + k * b_dim1] = *alpha * b[i__ + k * b_dim1];
						}
					}
				}
			}
		}
	}

	return 0;
} /* strsm_ */







/*  Purpose
	=======
	SGEMM  performs one of the matrix-matrix operations
	   C := alpha*op( A )*op( B ) + beta*C,
	where  op( X ) is one of
	   op( X ) = X   or   op( X ) = X',
	alpha and beta are scalars, and A, B and C are matrices, with op( A )
	an m by k matrix,  op( B )  a  k by n matrix and  C an m by n matrix.
*/

int sgemm_(char *transa, char *transb, integer *m, integer *n, integer *k, real *alpha, real *a,
		integer *lda, real *b, integer *ldb, real *beta, real *c__, integer *ldc)
{
	/* System generated locals */
	integer a_dim1, a_offset, b_dim1, b_offset, c_dim1, c_offset, i__1, i__2, i__3;
	/* Local variables */
	static integer i__, j, l, info;
	static logical nota, notb;
	static real temp;
	static integer nrowa, nrowb;

	/*  Parameter adjustments */
	a_dim1 = *lda;
	a_offset = 1 + a_dim1;
	a -= a_offset;
	b_dim1 = *ldb;
	b_offset = 1 + b_dim1;
	b -= b_offset;
	c_dim1 = *ldc;
	c_offset = 1 + c_dim1;
	c__ -= c_offset;

	/* Function Body */
	nota = lsame_(transa, (char *) "N");
	notb = lsame_(transb, (char *) "N");
	if (nota) {
		nrowa = *m;
	} else {
		nrowa = *k;
	}
	if (notb) {
		nrowb = *k;
	} else {
		nrowb = *n;
	}
	/*     Test the input parameters. */
	info = 0;
	if (! nota && ! lsame_(transa, (char *) "C") && ! lsame_(transa, (char *) "T")) {
		info = 1;
	} else if (! notb && ! lsame_(transb, (char *) "C") && ! lsame_(transb, (char *) "T")) {
		info = 2;
	} else if (*m < 0) {
		info = 3;
	} else if (*n < 0) {
		info = 4;
	} else if (*k < 0) {
		info = 5;
	} else if (*lda < MAX(1,nrowa)) {
		info = 8;
	} else if (*ldb < MAX(1,nrowb)) {
		info = 10;
	} else if (*ldc < MAX(1,*m)) {
		info = 13;
	}
	if (info != 0) {
		xerbla_((char *) "SGEMM ", &info);
		return 0;
	}
	/*     Quick return if possible. */
	if (*m == 0 || *n == 0 || ((*alpha == 0.f || *k == 0) && *beta == 1.f)) {
		return 0;
	}
	/*     And if  alpha.eq.zero. */
	if (*alpha == 0.f) {
		if (*beta == 0.f) {
			i__1 = *n;
			for (j = 1; j <= i__1; ++j) {
				i__2 = *m;
				for (i__ = 1; i__ <= i__2; ++i__) {
					c__[i__ + j * c_dim1] = 0.f;
				}
			}
		} else {
			i__1 = *n;
			for (j = 1; j <= i__1; ++j) {
				i__2 = *m;
				for (i__ = 1; i__ <= i__2; ++i__) {
					c__[i__ + j * c_dim1] = *beta * c__[i__ + j * c_dim1];
				}
			}
		}
		return 0;
	}
	/*     Start the operations. */
	if (notb) {
		if (nota) {
			/*           Form  C := alpha*A*B + beta*C. */
			i__1 = *n;
			for (j = 1; j <= i__1; ++j) {
				if (*beta == 0.f) {
					i__2 = *m;
					for (i__ = 1; i__ <= i__2; ++i__) {
						c__[i__ + j * c_dim1] = 0.f;
					}
				} else if (*beta != 1.f) {
					i__2 = *m;
					for (i__ = 1; i__ <= i__2; ++i__) {
						c__[i__ + j * c_dim1] = *beta * c__[i__ + j * c_dim1];
					}
				}
				i__2 = *k;
				for (l = 1; l <= i__2; ++l) {
					if (b[l + j * b_dim1] != 0.f) {
						temp = *alpha * b[l + j * b_dim1];
						i__3 = *m;
						for (i__ = 1; i__ <= i__3; ++i__) {
							c__[i__ + j * c_dim1] += temp * a[i__ + l * a_dim1];
						}
					}
				}
			}
		} else {
			/*           Form  C := alpha*A'*B + beta*C */
			i__1 = *n;
			for (j = 1; j <= i__1; ++j) {
				i__2 = *m;
				for (i__ = 1; i__ <= i__2; ++i__) {
					temp = 0.f;
					i__3 = *k;
					for (l = 1; l <= i__3; ++l) {
						temp += a[l + i__ * a_dim1] * b[l + j * b_dim1];
					}
					if (*beta == 0.f) {
						c__[i__ + j * c_dim1] = *alpha * temp;
					} else {
						c__[i__ + j * c_dim1] = *alpha * temp + *beta * c__[i__ + j * c_dim1];
					}
				}
			}
		}
	} else {
		if (nota) {
			/*           Form  C := alpha*A*B' + beta*C */
			i__1 = *n;
			for (j = 1; j <= i__1; ++j) {
				if (*beta == 0.f) {
					i__2 = *m;
					for (i__ = 1; i__ <= i__2; ++i__) {
						c__[i__ + j * c_dim1] = 0.f;
					}
				} else if (*beta != 1.f) {
					i__2 = *m;
					for (i__ = 1; i__ <= i__2; ++i__) {
						c__[i__ + j * c_dim1] = *beta * c__[i__ + j * c_dim1];
					}
				}
				i__2 = *k;
				for (l = 1; l <= i__2; ++l) {
					if (b[j + l * b_dim1] != 0.f) {
						temp = *alpha * b[j + l * b_dim1];
						i__3 = *m;
						for (i__ = 1; i__ <= i__3; ++i__) {
							c__[i__ + j * c_dim1] += temp * a[i__ + l * a_dim1];
						}
					}
				}
			}
		} else {
			/*           Form  C := alpha*A'*B' + beta*C */
			i__1 = *n;
			for (j = 1; j <= i__1; ++j) {
				i__2 = *m;
				for (i__ = 1; i__ <= i__2; ++i__) {
					temp = 0.f;
					i__3 = *k;
					for (l = 1; l <= i__3; ++l) {
						temp += a[l + i__ * a_dim1] * b[j + l * b_dim1];
					}
					if (*beta == 0.f) {
						c__[i__ + j * c_dim1] = *alpha * temp;
					} else {
						c__[i__ + j * c_dim1] = *alpha * temp + *beta * c__[i__ + j * c_dim1];
					}
				}
			}
		}
	}

	return 0;
} /* sgemm_ */





/*  Purpose
	=======
	SGEMV  performs one of the matrix-vector operations
	   y := alpha*A*x + beta*y,   or   y := alpha*A'*x + beta*y,
	where alpha and beta are scalars, x and y are vectors and A is an
	m by n matrix.
*/

int sgemv_(char *trans, integer *m, integer *n, real *alpha, real *a, integer *lda, real *x, integer *incx, real *beta, real *y, integer *incy)
{
	/* System generated locals */
	integer a_dim1, a_offset, i__1, i__2;
	/* Local variables */
	static integer i__, j, ix, iy, jx, jy, kx, ky, info;
	static real temp;
	static integer lenx, leny;

	/*  Parameter adjustments */
	a_dim1 = *lda;
	a_offset = 1 + a_dim1;
	a -= a_offset;
	--x;
	--y;

	/* Function Body */
	info = 0;
	if (! lsame_(trans, (char *) "N") && ! lsame_(trans, (char *) "T") && ! lsame_(trans, (char *) "C")) {
		info = 1;
	} else if (*m < 0) {
		info = 2;
	} else if (*n < 0) {
		info = 3;
	} else if (*lda < MAX(1,*m)) {
		info = 6;
	} else if (*incx == 0) {
		info = 8;
	} else if (*incy == 0) {
		info = 11;
	}
	if (info != 0) {
		xerbla_((char *) "SGEMV ", &info);
		return 0;
	}
	/*     Quick return if possible. */
	if (*m == 0 || *n == 0 || (*alpha == 0.f && *beta == 1.f)) {
		return 0;
	}
	/*     Set  LENX  and  LENY, the lengths of the vectors x and y, and set up the start points in  X  and  Y. */
	if (lsame_(trans, (char *) "N")) {
		lenx = *n;
		leny = *m;
	} else {
		lenx = *m;
		leny = *n;
	}
	if (*incx > 0) {
		kx = 1;
	} else {
		kx = 1 - (lenx - 1) * *incx;
	}
	if (*incy > 0) {
		ky = 1;
	} else {
		ky = 1 - (leny - 1) * *incy;
	}
	/*     Start the operations. In this version the elements of A are accessed sequentially with one pass through A.
           First form  y := beta*y. */
	if (*beta != 1.f) {
		if (*incy == 1) {
			if (*beta == 0.f) {
				i__1 = leny;
				for (i__ = 1; i__ <= i__1; ++i__) {
					y[i__] = 0.f;
				}
			} else {
				i__1 = leny;
				for (i__ = 1; i__ <= i__1; ++i__) {
					y[i__] = *beta * y[i__];
				}
			}
		} else {
			iy = ky;
			if (*beta == 0.f) {
				i__1 = leny;
				for (i__ = 1; i__ <= i__1; ++i__) {
					y[iy] = 0.f;
					iy += *incy;
				}
			} else {
				i__1 = leny;
				for (i__ = 1; i__ <= i__1; ++i__) {
					y[iy] = *beta * y[iy];
					iy += *incy;
				}
			}
		}
	}
	if (*alpha == 0.f) {
		return 0;
	}
	if (lsame_(trans, (char *) "N")) {
		/*        Form  y := alpha*A*x + y. */
		jx = kx;
		if (*incy == 1) {
			i__1 = *n;
			for (j = 1; j <= i__1; ++j) {
				if (x[jx] != 0.f) {
					temp = *alpha * x[jx];
					i__2 = *m;
					for (i__ = 1; i__ <= i__2; ++i__) {
						y[i__] += temp * a[i__ + j * a_dim1];
					}
				}
				jx += *incx;
			}
		} else {
			i__1 = *n;
			for (j = 1; j <= i__1; ++j) {
				if (x[jx] != 0.f) {
					temp = *alpha * x[jx];
					iy = ky;
					i__2 = *m;
					for (i__ = 1; i__ <= i__2; ++i__) {
						y[iy] += temp * a[i__ + j * a_dim1];
						iy += *incy;
					}
				}
				jx += *incx;
			}
		}
	} else {
		/*        Form  y := alpha*A'*x + y. */
		jy = ky;
		if (*incx == 1) {
			i__1 = *n;
			for (j = 1; j <= i__1; ++j) {
				temp = 0.f;
				i__2 = *m;
				for (i__ = 1; i__ <= i__2; ++i__) {
					temp += a[i__ + j * a_dim1] * x[i__];
				}
				y[jy] += *alpha * temp;
				jy += *incy;
			}
		} else {
			i__1 = *n;
			for (j = 1; j <= i__1; ++j) {
				temp = 0.f;
				ix = kx;
				i__2 = *m;
				for (i__ = 1; i__ <= i__2; ++i__) {
					temp += a[i__ + j * a_dim1] * x[ix];
					ix += *incx;
				}
				y[jy] += *alpha * temp;
				jy += *incy;
			}
		}
	}

	return 0;
} /* sgemv_ */




/*  Purpose
	=======
	scales a vector by a constant.
*/

int sscal_(integer *n, real *sa, real *sx, integer *incx)
{
	/* System generated locals */
	integer i__1, i__2;
	/* Local variables */
	static integer i__, m, mp1, nincx;

    /* Parameter adjustments */
	--sx;
	/* Function Body */
	if (*n <= 0 || *incx <= 0) {
		return 0;
	}
	if (*incx == 1) {
		goto L20;
	}
	/*        code for increment not equal to 1 */
	nincx = *n * *incx;
	i__1 = nincx;
	i__2 = *incx;
	for (i__ = 1; i__2 < 0 ? i__ >= i__1 : i__ <= i__1; i__ += i__2) {
		sx[i__] = *sa * sx[i__];
	}
	return 0;
	/*        code for increment equal to 1 clean-up loop */
L20:
	m = *n % 5;
	if (m == 0) {
		goto L40;
	}
	i__2 = m;
	for (i__ = 1; i__ <= i__2; ++i__) {
		sx[i__] = *sa * sx[i__];
	}
	if (*n < 5) {
		return 0;
	}
L40:
	mp1 = m + 1;
	i__2 = *n;
	for (i__ = mp1; i__ <= i__2; i__ += 5) {
		sx[i__] = *sa * sx[i__];
		sx[i__ + 1] = *sa * sx[i__ + 1];
		sx[i__ + 2] = *sa * sx[i__ + 2];
		sx[i__ + 3] = *sa * sx[i__ + 3];
		sx[i__ + 4] = *sa * sx[i__ + 4];
	}
	return 0;
} /* sscal_ */




/*  Purpose
	=======
	forms the dot product of two vectors.
*/

doublereal sdot_(integer *n, real *sx, integer *incx, real *sy, integer *incy)
{
	/* System generated locals */
	integer i__1;
	real ret_val;
	/* Local variables */
	static integer i__, m, ix, iy, mp1;
	static real stemp;

	/* Parameter adjustments */
	--sy;
	--sx;

	/* Function Body */
	stemp = 0.f;
	ret_val = 0.f;
	if (*n <= 0) {
		return ret_val;
	}
	if (*incx == 1 && *incy == 1) {
		goto L20;
	}
	/*        code for unequal increments or equal increments not equal to 1 */
	ix = 1;
	iy = 1;
	if (*incx < 0) {
		ix = (-(*n) + 1) * *incx + 1;
	}
	if (*incy < 0) {
		iy = (-(*n) + 1) * *incy + 1;
	}
	i__1 = *n;
	for (i__ = 1; i__ <= i__1; ++i__) {
		stemp += sx[ix] * sy[iy];
		ix += *incx;
		iy += *incy;
	}
	ret_val = stemp;
	return ret_val;
	/*        code for both increments equal to 1 clean-up loop */
L20:
	m = *n % 5;
	if (m == 0) {
		goto L40;
	}
	i__1 = m;
	for (i__ = 1; i__ <= i__1; ++i__) {
		stemp += sx[i__] * sy[i__];
	}
	if (*n < 5) {
		goto L60;
	}
L40:
	mp1 = m + 1;
	i__1 = *n;
	for (i__ = mp1; i__ <= i__1; i__ += 5) {
		stemp = stemp + sx[i__] * sy[i__] + sx[i__ + 1] * sy[i__ + 1] + sx[i__ + 2] * sy[i__ + 2] + sx[i__ + 3] * sy[i__ + 3] + sx[i__ + 4] * sy[i__ + 4];
	}
L60:
	ret_val = stemp;
	return ret_val;
} /* sdot_ */




/*	Purpose
	=======

	SPOTF2 computes the Cholesky factorization of a real symmetric
	positive definite matrix A.

	The factorization has the form
	   A = U' * U ,  if UPLO = 'U', or
	   A = L  * L',  if UPLO = 'L',
	where U is an upper triangular matrix and L is lower triangular.

	This is the unblocked version of the algorithm, calling Level 2 BLAS.
*/

int spotf2_(char *uplo, integer *n, real *a, integer *lda, integer *info)
{
	/* Table of constant values */
	static integer c__1 = 1;
	static real c_b10 = -1.f;
	static real c_b12 = 1.f;

	/* System generated locals */
	integer a_dim1, a_offset, i__1, i__2, i__3;
	real r__1;

	/* Local variables */
	static integer j;
	static real ajj;
	static logical upper;

	/* Parameter adjustments */
	a_dim1 = *lda;
	a_offset = 1 + a_dim1;
	a -= a_offset;

	/* Function Body */
	*info = 0;
	upper = lsame_(uplo, (char *) "U");
	if (! upper && ! lsame_(uplo, (char *) "L")) {
		*info = -1;
	} else if (*n < 0) {
		*info = -2;
	} else if (*lda < MAX(1,*n)) {
		*info = -4;
	}
	if (*info != 0) {
		i__1 = -(*info);
		xerbla_((char *) "SPOTF2", &i__1);
		return 0;
	}

	/*     Quick return if possible */
	if (*n == 0) {
		return 0;
	}

	if (upper) {

		/*        Compute the Cholesky factorization A = U'*U. */
		i__1 = *n;
		for (j = 1; j <= i__1; ++j) {

			/*           Compute U(J,J) and test for non-positive-definiteness. */
			i__2 = j - 1;
			ajj = a[j + j * a_dim1] - sdot_(&i__2, &a[j * a_dim1 + 1], &c__1, &a[j * a_dim1 + 1], &c__1);
			if (ajj <= 0.f) {
				a[j + j * a_dim1] = ajj;
				goto L30;
			}
			ajj = sqrt(ajj);
			a[j + j * a_dim1] = ajj;

			/*           Compute elements J+1:N of row J. */
			if (j < *n) {
				i__2 = j - 1;
				i__3 = *n - j;
				sgemv_((char *) "Transpose", &i__2, &i__3, &c_b10, &a[(j + 1) * a_dim1 + 1], lda, &a[j * a_dim1 + 1], &c__1, &c_b12, &a[j + (j + 1) * a_dim1], lda);
				i__2 = *n - j;
				r__1 = 1.f / ajj;
				sscal_(&i__2, &r__1, &a[j + (j + 1) * a_dim1], lda);
			}
		}
	} else {

		/*        Compute the Cholesky factorization A = L*L'. */
		i__1 = *n;
		for (j = 1; j <= i__1; ++j) {

			/*           Compute L(J,J) and test for non-positive-definiteness. */
			i__2 = j - 1;
			ajj = a[j + j * a_dim1] - sdot_(&i__2, &a[j + a_dim1], lda, &a[j + a_dim1], lda);
			if (ajj <= 0.f) {
				a[j + j * a_dim1] = ajj;
				goto L30;
			}
			ajj = sqrt(ajj);
			a[j + j * a_dim1] = ajj;

			/*           Compute elements J+1:N of column J. */
			if (j < *n) {
				i__2 = *n - j;
				i__3 = j - 1;
				sgemv_((char *) "No transpose", &i__2, &i__3, &c_b10, &a[j + 1 + a_dim1], lda, &a[j + a_dim1], lda, &c_b12, &a[j + 1 + j * a_dim1], &c__1);
				i__2 = *n - j;
				r__1 = 1.f / ajj;
				sscal_(&i__2, &r__1, &a[j + 1 + j * a_dim1], &c__1);
			}
		}
	}
	goto L40;

L30:
	*info = j;

L40:
	return 0;
} /* spotf2_ */

