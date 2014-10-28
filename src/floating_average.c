/*
 * floatingAverage.c
 *
 * Created: 25.10.2014 18:04:53
 *  Author: chris
 */ 

#include "floating_average.h"

void InitFloatAvg(tFloatAvgFilter * io_pFloatAvgFilter,
tFloatAvgType i_DefaultValue)
{
	// initialize buffer
	for (uint8_t i = 0; i < SIZE_OF_AVG; ++i)
	{
		io_pFloatAvgFilter->aData[i] = i_DefaultValue;
	}
	// put next value to the beginning of the buffer
	io_pFloatAvgFilter->IndexNextValue = 0;
}


void AddToFloatAvg(tFloatAvgFilter * io_pFloatAvgFilter,
tFloatAvgType i_NewValue)
{
	// Put new value to position
	io_pFloatAvgFilter->aData[io_pFloatAvgFilter->IndexNextValue] =
	i_NewValue;
	// Put next value the position after
	io_pFloatAvgFilter->IndexNextValue++;
	// By reaching the end start from the beginning
	io_pFloatAvgFilter->IndexNextValue %= SIZE_OF_AVG;
}


tFloatAvgType GetOutputValue(tFloatAvgFilter * io_pFloatAvgFilter)
{
	tTempSumType TempSum = 0;
	// Calculate average
	for (uint8_t i = 0; i < SIZE_OF_AVG; ++i)
	{
		TempSum += io_pFloatAvgFilter->aData[i];
	}
	// The cast is OK, if tFloatAvgType and tTempSumType are correctly chosen
	tFloatAvgType o_Result = (tFloatAvgType) (TempSum / SIZE_OF_AVG);
	return o_Result;
}