/*
 * filter.h
 *
 * Created: 31.10.2014 20:48:15
 *  Author: chris
 */ 


#ifndef FILTER_32_H_
#define FILTER_32_H_

#include <stdint.h>

typedef struct
{
	int64_t simple_filter_reg;
	uint8_t shift_K;
} tSimpleLowpassReg32;

/**
 * \brief Initialize simple lowpass filter with K
 *
 * \param io_pFloatAvgFilter Pointer to temp memory array
 * \param Parameter K
 *
 * \note Normalized bandwith (to 1Hz) and rise time (nr. of samples) for various values of k
 * k=1 bandwith = 0.1197 samples = 3
 * k=2 bandwith = 0.0466 samples = 8
 * k=3 bandwith = 0.0217 samples = 16
 * k=4 bandwith = 0.0104 samples = 34 
 * k=5 bandwith = 0.0051 samples = 69
 * k=6 bandwith = 0.0026 samples = 140
 * k=7 bandwith = 0.0012 samples = 280
 * k=8 bandwith = 0.0007 samples = 561  
 */
void init_simple_lowpass_32(tSimpleLowpassReg32 * simpleLpReg, uint8_t shift_K);

/**
 * \brief Add a value the simple lowpass and return a value
 *
 * \param io_pFloatAvgFilter Pointer to temp memory array
 * \return Filtered value
 */
int32_t simple_lowpass_32(tSimpleLowpassReg32 * simpleLpReg, int32_t input);


#endif /* FILTER_32_H_ */