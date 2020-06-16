#ifndef FLUID_DSP_H
#define FLUID_DSP_H

#ifdef __arm__
	static __INLINE int32_t __SMULWB(int16_t a,int32_t b)
	  {
			int32_t result;
			__asm__ ("smulwb %0, %1, %2":"=r"(result):"r"(b), "r"(a));
			return result;
	  }

	static __INLINE int32_t __SMLAWB(int16_t a,int32_t b,int32_t accu)
	  {
		  int32_t result ;
  		  __asm__ ("smlawb %0, %1, %2,%3":"=r" (result): "r" (b), "r" (a), "r" (accu)) ;
		  return result;
   	  }
#else

// saturate to range of int16_t
static int32_t __SSAT(int32_t x, int32_t y)
{
	if (x > (1 << (y - 1)) - 1) return (1 << (y - 1)) - 1;
	else if (x < -(1 << (y - 1))) return -(1 << (y - 1)) + 1;
	else return (int32_t)x;
}

#define __QADD(a, b) (int32_t)__SSAT(((int64_t)a+(int64_t)b),30)
#define __QSUB(a, b) (int32_t)__SSAT(((int64_t)a-(int64_t)b),30)

#endif

#ifdef FLUID_FIXED_POINT
	typedef int32_t fluid_buf_t; // q31_t / q17.15
	typedef int16_t fluid_buf16_t; // q15_t / q1.15
	#define FLUID_BUF_SAT(v) (int32_t) __SSAT(v,16)
	#define FLUID_BUF_SAT32(v) (int32_t) __SSAT(v,30)
	#define FLUID_BUF_FLOAT(v) (FLUID_BUF_SAT((v >> 15))/32768.0f)
	#define FLUID_BUF_S16(v) (v >> 15)

	#define FLUID_BUF_SAT_ADD32(a,b) __QADD(a,b)
	#define FLUID_BUF_SAT_SUB32(a,b) __QSUB(a,b)

	#define FLUID_BUF_SAMPLE(v) (v << 15)
	#ifdef BUGGY_ARM_OPTIM
		#define FLUID_BUF_MULT(a,b) __SMULWB(a,b)
		#define FLUID_BUF_MULT32(a,b) (int32_t)(((int64_t)a*(int64_t)b) >> 15)
		#define FLUID_BUF_MAC(a,b,accu) __SMLAWB(a,b,accu)
		#define FLUID_BUF_MAC32(a,b,accu) (int32_t)(((int64_t)a*b) >> 15) + accu
	#else
		#define FLUID_BUF_MULT(a,b) (int32_t)(((int64_t)a*b) >> 15)
		#define FLUID_BUF_MULT32(a,b) (int32_t)(((int64_t)a*(int64_t)b) >> 15)
		#define FLUID_BUF_MAC(a,b,accu) (int32_t)(((int64_t)a*b) >> 15) + accu
		#define FLUID_BUF_MAC32(a,b,accu) (int32_t)(((int64_t)a*b) >> 15) + accu
	#endif
	#define FLUID_REAL_TO_FRAC16(v) (int16_t)((v)*32768.0f)
	#define FLUID_REAL_TO_FRAC(v) (int32_t)((v)*32768.0f)
	#define FLUID_FRAC_TO_REAL(v) (fluid_real_t)((v)/32768.0f)
#else
	typedef float fluid_buf_t;
	typedef float fluid_buf16_t;
	#define FLUID_BUF_FLOAT(v) v
	#define FLUID_BUF_SAT(v) v
	#define FLUID_BUF_SAT32(v) v
	#define FLUID_BUF_S16(v) (v*32768.0f)
	#define FLUID_BUF_SAMPLE(v) v
	#define FLUID_BUF_SAT_ADD32(a,b) a+b
	#define FLUID_BUF_SAT_SUB32(a,b) a-b
	#define FLUID_BUF_MULT(a,b) a*b
	#define FLUID_BUF_MULT32(a,b) a*b
	#define FLUID_BUF_MAC(a,b,accu) a*b + accu
	#define FLUID_BUF_MAC32(a,b,accu) a*b + accu
	#define FLUID_REAL_TO_FRAC16(v) v
	#define FLUID_REAL_TO_FRAC(v) v
	#define FLUID_FRAC_TO_REAL(v) v
#endif
#endif