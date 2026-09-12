#include <math.h>
#include <stdio.h>
#include "labrador.h"
#include "greyhound.h"

typedef struct {
  size_t rank;
  size_t width;
  double bound;
  size_t beta;
  size_t dimension;
  double bits;
} test_vector;

int main(void) {
  size_t i,j,k;
  const double q = ldexp(1,LOGQ)-QOFF;
  const test_vector vectors[] = {
    { 21,1700,499535789,507,2976,134.355 },
    { 14,592,16228284,486,2394,128.790 },
    { 11,250,1707129,522,2176,138.330 }
  };

  if(fabs(greyhound_inner_commitment_l2_bound(2,7,123.0) -
          8*T*(ldexp(1,7)+1)*SLACK*123.0) > 1e-9 ||
     fabs(greyhound_inner_commitment_l2_bound(1,0,123.0) -
          16*T*SLACK*123.0) > 1e-9) {
    fprintf(stderr,"Greyhound inner commitment bound is not tight\n");
    return 1;
  }
  if(fabs(labrador_inner_commitment_l2_bound(2,7,1000000.0,123.0,0) -
          fmax(8*T*(ldexp(1,7)+1)*SLACK*123.0,
               2*(ldexp(1,7)+1)*SLACK*123.0+4*T*SLACK*1000000.0)) > 1e-9 ||
     fabs(labrador_inner_commitment_l2_bound(2,7,1000000.0,123.0,1) -
          fmax(8*T*(ldexp(1,7)+1)*123.0,
               2*(ldexp(1,7)+1)*123.0+4*T*SLACK*1000000.0)) > 1e-9) {
    fprintf(stderr,"LaBRADOR inner commitment bound does not match Theorem 5.1\n");
    return 1;
  }

  sis_set_security_mode(SIS_SECURITY_INVALID);
  if(sis_secure(21,1700,499535789)) {
    fprintf(stderr,"invalid SIS security mode did not fail closed\n");
    return 1;
  }
  sis_set_security_mode(SIS_SECURITY_L2_QUANTUM128_ADPS16);
  for(i=0;i<sizeof(vectors)/sizeof(vectors[0]);i++) {
    sis_estimate estimate = sis_estimate_l2_core_svp_adps16(
      vectors[i].rank,vectors[i].width,vectors[i].bound);
    if(!estimate.valid || !estimate.finite || estimate.trivially_easy ||
       estimate.beta != vectors[i].beta ||
       estimate.lattice_dimension != vectors[i].dimension ||
       fabs(estimate.quantum_bits-vectors[i].bits) > 1e-9 ||
       !sis_secure(vectors[i].rank,vectors[i].width,vectors[i].bound)) {
      fprintf(stderr,"SIS estimator mismatch in vector %zu\n",i);
      return 2;
    }
  }
  {
    /* This is the corrected 2^30-coefficient f=8, kappa=24 inner bound. */
    const double bound = 2568883812.0;
    sis_estimate estimate = sis_estimate_l2_core_svp_adps16(24,115856,bound);
    if(bound <= (q-1)/2 || bound >= q || !estimate.valid ||
       estimate.trivially_easy || !estimate.finite || estimate.beta != 491 ||
       estimate.lattice_dimension != 3144 ||
       fabs(estimate.quantum_bits-130.115) > 1e-9 ||
       !sis_secure(24,115856,bound)) {
      fprintf(stderr,"Euclidean SIS bound between q/2 and q was rejected\n");
      return 3;
    }
  }
  {
    sis_estimate estimate = sis_estimate_l2_core_svp_adps16(
      24,115856,nextafter(q,0));
    if(!estimate.valid || estimate.trivially_easy) {
      fprintf(stderr,"Euclidean SIS bound immediately below q was trivial\n");
      return 3;
    }
  }
  {
    sis_estimate estimate = sis_estimate_l2_core_svp_adps16(24,115856,q);
    if(!estimate.valid || !estimate.finite || !estimate.trivially_easy ||
       sis_secure(24,115856,q)) {
      fprintf(stderr,"Euclidean SIS bound at q was not rejected as trivial\n");
      return 3;
    }
  }
  if(sis_secure(0,1,1)) {
    fprintf(stderr,"SIS estimator accepted invalid/trivially-easy input\n");
    return 3;
  }
  if(sis_secure(13,592,16228284)) {
    fprintf(stderr,"SIS estimator accepted a 116.07-bit instance\n");
    return 4;
  }
  {
    const size_t ranks[] = {1,4,9,11,13,21,64};
    const size_t widths[] = {1,48,250,592,1700,4095};
    const double bounds[] = {1,5000,1707129,16228284,499535789,ldexp(1,31)};
    for(i=0;i<sizeof(ranks)/sizeof(ranks[0]);i++)
      for(j=0;j<sizeof(widths)/sizeof(widths[0]);j++)
        for(k=0;k<sizeof(bounds)/sizeof(bounds[0]);k++) {
          sis_estimate estimate = sis_estimate_l2_core_svp_adps16(
            ranks[i],widths[j],bounds[k]);
          int expected = estimate.valid && !estimate.trivially_easy &&
                         (!estimate.finite || estimate.quantum_bits >= 128);
          if(sis_secure(ranks[i],widths[j],bounds[k]) != expected) {
            fprintf(stderr,"Fast SIS predicate mismatch at %zu/%zu/%.0f\n",
                    ranks[i],widths[j],bounds[k]);
            return 5;
          }
        }
  }
  {
    polcomctx ctx = {};

    /* Inputs are vectors of 64-coefficient polynomials. */
    if(greyhound_test_schedule(&ctx,(size_t)1 << 23) || ctx.cpp->f != 7 ||
       ctx.cpp->b != 5 || ctx.cpp->kappa != 24 || ctx.m != 10240 ||
       ctx.n != 820 || ctx.cpp->bu != 6 || ctx.cpp->fu != 5 ||
       ctx.cpp->kappa1 != 10) {
      fprintf(stderr,"Greyhound 2^29-coefficient schedule regression\n");
      return 6;
    }
    ctx = (polcomctx){};
    if(greyhound_test_schedule(&ctx,(size_t)1 << 24) || ctx.cpp->f != 7 ||
       ctx.cpp->b != 5 || ctx.cpp->kappa != 25 || ctx.m != 14768 ||
       ctx.n != 1137 || ctx.cpp->bu != 6 || ctx.cpp->fu != 5 ||
       ctx.cpp->kappa1 != 10) {
      fprintf(stderr,"Greyhound 2^30-coefficient schedule regression\n");
      return 6;
    }
  }
  puts("SIS ADPS16 L2 security tests passed");
  return 0;
}
