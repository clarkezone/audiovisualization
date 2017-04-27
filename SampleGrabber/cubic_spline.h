#pragma once
// cubic spline interpolation for 3 discreet consequtive points
template <class T> class cubic_spline
{
	T _y2, _y1, _y0;
	T _a1, _b1, _a2, _b2;
public:
	cubic_spline(T y0, T y1, T y2)
	{
		_y0 = y0;
		_y1 = y1;
		_y2 = y2;
		T k1 = (y1 - y0) + (y2 - y1) / 2;
		T k0 = (3 * (y1 - y0) - k1) / 2;
		T k2 = (3 * (y2 - y1) - k1) / 2;
		_a1 = k0 - (y1 - y0);
		_b1 = -k1 + (y1 - y0);
		_a2 = k1 - (y2 - y1);
		_b2 = -k2 + (y2 - y1);
	}
	T get_1(T x) const	// Get value on first spline between points 0 and 1
	{
		T rx = 1 - x;
		return rx*_y0 + x * _y1 + x * rx * (_a1*rx + _b1*x);
	}
	T get_2(T x) const	// Get value on first spline between points 0 and 1
	{
		T rx = 1 - x;
		return rx*_y1 + x * _y2 + x * rx * (_a2*rx + _b2*x);
	}
};

// Map input array to logarithmic distribution into output, starting from outMin value to outMax (as indexes into input array) 
template<class T> HRESULT mapToLogScale(const T *pInput, size_t inputSize, T *pOutput, size_t outputSize, T outMin, T outMax)
{
	if (outMin <= 0)
		return E_INVALIDARG;
	if (outMax >= inputSize)
		return E_INVALIDARG;
	T range = outMax / outMin;
	T outStep = pow(range, 1 / (T)outputSize);	// Output step as a geometric progression
	T fInputIndex = outMin * inputSize;
	T fNextIndex = fInputIndex * outStep;

	int currentValueIndex = -1;
	cubic_spline<T> spline(0, 0, 0);

	for (size_t outIndex = 0; outIndex < outputSize; outIndex++)
	{
		int inValueIntIndex = (int) floor(fInputIndex);

		if (fNextIndex - fInputIndex < 1.0)
		{
			if (inValueIntIndex != currentValueIndex)
			{
				// Get new next and previous values
				// And calculate new cubic spline interpolation values
				currentValueIndex = inValueIntIndex;
				T prevValue = currentValueIndex > 0 ? pInput[currentValueIndex - 1] : 0;
				T nextValue = currentValueIndex < (int)inputSize - 1 ? pInput[currentValueIndex + 1] : 0;
				spline = cubic_spline<float>(prevValue, pInput[currentValueIndex], nextValue);	// Input array index has changed so calculate new spline
			}
			pOutput[outIndex] = spline.get_2(fInputIndex - inValueIntIndex);	// Interpolate the value
		}
		else
		{
			// More than 1 input element contributes to the value - add up and scale
			int inValueIntNextIndex = (int)floor(fNextIndex);
			float sum = 0.f;
			// Sum up values betwen index+1 and nextIndex
			for (int index = inValueIntIndex + 1; index < inValueIntNextIndex && index < (int)inputSize; index++)
			{
				sum += pInput[index];
			}
			// Add end values
			sum += pInput[inValueIntIndex] * (1.f - fInputIndex + inValueIntIndex);
			if (inValueIntNextIndex < inputSize)
			{
				sum += pInput[inValueIntNextIndex] * (fNextIndex - inValueIntNextIndex);
			}
			pOutput[outIndex] = sum / (fNextIndex - fInputIndex);	// Scale the sum
		}

		fInputIndex = fNextIndex;
		fNextIndex = fInputIndex * outStep;
	}
	return S_OK;
}
