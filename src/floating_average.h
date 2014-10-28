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

// Die Struktur, in der die Daten zwischengespeichert werden
typedef struct
{
	tFloatAvgType aData[SIZE_OF_AVG];
	uint8_t IndexNextValue;
} tFloatAvgFilter;


// Initialize with start value
void InitFloatAvg(tFloatAvgFilter * io_pFloatAvgFilter,
tFloatAvgType i_DefaultValue);

//add new value
void AddToFloatAvg(tFloatAvgFilter * io_pFloatAvgFilter,
tFloatAvgType i_ui16NewValue);

// Calculate the average from the last SIZE_OF_AVG values.
tFloatAvgType GetOutputValue(tFloatAvgFilter * io_pFloatAvgFilter);

#endif /* FLOATING_AVERAGE_H_ */