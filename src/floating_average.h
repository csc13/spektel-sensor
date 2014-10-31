/*
 * floating_average.h
 *
 * Created: 25.10.2014 18:05:58
 *  Author: chris
 */ 


#ifndef FLOATING_AVERAGE_H_
#define FLOATING_AVERAGE_H_

#include <inttypes.h>

// Number of values to calculate the floating average
#define SIZE_OF_AVG 9

// datatype to calculate average
typedef uint16_t tFloatAvgType;

// Internal value for calculation
// has to hold values, SIZE_OF_AVG times tFloatAvgType
typedef uint32_t tTempSumType;

// Structure to remember temporary results
typedef struct
{
	tFloatAvgType aData[SIZE_OF_AVG];
	uint8_t IndexNextValue;
} tFloatAvgFilter;

/**
 * \brief Initialize with start value
 *
 * \param io_pFloatAvgFilter Pointer to temp memory array
 * \param i_DefaultValue Default value
 */
void InitFloatAvg(tFloatAvgFilter * io_pFloatAvgFilter,
tFloatAvgType i_DefaultValue);

/**
 * \brief Add new value
 *
 * \param io_pFloatAvgFilter Pointer to temp memory array
 * \param i_ui16NewValue New value
 */
void AddToFloatAvg(tFloatAvgFilter * io_pFloatAvgFilter,
tFloatAvgType i_ui16NewValue);

/**
 * \brief Calculate the average from the last SIZE_OF_AVG values
 *
 * \param io_pFloatAvgFilter Pointer to temp memory array
 * \return Filtered value
 */
tFloatAvgType GetOutputValue(tFloatAvgFilter * io_pFloatAvgFilter);

#endif /* FLOATING_AVERAGE_H_ */