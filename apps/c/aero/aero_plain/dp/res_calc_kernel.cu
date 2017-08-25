//
// auto-generated by op2.py
//

//user function
__device__ void res_calc_gpu( const double **x, const double **phim, double *K,
                      double **res) {
  for (int j = 0; j < 4; j++) {
    for (int k = 0; k < 4; k++) {
      K[j * 4 + k] = 0;
    }
  }
  for (int i = 0; i < 4; i++) {
    double det_x_xi = 0;
    double N_x[8];

    double a = 0;
    for (int m = 0; m < 4; m++)
      det_x_xi += Ng2_xi[4 * i + 16 + m] * x[m][1];
    for (int m = 0; m < 4; m++)
      N_x[m] = det_x_xi * Ng2_xi[4 * i + m];

    a = 0;
    for (int m = 0; m < 4; m++)
      a += Ng2_xi[4 * i + m] * x[m][0];
    for (int m = 0; m < 4; m++)
      N_x[4 + m] = a * Ng2_xi[4 * i + 16 + m];

    det_x_xi *= a;

    a = 0;
    for (int m = 0; m < 4; m++)
      a += Ng2_xi[4 * i + m] * x[m][1];
    for (int m = 0; m < 4; m++)
      N_x[m] -= a * Ng2_xi[4 * i + 16 + m];

    double b = 0;
    for (int m = 0; m < 4; m++)
      b += Ng2_xi[4 * i + 16 + m] * x[m][0];
    for (int m = 0; m < 4; m++)
      N_x[4 + m] -= b * Ng2_xi[4 * i + m];

    det_x_xi -= a * b;

    for (int j = 0; j < 8; j++)
      N_x[j] /= det_x_xi;

    double wt1 = wtg2[i] * det_x_xi;


    double u[2] = {0.0, 0.0};
    for (int j = 0; j < 4; j++) {
      u[0] += N_x[j] * phim[j][0];
      u[1] += N_x[4 + j] * phim[j][0];
    }

    double Dk = 1.0 + 0.5 * gm1 * (m2 - (u[0] * u[0] + u[1] * u[1]));
    double rho = pow(Dk, gm1i);
    double rc2 = rho / Dk;

    for (int j = 0; j < 4; j++) {
      res[j][0] += wt1 * rho * (u[0] * N_x[j] + u[1] * N_x[4 + j]);
    }
    for (int j = 0; j < 4; j++) {
      for (int k = 0; k < 4; k++) {
        K[j * 4 + k] +=
            wt1 * rho * (N_x[j] * N_x[k] + N_x[4 + j] * N_x[4 + k]) -
            wt1 * rc2 * (u[0] * N_x[j] + u[1] * N_x[4 + j]) *
                (u[0] * N_x[k] + u[1] * N_x[4 + k]);
      }
    }
  }
}

// CUDA kernel function
__global__ void op_cuda_res_calc(
  const double *__restrict ind_arg0,
  const double *__restrict ind_arg1,
  double *__restrict ind_arg2,
  const int *__restrict opDat0Map,
  double *arg8,
  int   *ind_map,
  short *arg_map,
  int   *ind_arg_sizes,
  int   *ind_arg_offs,
  int    block_offset,
  int   *blkmap,
  int   *offset,
  int   *nelems,
  int   *ncolors,
  int   *colors,
  int   nblocks,
  int   set_size) {
  double arg9_l[1];
  double arg10_l[1];
  double arg11_l[1];
  double arg12_l[1];
  double *arg9_vec[4] = {
    arg9_l,
    arg10_l,
    arg11_l,
    arg12_l,
  };

  __shared__  int  *ind_arg2_map, ind_arg2_size;
  __shared__  double *ind_arg2_s;

  __shared__ int    nelems2, ncolor;
  __shared__ int    nelem, offset_b;

  extern __shared__ char shared[];

  if (blockIdx.x+blockIdx.y*gridDim.x >= nblocks) {
    return;
  }
  if (threadIdx.x==0) {

    //get sizes and shift pointers and direct-mapped data

    int blockId = blkmap[blockIdx.x + blockIdx.y*gridDim.x  + block_offset];

    nelem    = nelems[blockId];
    offset_b = offset[blockId];

    nelems2  = blockDim.x*(1+(nelem-1)/blockDim.x);
    ncolor   = ncolors[blockId];

    ind_arg2_size = ind_arg_sizes[0+blockId*1];

    ind_arg2_map = &ind_map[0*set_size] + ind_arg_offs[0+blockId*1];

    //set shared memory pointers
    int nbytes = 0;
    ind_arg2_s = (double *) &shared[nbytes];
  }
  __syncthreads(); // make sure all of above completed

  for ( int n=threadIdx.x; n<ind_arg2_size*1; n+=blockDim.x ){
    ind_arg2_s[n] = ZERO_double;
  }

  __syncthreads();

  for ( int n=threadIdx.x; n<nelems2; n+=blockDim.x ){
    int col2 = -1;
    int map0idx;
    int map1idx;
    int map2idx;
    int map3idx;
    if (n<nelem) {
      //initialise local variables
      for ( int d=0; d<1; d++ ){
        arg9_l[d] = ZERO_double;
      }
      for ( int d=0; d<1; d++ ){
        arg10_l[d] = ZERO_double;
      }
      for ( int d=0; d<1; d++ ){
        arg11_l[d] = ZERO_double;
      }
      for ( int d=0; d<1; d++ ){
        arg12_l[d] = ZERO_double;
      }
      map0idx = opDat0Map[n + offset_b + set_size * 0];
      map1idx = opDat0Map[n + offset_b + set_size * 1];
      map2idx = opDat0Map[n + offset_b + set_size * 2];
      map3idx = opDat0Map[n + offset_b + set_size * 3];

      const double* arg0_vec[] = {
         &ind_arg0[2 * map0idx],
         &ind_arg0[2 * map1idx],
         &ind_arg0[2 * map2idx],
         &ind_arg0[2 * map3idx]};
      const double* arg4_vec[] = {
         &ind_arg1[1 * map0idx],
         &ind_arg1[1 * map1idx],
         &ind_arg1[1 * map2idx],
         &ind_arg1[1 * map3idx]};

      //user-supplied kernel call
      res_calc_gpu(arg0_vec,
             arg4_vec,
             arg8+(n+offset_b)*16,
             arg9_vec);
      col2 = colors[n+offset_b];
    }

    //store local variables

    int arg9_map;
    int arg10_map;
    int arg11_map;
    int arg12_map;
    if (col2>=0) {
      arg9_map = arg_map[0*set_size+n+offset_b];
      arg10_map = arg_map[1*set_size+n+offset_b];
      arg11_map = arg_map[2*set_size+n+offset_b];
      arg12_map = arg_map[3*set_size+n+offset_b];
    }

    for ( int col=0; col<ncolor; col++ ){
      if (col2==col) {
        arg9_l[0] += ind_arg2_s[0+arg9_map*1];
        arg10_l[0] += ind_arg2_s[0+arg10_map*1];
        arg11_l[0] += ind_arg2_s[0+arg11_map*1];
        arg12_l[0] += ind_arg2_s[0+arg12_map*1];
        ind_arg2_s[0+arg9_map*1] = arg9_l[0];
        ind_arg2_s[0+arg10_map*1] = arg10_l[0];
        ind_arg2_s[0+arg11_map*1] = arg11_l[0];
        ind_arg2_s[0+arg12_map*1] = arg12_l[0];
      }
      __syncthreads();
    }
  }
  for ( int n=threadIdx.x; n<ind_arg2_size*1; n+=blockDim.x ){
    ind_arg2[n%1+ind_arg2_map[n/1]*1] += ind_arg2_s[n];
  }
}


//host stub function
void op_par_loop_res_calc(char const *name, op_set set,
  op_arg arg0,
  op_arg arg4,
  op_arg arg8,
  op_arg arg9){

  int nargs = 13;
  op_arg args[13];

  arg0.idx = 0;
  args[0] = arg0;
  for ( int v=1; v<4; v++ ){
    args[0 + v] = op_arg_dat(arg0.dat, v, arg0.map, 2, "double", OP_READ);
  }

  arg4.idx = 0;
  args[4] = arg4;
  for ( int v=1; v<4; v++ ){
    args[4 + v] = op_arg_dat(arg4.dat, v, arg4.map, 1, "double", OP_READ);
  }

  args[8] = arg8;
  arg9.idx = 0;
  args[9] = arg9;
  for ( int v=1; v<4; v++ ){
    args[9 + v] = op_arg_dat(arg9.dat, v, arg9.map, 1, "double", OP_INC);
  }


  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(0);
  op_timers_core(&cpu_t1, &wall_t1);
  OP_kernels[0].name      = name;
  OP_kernels[0].count    += 1;


  int    ninds   = 3;
  int    inds[13] = {0,0,0,0,1,1,1,1,-1,2,2,2,2};

  if (OP_diags>2) {
    printf(" kernel routine with indirection: res_calc\n");
  }

  //get plan
  #ifdef OP_PART_SIZE_0
    int part_size = OP_PART_SIZE_0;
  #else
    int part_size = OP_part_size;
  #endif

  int set_size = op_mpi_halo_exchanges_cuda(set, nargs, args);
  if (set->size > 0) {

    op_plan *Plan = op_plan_get_stage(name,set,part_size,nargs,args,ninds,inds,OP_STAGE_INC);

    //execute plan

    int block_offset = 0;
    for ( int col=0; col<Plan->ncolors; col++ ){
      if (col==Plan->ncolors_core) {
        op_mpi_wait_all_cuda(nargs, args);
      }
      #ifdef OP_BLOCK_SIZE_0
      int nthread = OP_BLOCK_SIZE_0;
      #else
      int nthread = OP_block_size;
      #endif

      dim3 nblocks = dim3(Plan->ncolblk[col] >= (1<<16) ? 65535 : Plan->ncolblk[col],
      Plan->ncolblk[col] >= (1<<16) ? (Plan->ncolblk[col]-1)/65535+1: 1, 1);
      if (Plan->ncolblk[col] > 0) {
        int nshared = Plan->nsharedCol[col];
        op_cuda_res_calc<<<nblocks,nthread,nshared>>>(
        (double *)arg0.data_d,
        (double *)arg4.data_d,
        (double *)arg9.data_d,
        arg0.map_data_d,
        (double*)arg8.data_d,
        Plan->ind_map,
        Plan->loc_map,
        Plan->ind_sizes,
        Plan->ind_offs,
        block_offset,
        Plan->blkmap,
        Plan->offset,
        Plan->nelems,
        Plan->nthrcol,
        Plan->thrcol,
        Plan->ncolblk[col],
        set->size+set->exec_size);

      }
      block_offset += Plan->ncolblk[col];
    }
    OP_kernels[0].transfer  += Plan->transfer;
    OP_kernels[0].transfer2 += Plan->transfer2;
  }
  op_mpi_set_dirtybit_cuda(nargs, args);
  cutilSafeCall(cudaDeviceSynchronize());
  //update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[0].time     += wall_t2 - wall_t1;
}
