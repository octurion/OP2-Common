//
// auto-generated by op2.py
//

//user function
class res_calc_kernel;

//host stub function
void op_par_loop_res_calc(char const *name, op_set set,
  op_arg arg0,
  op_arg arg1,
  op_arg arg2,
  op_arg arg3,
  op_arg arg4,
  op_arg arg5,
  op_arg arg6,
  op_arg arg7){

  int nargs = 8;
  op_arg args[8];

  args[0] = arg0;
  args[1] = arg1;
  args[2] = arg2;
  args[3] = arg3;
  args[4] = arg4;
  args[5] = arg5;
  args[6] = arg6;
  args[7] = arg7;

  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(2);
  op_timers_core(&cpu_t1, &wall_t1);
  OP_kernels[2].name      = name;
  OP_kernels[2].count    += 1;


  int    ninds   = 4;
  int    inds[8] = {0,0,1,1,2,2,3,3};

  if (OP_diags>2) {
    printf(" kernel routine with indirection: res_calc\n");
  }

  //get plan
  #ifdef OP_PART_SIZE_2
    int part_size = OP_PART_SIZE_2;
  #else
    int part_size = OP_part_size;
  #endif

  op_mpi_halo_exchanges_cuda(set, nargs, args);
  if (set->size > 0) {

    op_plan *Plan = op_plan_get_stage(name,set,part_size,nargs,args,ninds,inds,OP_COLOR2);

    cl::sycl::buffer<double,1> *arg0_buffer = static_cast<cl::sycl::buffer<double,1>*>((void*)arg0.data_d);
    cl::sycl::buffer<double,1> *arg2_buffer = static_cast<cl::sycl::buffer<double,1>*>((void*)arg2.data_d);
    cl::sycl::buffer<double,1> *arg4_buffer = static_cast<cl::sycl::buffer<double,1>*>((void*)arg4.data_d);
    cl::sycl::buffer<double,1> *arg6_buffer = static_cast<cl::sycl::buffer<double,1>*>((void*)arg6.data_d);
    cl::sycl::buffer<int,1> *map0_buffer = static_cast<cl::sycl::buffer<int,1>*>((void*)arg0.map_data_d);
    cl::sycl::buffer<int,1> *map2_buffer = static_cast<cl::sycl::buffer<int,1>*>((void*)arg2.map_data_d);
    cl::sycl::buffer<int,1> *col_reord_buffer = static_cast<cl::sycl::buffer<int,1>*>((void*)Plan->col_reord);
    int set_size = set->size+set->exec_size;
    //execute plan
    for ( int col=0; col<Plan->ncolors; col++ ){
      if (col==Plan->ncolors_core) {
        op_mpi_wait_all_cuda(nargs, args);
      }
      #ifdef OP_BLOCK_SIZE_2
      int nthread = OP_BLOCK_SIZE_2;
      #else
      int nthread = OP_block_size;
      #endif

      int start = Plan->col_offsets[0][col];
      int end = Plan->col_offsets[0][col+1];
      int nblocks = (end - start - 1)/nthread + 1;
      try {
      op2_queue->submit([&](cl::sycl::handler& cgh) {
        auto ind_arg0 = (*arg0_buffer).template get_access<cl::sycl::access::mode::read_write>(cgh);
        auto ind_arg1 = (*arg2_buffer).template get_access<cl::sycl::access::mode::read_write>(cgh);
        auto ind_arg2 = (*arg4_buffer).template get_access<cl::sycl::access::mode::read_write>(cgh);
        auto ind_arg3 = (*arg6_buffer).template get_access<cl::sycl::access::mode::read_write>(cgh);
        auto opDat0Map =  (*map0_buffer).template get_access<cl::sycl::access::mode::read>(cgh);
        auto opDat2Map =  (*map2_buffer).template get_access<cl::sycl::access::mode::read>(cgh);
        auto col_reord = (*col_reord_buffer).template get_access<cl::sycl::access::mode::read>(cgh);

        auto gm1_sycl = (*gm1_p).template get_access<cl::sycl::access::mode::read>(cgh);
        auto eps_sycl = (*eps_p).template get_access<cl::sycl::access::mode::read>(cgh);

        //user fun as lambda
        auto res_calc_gpu = [=]( const double *x1, const double *x2, const double *q1,
                               const double *q2, const double *adt1, const double *adt2,
                               double *res1, double *res2) {
            double dx, dy, mu, ri, p1, vol1, p2, vol2, f;
          
            dx = x1[0] - x2[0];
            dy = x1[1] - x2[1];
          
            ri = 1.0f / q1[0];
            p1 = gm1_sycl[0] * (q1[3] - 0.5f * ri * (q1[1] * q1[1] + q1[2] * q1[2]));
            vol1 = ri * (q1[1] * dy - q1[2] * dx);
          
            ri = 1.0f / q2[0];
            p2 = gm1_sycl[0] * (q2[3] - 0.5f * ri * (q2[1] * q2[1] + q2[2] * q2[2]));
            vol2 = ri * (q2[1] * dy - q2[2] * dx);
          
            mu = 0.5f * ((*adt1) + (*adt2)) * eps_sycl[0];
          
            f = 0.5f * (vol1 * q1[0] + vol2 * q2[0]) + mu * (q1[0] - q2[0]);
            res1[0] += f;
            res2[0] -= f;
            f = 0.5f * (vol1 * q1[1] + p1 * dy + vol2 * q2[1] + p2 * dy) +
                mu * (q1[1] - q2[1]);
            res1[1] += f;
            res2[1] -= f;
            f = 0.5f * (vol1 * q1[2] - p1 * dx + vol2 * q2[2] - p2 * dx) +
                mu * (q1[2] - q2[2]);
            res1[2] += f;
            res2[2] -= f;
            f = 0.5f * (vol1 * (q1[3] + p1) + vol2 * (q2[3] + p2)) + mu * (q1[3] - q2[3]);
            res1[3] += f;
            res2[3] -= f;
          
          };
          
        auto kern = [=](cl::sycl::nd_item<1> item) {
          int tid = item.get_global_linear_id();
          if (tid + start < end) {
            int n = col_reord[tid + start];
            //initialise local variables
            int map0idx;
            int map1idx;
            int map2idx;
            int map3idx;
            map0idx = opDat0Map[n + set_size * 0];
            map1idx = opDat0Map[n + set_size * 1];
            map2idx = opDat2Map[n + set_size * 0];
            map3idx = opDat2Map[n + set_size * 1];

            //user-supplied kernel call
            res_calc_gpu(&ind_arg0[map0idx*2],
             &ind_arg0[map1idx*2],
             &ind_arg1[map2idx*4],
             &ind_arg1[map3idx*4],
             &ind_arg2[map2idx*1],
             &ind_arg2[map3idx*1],
             &ind_arg3[map2idx*4],
             &ind_arg3[map3idx*4]);
          }

        };
        cgh.parallel_for<class res_calc_kernel>(cl::sycl::nd_range<1>(nthread*nblocks,nthread), kern);
      });
      }catch(cl::sycl::exception const &e) {
      std::cout << e.what() << std::endl;exit(-1);
      }

    }
    OP_kernels[2].transfer  += Plan->transfer;
    OP_kernels[2].transfer2 += Plan->transfer2;
  }
  op_mpi_set_dirtybit_cuda(nargs, args);
  op2_queue->wait();
  //update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[2].time     += wall_t2 - wall_t1;
}
