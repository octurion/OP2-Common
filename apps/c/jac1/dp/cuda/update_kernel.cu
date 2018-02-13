//
// auto-generated by op2.py
//

#include "../update.h"
//user function
__device__ void update_gpu( const double *r, double *du, double *u, double *u_sum,
                   double *u_max) {
  *u += *du + alpha * (*r);
  *du = 0.0f;
  *u_sum += (*u) * (*u);
  *u_max = maxfun(*u_max, *u);
}

// CUDA kernel function
__global__ void op_cuda_update(
  const double *__restrict arg0,
  double *arg1,
  double *arg2,
  double *arg3,
  double *arg4,
  int   set_size ) {

  double arg3_l[1];
  for ( int d=0; d<1; d++ ){
    arg3_l[d]=ZERO_double;
  }
  double arg4_l[1];
  for ( int d=0; d<1; d++ ){
    arg4_l[d]=arg4[d+blockIdx.x*1];
  }

  //process set elements
  for ( int n=threadIdx.x+blockIdx.x*blockDim.x; n<set_size; n+=blockDim.x*gridDim.x ){

    //user-supplied kernel call
    update_gpu(arg0+n*1,
           arg1+n*1,
           arg2+n*1,
           arg3_l,
           arg4_l);
  }

  //global reductions

  for ( int d=0; d<1; d++ ){
    op_reduction<OP_INC>(&arg3[d+blockIdx.x*1],arg3_l[d]);
  }
  for ( int d=0; d<1; d++ ){
    op_reduction<OP_MAX>(&arg4[d+blockIdx.x*1],arg4_l[d]);
  }
}


//host stub function
void op_par_loop_update(char const *name, op_set set,
  op_arg arg0,
  op_arg arg1,
  op_arg arg2,
  op_arg arg3,
  op_arg arg4){

  double*arg3h = (double *)arg3.data;
  double*arg4h = (double *)arg4.data;
  int nargs = 5;
  op_arg args[5];

  args[0] = arg0;
  args[1] = arg1;
  args[2] = arg2;
  args[3] = arg3;
  args[4] = arg4;

  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(1);
  op_timers_core(&cpu_t1, &wall_t1);
  OP_kernels[1].name      = name;
  OP_kernels[1].count    += 1;


  if (OP_diags>2) {
    printf(" kernel routine w/o indirection:  update");
  }

  op_mpi_halo_exchanges_cuda(set, nargs, args);
  if (set->size > 0) {

    //set CUDA execution parameters
    #ifdef OP_BLOCK_SIZE_1
      int nthread = OP_BLOCK_SIZE_1;
    #else
      int nthread = OP_block_size;
    //  int nthread = 128;
    #endif

    int nblocks = 200;

    //transfer global reduction data to GPU
    int maxblocks = nblocks;
    int reduct_bytes = 0;
    int reduct_size  = 0;
    reduct_bytes += ROUND_UP(maxblocks*1*sizeof(double));
    reduct_size   = MAX(reduct_size,sizeof(double));
    reduct_bytes += ROUND_UP(maxblocks*1*sizeof(double));
    reduct_size   = MAX(reduct_size,sizeof(double));
    reallocReductArrays(reduct_bytes);
    reduct_bytes = 0;
    arg3.data   = OP_reduct_h + reduct_bytes;
    arg3.data_d = OP_reduct_d + reduct_bytes;
    for ( int b=0; b<maxblocks; b++ ){
      for ( int d=0; d<1; d++ ){
        ((double *)arg3.data)[d+b*1] = ZERO_double;
      }
    }
    reduct_bytes += ROUND_UP(maxblocks*1*sizeof(double));
    arg4.data   = OP_reduct_h + reduct_bytes;
    arg4.data_d = OP_reduct_d + reduct_bytes;
    for ( int b=0; b<maxblocks; b++ ){
      for ( int d=0; d<1; d++ ){
        ((double *)arg4.data)[d+b*1] = arg4h[d];
      }
    }
    reduct_bytes += ROUND_UP(maxblocks*1*sizeof(double));
    mvReductArraysToDevice(reduct_bytes);

    int nshared = reduct_size*nthread;
    op_cuda_update<<<nblocks,nthread,nshared>>>(
      (double *) arg0.data_d,
      (double *) arg1.data_d,
      (double *) arg2.data_d,
      (double *) arg3.data_d,
      (double *) arg4.data_d,
      set->size );
    //transfer global reduction data back to CPU
    mvReductArraysToHost(reduct_bytes);
    for ( int b=0; b<maxblocks; b++ ){
      for ( int d=0; d<1; d++ ){
        arg3h[d] = arg3h[d] + ((double *)arg3.data)[d+b*1];
      }
    }
    arg3.data = (char *)arg3h;
    op_mpi_reduce(&arg3,arg3h);
    for ( int b=0; b<maxblocks; b++ ){
      for ( int d=0; d<1; d++ ){
        arg4h[d] = MAX(arg4h[d],((double *)arg4.data)[d+b*1]);
      }
    }
    arg4.data = (char *)arg4h;
    op_mpi_reduce(&arg4,arg4h);
  }
  op_mpi_set_dirtybit_cuda(nargs, args);
  cutilSafeCall(cudaDeviceSynchronize());
  //update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[1].time     += wall_t2 - wall_t1;
  OP_kernels[1].transfer += (float)set->size * arg0.size;
  OP_kernels[1].transfer += (float)set->size * arg1.size * 2.0f;
  OP_kernels[1].transfer += (float)set->size * arg2.size * 2.0f;
}
