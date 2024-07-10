#include "qdp.h"


namespace QDP
{
  namespace
  {
    // LLVM opt
    bool llvm_opt_instcombine = true;
    bool llvm_opt_inline = false;
    
    // Memory Pool
    size_t thread_stack = 512 * sizeof(REAL);
    bool use_total_pool_size = false;
    size_t pool_size = 0;
    int max_allocation_size = -1;
    size_t pool_alignment = 128;
    size_t min_total_reserved_GPU_memory = 50*1024*1024; // 50 MB
    
    // In case the Layout is not initialized when the pool size is set
    // use this fraction of free GPU memory to determine the pool size
    double free_mem_fraction = 0.92; 

    enum PoolSetMethod { User , PerThread , Fraction };
    PoolSetMethod poolSetMethod = PoolSetMethod::PerThread;
    
    // In case memory allocation fails, decrease Pool size by this amount for next try.
    size_t pool_size_decrement = 10 * 1024*1024;   // 10MB

    bool pool_count_allocations = false;

    bool verbose_output = false;

    // Kernel Launch & Tuning
    //
    bool tuning = false;
    bool tuning_verbose = false;
    int threads_per_block = 128;        // default value
    int threads_per_block_min = 8;
    int threads_per_block_max = 256;
    int threads_per_block_step = 8;
    int threads_per_block_loops = 1000; // Number of loops to measure (after dry run of 5)
    std::string tuning_file = "qdp-jit.tuning.dat";

    // Delay output when QDPIO is not ready yet
    std::vector<std::string> delayed_output;

    // Ring buffer size for OScalars
    int oscalar_ringbuffer_size = 100;

    // Timing run?
    bool timing_run = false;
    
#ifdef QDP_BACKEND_ROCM
    int  codegen_opt = 1;
    
    std::vector<std::string> extra_libs;
    std::string prepend_path = "./";

    std::string libdevice_name = "libomptarget-new-amdgpu-gfx90a-5.4.3.bc";
    std::string libdevice_path = "/libdevice/";
#endif

#ifdef QDP_BACKEND_CUDA
    int CUDA_FTZ = 0;
    int max_regs_per_block = 64*1024;
#endif

#if defined(QDP_BACKEND_ROCM) || defined (QDP_BACKEND_L0)
    bool codegen_keepfiles = false;
#endif
    
#ifdef QDP_DEEP_LOG
    bool deep_log = false;
    bool deep_log_create = false;
    std::string deep_log_name = "qdp-jit-log.dat";
    double tolerance = 1.0e-5;
    double fuzzfactor = 50;
    size_t deep_log_events = 0;
#endif

    int addressspace_local = 3;

#if defined (QDP_BACKEND_CUDA) || defined (QDP_BACKEND_ROCM)
    int addressspace_global = 0;
#elif defined (QDP_BACKEND_L0)
    int addressspace_global = 1;
#else
#error "No backend specified"
#endif


    
    int use_gpu = -1;
    
    bool gpu_direct = false;
    bool opt_shifts = false;

  }


  int  qdp_jit_config_get_global_addrspace()      { return addressspace_global;}
  void qdp_jit_config_set_global_addrspace(int v) { addressspace_global = v; }

  int  qdp_jit_config_get_local_addrspace()      { return addressspace_local;}
  void qdp_jit_config_set_local_addrspace(int v) { addressspace_local = v; }
  
  bool qdp_jit_config_get_opt_shifts() {return opt_shifts;}
  void qdp_jit_config_set_opt_shifts(bool v) { opt_shifts = v; } 

  bool jit_config_get_instcombine() { return llvm_opt_instcombine; }
  bool jit_config_get_inline()      { return llvm_opt_inline; }

  void jit_config_set_instcombine(bool b) { llvm_opt_instcombine = b; }
  void jit_config_set_inline(bool b)      { llvm_opt_inline = b; }

  int  qdp_jit_config_get_use_gpu() {return use_gpu;}
  void qdp_jit_config_set_use_gpu(int v) { use_gpu = v; } 


#ifdef QDP_BACKEND_CUDA
  int  jit_config_get_CUDA_FTZ()   { return CUDA_FTZ; }
  void jit_config_set_CUDA_FTZ(int i)   { CUDA_FTZ = i; }

  int  jit_config_get_max_regs_per_block()   { return max_regs_per_block; }
#endif


  bool jit_config_get_timing_run() { return timing_run; }
  void jit_config_set_timing_run(bool v) { timing_run = v; }
  

  void jit_config_delayed_message(std::string txt)
  {
    delayed_output.push_back(txt);
  }

  void jit_config_print_delayed_message()
  {
    for ( auto i : delayed_output )
      {
	QDPIO::cout << i << std::endl;
      }
  }
  
  void jit_config_print()
  {
#if defined (QDP_ENABLE_MANAGED_MEMORY)
    QDPIO::cout << "Using managed GPU memory\n";
#else
#if ! defined(QDP_BACKEND_AVX)
    QDPIO::cout << "Memory pool config:\n";

    switch (poolSetMethod) {
    case PoolSetMethod::PerThread:
      QDPIO::cout << "  reserved memory per thread          : " << thread_stack << " bytes\n";
      QDPIO::cout << "  resulting memory pool size          : " << pool_size/1024/1024 << " MB\n";
      break;
    case PoolSetMethod::User:
      QDPIO::cout << "  memory pool size (user request)     : " << pool_size/1024/1024 << " MB\n";
      break;
    case PoolSetMethod::Fraction:
      QDPIO::cout << "  memory pool size (per fraction)     : " << pool_size/1024/1024 << " MB\n";
      break;
    }
#endif
#endif

    
    QDPIO::cout << "Base precision                        : " << (int)BASE_PRECISION << "\n";
    
    QDPIO::cout << "Accurate timing                       : " << (int)jit_config_get_timing_run() << "\n";

    
    QDPIO::cout << "Code generation:\n";
#if defined (QDP_BACKEND_CUDA)
    QDPIO::cout << "  CUDA flush denormals to zero        : " << (int)jit_config_get_CUDA_FTZ() << "\n";
#endif
    QDPIO::cout << "  Shift optimization                  : " << (int)qdp_jit_config_get_opt_shifts() << "\n";
    QDPIO::cout << "  Propagator optimization             : ";
#if defined (QDP_PROP_OPT)
    QDPIO::cout << "1\n";
#else
    QDPIO::cout << "0\n";
#endif

#ifdef QDP_BACKEND_ROCM
    QDPIO::cout <<"  Temp Files Prepend path              : " <<  jit_config_get_prepend_path() << "\n"; 
#endif
    
    QDPIO::cout << "Launch configuration:\n";
    QDPIO::cout << "  Threads per block                   : " << threads_per_block << "\n";
#if defined (QDP_BACKEND_CUDA) || defined (QDP_BACKEND_ROCM) || defined (QDP_BACKEND_L0)
    QDPIO::cout << "  Using GPU direct                    : " << (int)jit_config_get_gpu_direct() << "\n";
#endif
  }


  std::string jit_config_get_tuning_file() { return tuning_file; }
  void jit_config_set_tuning_file( std::string v) { tuning_file = v; }
  
  bool jit_config_get_tuning() { return tuning; }
  void jit_config_set_tuning(bool v) { tuning = v; }

  bool jit_config_get_tuning_verbose() { return tuning_verbose; }
  void jit_config_set_tuning_verbose(bool v) { tuning_verbose = v; }

  bool jit_config_get_verbose_output() { return verbose_output; }
  void jit_config_set_verbose_output(bool v) { verbose_output = v; }

  int jit_config_get_oscalar_ringbuffer_size() { return oscalar_ringbuffer_size; }
  void jit_config_set_oscalar_ringbuffer_size(int n) { oscalar_ringbuffer_size = n; }

  
#ifdef QDP_BACKEND_ROCM
  std::string jit_config_get_libdevice_path() { return libdevice_path; }
  std::string jit_config_get_libdevice_name() { return libdevice_name; }

  
  std::string jit_config_get_prepend_path() { return prepend_path; }
  void        jit_config_set_prepend_path(std::string s) { prepend_path = s; }
  
  int jit_config_get_codegen_opt() { return codegen_opt; }
  void jit_config_set_codegen_opt(int opt) { codegen_opt = opt; }

  void jit_config_add_extra_lib( std::string l )
  {
    extra_libs.push_back( l );
  }
  std::vector<std::string>& jit_config_get_extra_lib()
  {
    return extra_libs;
  }
#endif

  
#if defined(QDP_BACKEND_ROCM) || defined (QDP_BACKEND_L0)
  bool jit_config_get_keepfiles() { return codegen_keepfiles; }
  void jit_config_set_keepfiles(bool v) { codegen_keepfiles = v; }
#endif

  size_t jit_config_get_pool_alignment() { return pool_alignment; }
  void jit_config_set_pool_alignment(size_t size ) { pool_alignment = size; }

  
  int jit_config_get_max_allocation() { return max_allocation_size; }
  void jit_config_set_max_allocation(int size ) { max_allocation_size = size; }
  
  bool jit_config_pool_stats() { return pool_count_allocations; }
  void jit_set_config_pool_stats() { pool_count_allocations = true; }


#ifdef QDP_DEEP_LOG
  size_t jit_config_get_log_events() { return deep_log_events; }
  void jit_config_set_log_events( unsigned e ) { deep_log_events = e; }

  double jit_config_get_fuzzfactor()           { return fuzzfactor; }
  void   jit_config_set_fuzzfactor(double i)   { fuzzfactor = i; }
  
  double jit_config_get_tolerance()           { return tolerance; }
  void   jit_config_set_tolerance(double i)   { tolerance = i; }

  bool        jit_config_deep_log() { return deep_log; }
  bool        jit_config_deep_log_create() { return deep_log_create; }
  std::string jit_config_deep_log_name() { return deep_log_name; }

  void        jit_config_deep_log_start() { deep_log = true; }
  void        jit_config_deep_log_stop() { deep_log = false; }

  void        jit_config_deep_set( std::string name , bool create )
  {
    deep_log = true;
    deep_log_create = create;
    deep_log_name = name;
  }
#endif

  
  

  size_t qdp_jit_config_pool_size_decrement()
  {
    return pool_size_decrement;
  }
  
  
  void jit_config_set_threads_per_block( int t )
  {
    threads_per_block = t;
  }

  int jit_config_get_threads_per_block()
  {
    return threads_per_block;
  }



  void jit_config_set_threads_per_block_min( int t )  {    threads_per_block_min = t;  }
  void jit_config_set_threads_per_block_max( int t )  {    threads_per_block_max = t;  }
  void jit_config_set_threads_per_block_step( int t )  {   threads_per_block_step = t;  }
  void jit_config_set_threads_per_block_loops( int t )  {   threads_per_block_loops = t;  }
  int jit_config_get_threads_per_block_min()  {    return threads_per_block_min;  }
  int jit_config_get_threads_per_block_max()  {    return threads_per_block_max;  }
  int jit_config_get_threads_per_block_step()  {   return threads_per_block_step; }
  int jit_config_get_threads_per_block_loops()  {  return threads_per_block_loops; }

  


  void jit_config_set_pool_size( size_t val )
  {
    use_total_pool_size = true;
    pool_size = val;
  }
  
  void jit_config_set_thread_stack( int stack )
  {
    thread_stack = stack;
  }

  size_t jit_config_get_pool_size()
  {
    if (use_total_pool_size)
      {
	poolSetMethod = PoolSetMethod::User;

	return pool_size;
      }
    else
      {
	size_t size;
	
	if (Layout::initialized())
	  {
	    size = gpu_mem_free() - (size_t)Layout::sitesOnNode() * thread_stack;

	    if ( (size_t)Layout::sitesOnNode() * thread_stack < min_total_reserved_GPU_memory )
	      {
		size = gpu_mem_free() - min_total_reserved_GPU_memory;
	      }

	    poolSetMethod = PoolSetMethod::PerThread;
	    pool_size = size;
	  }
	else
	  {
	    size = (size_t)((double)gpu_mem_free() * free_mem_fraction);
	    poolSetMethod = PoolSetMethod::Fraction;
	    pool_size = size;
	  }
	  
	return size;
      }
  }


#if defined(QDP_USE_PROFILING)
  namespace
  {
    std::map<std::string,int> _map_cpu;
  }

  void qdp_jit_CPU_add( const std::string& pretty )
  {
    _map_cpu[ pretty ]++;
  }
  
  std::map<std::string,int>& qdp_jit_CPU_getall()
  {
    return _map_cpu;
  }
#endif


  void jit_config_set_gpu_direct(bool g)
  {
    gpu_direct = g;
  }
  
  bool jit_config_get_gpu_direct()
  {
    return gpu_direct;
  }

  
}
