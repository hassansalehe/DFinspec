#include <stdio.h>
#include <ctype.h>
#include <math.h>

#define MIN(a,b) (((a)<=(b)) ? a : b)
#define MAX(a,b) (((a)>=(b)) ? a : b)

typedef float real;
typedef float doublereal;
typedef long integer;
typedef bool logical;



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



int main() {
	return 0;
}

