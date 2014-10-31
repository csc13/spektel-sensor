/*
 * filter.c
 *
 * Created: 31.10.2014 20:48:39
 *  Author: chris
 */ 

#include "filter.h"

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
void init_simple_lowpass(tSimpleLowpassReg * simpleLpReg, uint8_t shift_K) {
	simpleLpReg->shift_K = shift_K;
}

/**
 * \brief Add a value the simple lowpass and return a value
 *
 * \param io_pFloatAvgFilter Pointer to temp memory array
 * \return Filtered value
 */
int16_t simple_lowpass(tSimpleLowpassReg * simpleLpReg, int16_t input) {
	simpleLpReg->simple_filter_reg = simpleLpReg->simple_filter_reg - (simpleLpReg->simple_filter_reg >> simpleLpReg->shift_K) + input;
	return simpleLpReg->simple_filter_reg >> simpleLpReg->shift_K; 
}