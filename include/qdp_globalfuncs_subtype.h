#ifndef QDP_GLOBALFUNCS_SUBTYPE_H
#define QDP_GLOBALFUNCS_SUBTYPE_H

namespace QDP
{


  template<class T1, class C1, class T2, class C2>
  inline typename BinaryReturn<C1, C2, FnInnerProduct>::Type_t
  innerProduct(const QDPSubType<T1,C1>& s1, const QDPType<T2,C2>& s2)
  {
    return sum(localInnerProduct(s1,s2));
  }
  template<class T1, class C1, class T2, class C2>
  inline typename BinaryReturn<C1, C2, FnInnerProduct>::Type_t
  innerProduct(const QDPType<T1,C1>& s1, const QDPSubType<T2,C2>& s2)
  {
    return sum(localInnerProduct(s1,s2));
  }
#if 0
  template<class T1, class C1, class T2, class C2>
  inline typename BinaryReturn<C1, C2, FnInnerProduct>::Type_t
  innerProduct(const QDPSubType<T1,C1>& s1, const QDPSubType<T2,C2>& s2)
  {
    return sum(localInnerProduct(s1,s2));
  }
#endif

  //#if defined (QDP_BACKEND_CUDA) || defined (QDP_BACKEND_ROCM)

  template<class T1,class C1,class T2,class C2>
  typename QDPSubTypeTrait< typename BinaryReturn<C1,C2,FnLocalInnerProduct>::Type_t >::Type_t
  localInnerProduct(const QDPSubType<T1,C1> & l,const QDPType<T2,C2> & r)
  {
    if (!l.getOwnsMemory())
      QDP_error_exit("localInnerProduct with subtype view called");

    typename QDPSubTypeTrait< typename BinaryReturn<C1,C2,FnLocalInnerProduct>::Type_t >::Type_t ret;
    ret.setSubset( l.subset() );

    static JitFunction function;

    if (function.empty())
      function_OP_subtype_type_build<FnLocalInnerProduct>(function, ret, l , r );

    function_OP_exec<FnLocalInnerProduct>(function, ret, l, r, l.subset() );
    
    return ret;
  }



  template<class T1,class C1,class T2,class C2>
  typename QDPSubTypeTrait< typename BinaryReturn<C1,C2,FnLocalInnerProduct>::Type_t >::Type_t
  localInnerProduct(const QDPType<T1,C1> & l,const QDPSubType<T2,C2> & r)
  {
    if (!r.getOwnsMemory())
      QDP_error_exit("localInnerProduct with subtype view called");

    typename QDPSubTypeTrait< typename BinaryReturn<C1,C2,FnLocalInnerProduct>::Type_t >::Type_t ret;
    ret.setSubset( r.subset() );

    static JitFunction function;

    if (function.empty())
      function_OP_type_subtype_build<FnLocalInnerProduct>(function, ret, l , r );

    function_OP_exec<FnLocalInnerProduct>(function, ret, l, r, r.subset() );
    
    return ret;
  }

#if 0
  //#elif defined (QDP_BACKEND_AVX)
  template<class T1,class C1,class T2,class C2>
  typename QDPSubTypeTrait< typename BinaryReturn<C1,C2,FnLocalInnerProduct>::Type_t >::Type_t
  localInnerProduct(const QDPSubType<T1,C1> & l,const QDPType<T2,C2> & r)
  {
    if (!l.getOwnsMemory())
      QDP_error_exit("localInnerProduct with subtype view called");

    typename QDPSubTypeTrait< typename BinaryReturn<C1,C2,FnLocalInnerProduct>::Type_t >::Type_t ret;
    ret.setSubset( l.subset() );

    //QDP_info("localInnerProduct %d sites",l.subset().numSiteTable());

    const int *tab = l.subset().siteTable().slice();
    for(int j=0; j < l.subset().numSiteTable(); ++j)
      {
	int i = tab[j];
	FnLocalInnerProduct op;
	ret.getF()[j] = op( l.getF()[j] , r.elem(i) );
      }

    return ret;
  }

  template<class T1,class C1,class T2,class C2>
  typename QDPSubTypeTrait< typename BinaryReturn<C1,C2,FnLocalInnerProduct>::Type_t >::Type_t
  localInnerProduct(const QDPType<T1,C1> & l,const QDPSubType<T2,C2> & r)
  {
    if (!r.getOwnsMemory())
      QDP_error_exit("localInnerProduct with subtype view called");

    typename QDPSubTypeTrait< typename BinaryReturn<C1,C2,FnLocalInnerProduct>::Type_t >::Type_t ret;
    ret.setSubset( r.subset() );

    const int *tab = r.subset().siteTable().slice();
    for(int j=0; j < r.subset().numSiteTable(); ++j)
      {
	int i = tab[j];
	FnLocalInnerProduct op;
	ret.getF()[j] = op( l.elem(i) , r.getF()[j] );
      }

    return ret;
  }
  //#else
  //#error "No backend specified"  
#endif



#if defined (QDP_BACKEND_CUDA) || defined (QDP_BACKEND_ROCM) || defined (QDP_BACKEND_L0)
template<class T>
typename UnaryReturn<OLattice<T>, FnSum>::Type_t
sum( const OSubLattice<T>& s1 )
{
  if (!s1.getOwnsMemory())
    QDP_error_exit("sum with subtype view called");

  typename UnaryReturn<OLattice<T>, FnSum>::Type_t  d;

// #if defined(QDP_USE_PROFILING)
//     static QDPProfile_t prof(d, OpAssign(), FnSum(), s1);
//     prof.start_time();
// #endif

  // Register the destination object with the memory cache
  int d_id = QDP_get_global_cache().registrateOwnHostMem( sizeof(typename UnaryReturn<OLattice<T>, FnSum>::Type_t::SubType_t) , d.getF() , nullptr );
  
  zero_rep(d);

  typedef typename UnaryReturn<OLattice<T>, FnSum>::Type_t::SubType_t T2;
    
  //QDP_info("sum(lat,subset) dev");

  int out_id, in_id;

  unsigned actsize=s1.subset().numSiteTable();
  bool first=true;
  bool allocated=false;
  while (actsize > 0) {

    unsigned numThreads = gpu_getMaxBlockX();
    while ((numThreads*sizeof(T2) > gpu_getMaxSMem()) || (numThreads > (unsigned)actsize)) {
      numThreads >>= 1;
    }
    unsigned numBlocks=(int)ceil(float(actsize)/numThreads);
    
    if (numBlocks > gpu_getMaxGridX()) {
      QDP_error_exit( "sum(SubLat) numBlocks(%d) > maxGridX(%d)",numBlocks,(int)gpu_getMaxGridX());
    }

    //QDP_info("sum(Lat,subset): using %d threads per block, %d blocks, shared mem=%d" , numThreads , numBlocks , shared_mem_usage );

    if (first) {
      allocated=true;
      out_id = QDP_get_global_cache().add( numBlocks*sizeof(T2) , QDPCache::Flags::Empty , QDPCache::Status::undef , NULL , NULL , NULL );
      in_id  = QDP_get_global_cache().add( numBlocks*sizeof(T2) , QDPCache::Flags::Empty , QDPCache::Status::undef , NULL , NULL , NULL );
    }

    if (numBlocks == 1)
      {
	if (first)
	  {
	    qdp_jit_reduce_convert< typename ScalarType<T>::Type_t ,T2,JitDeviceLayout::Scalar>(actsize, numThreads, numBlocks,   // ok: Scalar
												s1.getId(),
												d_id );
	  }
	else
	  {
	    qdp_jit_reduce<T2>( actsize , numThreads , numBlocks, 
				in_id , d_id );
	  }
      }
    else
      {
      if (first)
	{
	  qdp_jit_reduce_convert< typename ScalarType<T>::Type_t ,T2,JitDeviceLayout::Scalar>(actsize, numThreads, numBlocks,        // ok: Scalar
											      s1.getId(),
											      out_id );
	}
      else
	{
	  qdp_jit_reduce<T2>( actsize , numThreads , numBlocks , in_id , out_id );
	}
      }
  
    first =false;
    
    if (numBlocks==1) 
      break;

    actsize=numBlocks;
    
    int tmp = in_id;
    in_id = out_id;
    out_id = tmp;
  }

  if (allocated)
    {
      QDP_get_global_cache().signoff( in_id );
      QDP_get_global_cache().signoff( out_id );
    }

  // Copy result to host
  QDP_get_global_cache().assureOnHost(d_id);

  // Global sum
  QDPInternal::globalSum(d);

  // Sign off result
  QDP_get_global_cache().signoff( d_id );

// #if defined(QDP_USE_PROFILING)
//     prof.end_time();
// #endif

  return d;
}
#elif defined (QDP_BACKEND_AVX)
template<class T>
typename UnaryReturn<OLattice<T>, FnSum>::Type_t
sum( const OSubLattice<T>& s1 )
{
  typename UnaryReturn<OLattice<T>, FnSum>::Type_t  d;

  typedef typename ScalarType<T>::Type_t T_scalar;
  
  // Must initialize to zero since we do not know if the loop will be entered
  zero_rep(d.elem());

  for(int j=0; j < s1.subset().numSiteTable(); ++j) 
    {
      d.elem() += s1.getF()[j];
    }

  // Do a global sum on the result
  QDPInternal::globalSum(d);

  return d;
 }
#else
#error "no backend specified"
#endif
  

  template<class T> 
  inline
  void zero_rep_F( T* dest, const Subset& s)
  {
    QDPIO::cout << "zero_rep_F(OSubLattice) on CPU: " << s.numSiteTable() << "\n";
    for(int j=0; j < s.numSiteTable(); ++j)
      {
	zero_rep( dest[j] );
      }
  }


  //! dest  = 0 
  template<class T>
  void zero_rep(OSubLattice<T>& dd)
  {
    if (dd.getOwnsMemory()) {
      QDPIO::cout << "OSubLattice owns\n";
      zero_rep_F(dd.getF(),dd.subset());
    } else {
      QDPIO::cout << "OSubLattice does not own\n";
      OLattice<T> tmp(dd.getId(),1.0);
      zero_rep(tmp,dd.subset());
    }
  }


} // namespace

#endif
