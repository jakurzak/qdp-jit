// $Id: junk.cc,v 1.3 2002-10-25 03:33:26 edwards Exp $

#include "tests.h"

using namespace QDP;

void junk(NmlWriter& nml, 
	  LatticeGauge& b3, const LatticeGauge& b1, const LatticeGauge& b2, 
	  const Subset& s)
{
//  Fermion f1, f2;
//  Gamma g(7);
//  f2 = g * f1;

//  b3 = b1 * b2;
  Double sum;

  nml << "b1 before shift";
  Write(nml,b1);
  
  sum = norm2(b1);
  cerr << "Norm2 before shift = " << sum << endl;
  nml << "Norm2 before shift";
  Write(nml,sum);

  b3 = shift(b1,FORWARD,0);

  nml << "b3 after shift of b1";
  Write(nml,b3);

  sum = norm2(b3);
  nml << "Norm2 after shift";
  Write(nml,sum);
  cerr << "Norm2 after shift = " << sum << endl;

  sum = innerproductReal(b3,b3);
  cerr << "Inner product = " << sum << endl;
  nml << "Inner product";
  Write(nml,sum);

  DComplex dcsum;
  dcsum = innerproduct(b3,b3);
  cerr << "Complex Inner product = " << dcsum << endl;

//  fprintf(stderr,"Test 3\n");
//  b3 = b1*b2;

//  fprintf(stderr,"Test 4\n");
//  b3 = conj(b1)*b2;

//  fprintf(stderr,"Test 5\n");
//  b3 = b1*b1*b2;
}
