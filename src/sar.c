/*
 *  TTR: Technical Trading Rules
 *
 *  Copyright (C) 2007-2013  Joshua M. Ulrich
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <R.h>
#include <Rinternals.h>
#include <math.h>

SEXP sar (SEXP hi, SEXP lo, SEXP xl) {

    /* Initialize loop and PROTECT counters */
    int i, P=0;

    /* Ensure all arguments are double */
    if(TYPEOF(hi) != REALSXP) {
      PROTECT(hi = coerceVector(hi, REALSXP)); P++;
    }
    if(TYPEOF(lo) != REALSXP) {
      PROTECT(lo = coerceVector(lo, REALSXP)); P++;
    }
    if(TYPEOF(xl) != REALSXP) {
      PROTECT(xl = coerceVector(xl, REALSXP)); P++;
    }

    /* Pointers to function arguments */
    double *d_hi = REAL(hi);
    double *d_lo = REAL(lo);
    double *d_xl = REAL(xl);

    /* check acceleration factors */
    if(d_xl[0] <= 0) error("acceleration factor must be > 0");
    if(d_xl[1] <= d_xl[0]) error("maximum acceleration must be > acceleration factor");

    /* Input object length */
    int nr = nrows(hi);

    /* Initialize result R object */
    SEXP sar; PROTECT(sar = allocMatrix(REALSXP, nr, 1)); P++;
    double *d_sar = REAL(sar);

    /* Find first non-NA value */
    int beg = 1;
    for(i=0; i < nr; i++) {
      if( ISNA(d_hi[i]) || ISNA(d_lo[i]) ) {
        d_sar[i] = NA_REAL;
        beg++;
      } else {
        break;
      }
    }

    /* Initialize values needed by the routine */
    int sig0 = 1, sig1 = 0;
    double xpt0 = d_hi[beg-1], xpt1 = 0;
    double af0 = d_xl[0], af1 = 0;
    double lmin, lmax;

    double hi1 = d_hi[beg-1];
    double lo1 = d_lo[beg-1];
    double mu1 = (hi1 + lo1) / 2.0;

    /* Use stdev of first high and low as the initial gap */
    double initGap = sqrt((hi1-mu1)*(hi1-mu1) + (lo1-mu1)*(lo1-mu1));
    d_sar[beg-1] = d_lo[beg-1]-initGap;

    for(i=beg; i < nr; i++) {
      /* Increment signal, extreme point, and acceleration factor */
      sig1 = sig0;
      xpt1 = xpt0;
      af1 = af0;

      /* Local extrema */
      lmin = fmin(d_lo[i-1], d_lo[i]);
      lmax = fmax(d_hi[i-1], d_hi[i]);

      /* Create signal and extreme price vectors */
      if( sig1 == 1 ) {  /* Previous buy signal */
        sig0 = (d_lo[i] > d_sar[i-1]) ? 1 : -1;  /* New signal */
        xpt0 = fmax(lmax, xpt1);                 /* New extreme price */
      } else {           /* Previous sell signal */
        sig0 = (d_hi[i] < d_sar[i-1]) ? -1 : 1;  /* New signal */
        xpt0 = fmin(lmin, xpt1);                 /* New extreme price */
      }

      /*
       * Calculate acceleration factor (af)
       * and stop-and-reverse (sar) vector
       */

      /* No signal change */
      if( sig0 == sig1 ) {
        /* Initial calculations */
        d_sar[i] = d_sar[i-1] + ( xpt1 - d_sar[i-1] ) * af1;
        af0 = (af1 == d_xl[1]) ? d_xl[1] : (d_xl[0] + af1);
        /* Current buy signal */
        if( sig0 == 1 ) {
          af0 = (xpt0 > xpt1) ? af0 : af1;  /* Update acceleration factor */
          d_sar[i] = fmin(d_sar[i],lmin);   /* Determine sar value */
        }
        /* Current sell signal */
        else {
          af0 = (xpt0 < xpt1) ? af0 : af1;  /* Update acceleration factor */
          d_sar[i] = fmax(d_sar[i],lmax);   /* Determine sar value */
        }
      }
      /* New signal */
      else {
        af0 = d_xl[0];    /* reset acceleration factor */
        d_sar[i] = xpt0;  /* set sar value */
      }
    }
    
    /* UNPROTECT R objects and return result */
    UNPROTECT(P);
    return(sar);
}

