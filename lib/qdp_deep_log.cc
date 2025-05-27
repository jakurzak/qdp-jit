#include "qdp_config_internal.h" 
#include<math.h>
#include<cmath>
#include "qdp.h"

#ifdef QDP_DEEP_LOG
#include <signal.h>
#endif


namespace QDP {


  namespace
  {
    std::shared_ptr<JITLog> __jit_log;
  }
  
  JITLog& QDP_get_global_logger()
  {
    if (!__jit_log) {
      __jit_log = make_shared<JITLog>();
    }
    return *__jit_log;
  }



  bool JITLog::fuzzy_cmp(float a,float b)
  {
    if (!std::isfinite(a) || !std::isfinite(b))
      return false;

    if (a==b)
      return true;

#if 0
    float fuzz = std::numeric_limits<float>::epsilon();

    if (fabsf(a) <= (float)jit_config_get_fuzzfactor() * fuzz  &&  fabsf(b) <= (float)jit_config_get_fuzzfactor() * fuzz)
      return true;

    if ( fabsf(1.0 - fabsf(b/a)) > jit_config_get_tolerance() )
      return false;

    return true;
#endif
    return false;
  }

    
  bool JITLog::fuzzy_cmp(double a,double b)
  {
    if (!std::isfinite(a) || !std::isfinite(b))
      return false;

    if (a==b)
      return true;

#if 0
    double fuzz = std::numeric_limits<double>::epsilon();

    if (fabs(a) <= jit_config_get_fuzzfactor() * fuzz  &&  fabs(b) <= jit_config_get_fuzzfactor() * fuzz)
      return true;

    if ( fabs(1.0 - fabs(b/a)) > jit_config_get_tolerance() )
      return false;

    return true;
#endif
    return false;
  }



  bool JITLog::fuzzy_cmp(int a,int b)
  {
    return a == b;
  }


  bool JITLog::fuzzy_cmp(bool a,bool b)
  {
    return a == b;
  }



  
  
#ifdef QDP_DEEP_LOG
  void JITLog::check_created()
  {
    if (!created)
      {
	std::string fname = std::string(jit_config_deep_log_name().c_str()) + "_" + std::to_string(Layout::nodeNumber());
	
	if (compare)
	  {
	    logger_cmp.open( fname.c_str() , ios::in | ios::binary);
	  }
	else
	  {
	    logger_cmp.open( fname.c_str() , ios::out | ios::binary);
	  }

	// check if good
	if(!logger_cmp)
	  {
	    QDPIO::cout << "Cannot open log file: " << fname << std::endl;
	    raise(SIGSEGV);
	    QDP_abort(1);
	  }
      }
    
    // mark it
    created = true;
  }




  namespace
  {
    template<class T>
    inline
    StandardOutputStream& operator<<(StandardOutputStream& s, const multi1d<T>& d)
    {
      for(int i=0; i < d.size(); ++i)
	s << d[i] << " ";
      return s;
    }
  }


#endif  

} // QDP
