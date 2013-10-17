/*
 * Open source copyright declaration based on BSD open source template:
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * This file is part of the OP2 distribution.
 *
 * Copyright (c) 2011, Mike Giles and others. Please see the AUTHORS file in
 * the main source directory for a full list of copyright holders.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of Mike Giles may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Mike Giles ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Mike Giles BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//
//     Nonlinear airfoil lift calculation
//
//     Written by Mike Giles, 2010-2011, based on FORTRAN code
//     by Devendra Ghate and Mike Giles, 2005
//

//
// standard headers
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// global constants

double gam, gm1, cfl, eps, mach, alpha, qinf[4];

//
// OP header file
//

#include "op_seq.h"


//
// kernel routines for parallel loops
//

#include "save_soln.h"
#include "adt_calc.h"
#include "res_calc.h"
#include "bres_calc.h"
#include "update.h"

// main program

int main(int argc, char **argv)
{
  // OP initialisation
  op_init(argc,argv,2);

  int    *becell, *ecell,  *bound, *bedge, *edge, *cell;
  double  *x, *q, *qold, *adt, *res;

  int    nnode,ncell,nedge,nbedge,niter;
  double  rms;

  //timer
  double cpu_t1, cpu_t2, wall_t1, wall_t2;

  // read in grid
  
  op_printf("reading in grid \n");

  FILE *fp;
  if ( (fp = fopen("/home/e160/e160/gb308/work/OP2-Common/apps/c/airfoil/airfoil_tiling/dp/new_grid.dat","r")) == NULL) {
    op_printf("can't open file new_grid.dat\n"); exit(-1);
  }

  if (fscanf(fp,"%d %d %d %d \n",&nnode, &ncell, &nedge, &nbedge) != 4) {
    op_printf("error reading from new_grid.dat\n"); exit(-1);
  }

  cell   = (int *) malloc(4*ncell*sizeof(int));
  edge   = (int *) malloc(2*nedge*sizeof(int));
  ecell  = (int *) malloc(2*nedge*sizeof(int));
  bedge  = (int *) malloc(2*nbedge*sizeof(int));
  becell = (int *) malloc(  nbedge*sizeof(int));
  bound  = (int *) malloc(  nbedge*sizeof(int));

  x      = (double *) malloc(2*nnode*sizeof(double));
  q      = (double *) malloc(4*ncell*sizeof(double));
  qold   = (double *) malloc(4*ncell*sizeof(double));
  res    = (double *) malloc(4*ncell*sizeof(double));
  adt    = (double *) malloc(  ncell*sizeof(double));

  for (int n=0; n<nnode; n++) {
    if (fscanf(fp,"%lf %lf \n",&x[2*n], &x[2*n+1]) != 2) {
      op_printf("error reading from new_grid.dat\n"); exit(-1);
    }
  }

  for (int n=0; n<ncell; n++) {
    if (fscanf(fp,"%d %d %d %d \n",&cell[4*n  ], &cell[4*n+1],
                                   &cell[4*n+2], &cell[4*n+3]) != 4) {
      op_printf("error reading from new_grid.dat\n"); exit(-1);
    }
  }

  for (int n=0; n<nedge; n++) {
    if (fscanf(fp,"%d %d %d %d \n",&edge[2*n], &edge[2*n+1],
                                   &ecell[2*n],&ecell[2*n+1]) != 4) {
      op_printf("error reading from new_grid.dat\n"); exit(-1);
    }
  }

  for (int n=0; n<nbedge; n++) {
    if (fscanf(fp,"%d %d %d %d \n",&bedge[2*n],&bedge[2*n+1],
                                   &becell[n], &bound[n]) != 4) {
      op_printf("error reading from new_grid.dat\n"); exit(-1);
    }
  }

  fclose(fp);

  // set constants and initialise flow field and residual

  op_printf("initialising flow field \n");

  gam = 1.4f;
  gm1 = gam - 1.0f;
  cfl = 0.9f;
  eps = 0.05f;

  double mach  = 0.4f;
  double alpha = 3.0f*atan(1.0f)/45.0f;
  double p     = 1.0f;
  double r     = 1.0f;
  double u     = sqrt(gam*p/r)*mach;
  double e     = p/(r*gm1) + 0.5f*u*u;

  qinf[0] = r;
  qinf[1] = r*u;
  qinf[2] = 0.0f;
  qinf[3] = r*e;

  for (int n=0; n<ncell; n++) {
    for (int m=0; m<4; m++) {
        q[4*n+m] = qinf[m];
      res[4*n+m] = 0.0f;
    }
  }

  // declare sets, pointers, datasets and global constants

  op_set nodes  = op_decl_set(nnode,  "nodes");
  op_set edges  = op_decl_set(nedge,  "edges");
  op_set bedges = op_decl_set(nbedge, "bedges");
  op_set cells  = op_decl_set(ncell,  "cells");

  op_map pedge   = op_decl_map(edges, nodes,2,edge,  "pedge");
  op_map pecell  = op_decl_map(edges, cells,2,ecell, "pecell");
  op_map pbedge  = op_decl_map(bedges,nodes,2,bedge, "pbedge");
  op_map pbecell = op_decl_map(bedges,cells,1,becell,"pbecell");
  op_map pcell   = op_decl_map(cells, nodes,4,cell,  "pcell");

  op_dat p_bound = op_decl_dat(bedges,1,"int"  ,bound,"p_bound");
  op_dat p_x     = op_decl_dat(nodes ,2,"double",x    ,"p_x");
  op_dat p_q     = op_decl_dat(cells ,4,"double",q    ,"p_q");
  op_dat p_qold  = op_decl_dat(cells ,4,"double",qold ,"p_qold");
  op_dat p_adt   = op_decl_dat(cells ,1,"double",adt  ,"p_adt");
  op_dat p_res   = op_decl_dat(cells ,4,"double",res  ,"p_res");

  op_decl_const(1,"double",&gam  );
  op_decl_const(1,"double",&gm1  );
  op_decl_const(1,"double",&cfl  );
  op_decl_const(1,"double",&eps  );
  op_decl_const(1,"double",&mach );
  op_decl_const(1,"double",&alpha);
  op_decl_const(4,"double",qinf  );

  op_diagnostic_output();

  
  //initialise timers for total execution wall time
  op_timers(&cpu_t1, &wall_t1);

  // main time-marching loop

  niter = 1000;

  for(int iter=1; iter<=niter; iter++) {

    // save old flow solution

    op_par_loop(save_soln,"save_soln", cells,
      op_arg_dat(p_q,   -1,OP_ID, 4,"double",OP_READ ),
      op_arg_dat(p_qold,-1,OP_ID, 4,"double",OP_WRITE));

    // predictor/corrector update loop

    for(int k=0; k<2; k++) {

      // calculate area/timstep
      rms = 0.0;
      
      op_par_loop(adt_calc,"adt_calc",cells,
          op_arg_dat(p_x,   0,pcell, 2,"double",OP_READ ),
          op_arg_dat(p_x,   1,pcell, 2,"double",OP_READ ),
          op_arg_dat(p_x,   2,pcell, 2,"double",OP_READ ),
          op_arg_dat(p_x,   3,pcell, 2,"double",OP_READ ),
          op_arg_dat(p_q,  -1,OP_ID, 4,"double",OP_READ ),
          op_arg_dat(p_adt,-1,OP_ID, 1,"double",OP_WRITE));
      
      
      // calculate flux residual
      
      op_par_loop(res_calc,"res_calc",edges,
          op_arg_dat(p_x,    0,pedge, 2,"double",OP_READ),
          op_arg_dat(p_x,    1,pedge, 2,"double",OP_READ),
          op_arg_dat(p_q,    0,pecell,4,"double",OP_READ),
          op_arg_dat(p_q,    1,pecell,4,"double",OP_READ),
          op_arg_dat(p_adt,  0,pecell,1,"double",OP_READ),
          op_arg_dat(p_adt,  1,pecell,1,"double",OP_READ),
          op_arg_dat(p_res,  0,pecell,4,"double",OP_INC ),
          op_arg_dat(p_res,  1,pecell,4,"double",OP_INC ));
      
      op_par_loop(bres_calc,"bres_calc",bedges,
          op_arg_dat(p_x,     0,pbedge, 2,"double",OP_READ),
          op_arg_dat(p_x,     1,pbedge, 2,"double",OP_READ),
          op_arg_dat(p_q,     0,pbecell,4,"double",OP_READ),
          op_arg_dat(p_adt,   0,pbecell,1,"double",OP_READ),
          op_arg_dat(p_res,   0,pbecell,4,"double",OP_INC ),
          op_arg_dat(p_bound,-1,OP_ID  ,1,"int",  OP_READ));
      
      // update flow field
      
      //rms = 0.0;
      
      op_par_loop(update,"update",cells,
          op_arg_dat(p_qold,-1,OP_ID, 4,"double",OP_READ ),
          op_arg_dat(p_q,   -1,OP_ID, 4,"double",OP_WRITE),
          op_arg_dat(p_res, -1,OP_ID, 4,"double",OP_RW   ),
          op_arg_dat(p_adt, -1,OP_ID, 1,"double",OP_READ ),
          op_arg_gbl(&rms,1,"double",OP_INC));
    }

    // print iteration history
    rms = sqrt(rms/(double) op_get_size(cells));
    //if (iter%100 == 0)
    if (iter%10 == 0)
      op_printf(" %d  %10.5e \n",iter,rms);
  }

  op_timers(&cpu_t2, &wall_t2);

  //output the result dat array to files
  op_print_dat_to_txtfile(p_q, "out_grid_seq.dat"); //ASCI
  op_print_dat_to_binfile(p_q, "out_grid_seq.bin"); //Binary

  op_timing_output();
  op_printf("Max total runtime = \n%f\n",wall_t2-wall_t1);

  op_exit();

  
  free(cell);
  free(edge);
  free(ecell);
  free(bedge);
  free(becell);
  free(bound);
  free(x);
  free(q);
  free(qold);
  free(res);
  free(adt);
}
