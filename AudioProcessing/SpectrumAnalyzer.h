#pragma once
#include<iterator>
#include<vector>
#include<queue>
#include<mutex>
#include<math.h>
#include<DirectXMath.h>
#include <wrl.h>

namespace AudioProcessing
{
	// cubic spline interpolation for 3 discreet consequtive points
	template <class T> class cubic_spline
	{
		T _y2,_y1,_y0;
		T _a1, _b1, _a2, _b2;
	public:
		cubic_spline(T y0,T y1, T y2)
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
	template<class T> HRESULT mapToLogScale(const T *pInput,size_t inputSize, T *pOutput, size_t outputSize, T outMin, T outMax)
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

		for (size_t outIndex=0;outIndex < outputSize;outIndex++)
		{
			int inValueIntIndex = floor(fInputIndex);

			if (fNextIndex - fInputIndex < 1.0)
			{
				if (inValueIntIndex != currentValueIndex)
				{
					// Get new next and previous values
					// And calculate new cubic spline interpolation values
					currentValueIndex = inValueIntIndex;
					T prevValue = currentValueIndex > 0 ? pInput[currentValueIndex - 1] : 0;
					T nextValue = currentValueIndex < inputSize - 1 ? pInput[currentValueIndex + 1] : 0;
					spline = cubic_spline<float>(prevValue, pInput[currentValueIndex], nextValue);	// Input array index has changed so calculate new spline
				}
				pOutput[outIndex] = spline.get_2(fInputIndex - inValueIntIndex);	// Interpolate the value
			}
			else
			{
				// More than 1 input element contributes to the value - add up and scale
				int inValueIntNextIndex = floor(fNextIndex);
				float sum = 0.f;
				// Sum up values betwen index+1 and nextIndex
				for (int index = inValueIntIndex+1; index < inValueIntNextIndex && index < inputSize; index++)
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

	class CSpectrumAnalyzer
	{
	private:
		unsigned m_AudioChannels;
		unsigned m_InputSampleRate;
		unsigned m_StepFrameCount;
		unsigned m_StepFrameOverlap;
		unsigned m_StepTotalFrames;
		unsigned m_FFTLength;
		unsigned m_FFTLengthPow2;	// FFT length expressed as power of 2

		size_t m_CopiedFrameCount;	// Keeps track of how many frames have been copied in case of a jump from sample to next
		size_t m_InputWriteIndex;	// Index into m_pInputBuffer - this is where next sample will be copied to
		float *m_pInputBuffer;

		float *m_pWindow;
		DirectX::XMVECTOR *m_pFftReal;
		DirectX::XMVECTOR *m_pFftImag;
		DirectX::XMVECTOR *m_pFftRealUnswizzled;
		DirectX::XMVECTOR *m_pFftImagUnswizzled;
		DirectX::XMVECTOR *m_pFftUnityTable;

		bool m_bUseLogScale;
		float m_fLogMin;
		float m_fLogMax;
		size_t m_logElementsCount;

		std::queue<Microsoft::WRL::ComPtr<IMFSample>> m_InputQueue;
		std::mutex m_QueueAccess;
		Microsoft::WRL::ComPtr<IMFMediaBuffer> m_spCurrentBuffer;
		REFERENCE_TIME m_hnsCurrentBufferTime;	// Timestamp of current buffer being copied
		unsigned m_CurrentBufferSampleIndex;	// Sample index into the m_spCurrentBuffer
		REFERENCE_TIME m_hnsOutputFrameTime;	// Timestamp for the output frame

		HRESULT GetNextSample();
		HRESULT AllocateBuffers();
		void FreeBuffers();
		void AnalyzeData(DirectX::XMVECTOR *pOutput,float *pRms);
		HRESULT CopyDataFromInputBuffer();
		void CopyDataToFftBuffer(int readIndex, float *pData);

		std::mutex m_ConfigAccess;
	public:
		CSpectrumAnalyzer();
		~CSpectrumAnalyzer();

		/* 
		Configures the analyzer for operation
			pInputType:
				specifies the properties of input samples (sample rate, channels)
				currently only float pcm is supported, 1 or more channels
			stepFrameCount
				Analyzer gathers this amount of samples from input stream at each pass
			stepFrameOverlap
				This amount of samples that will overlap with previous samples
			fftLength
				Indicates the number of elements in FFT tranform
				Needs to be greater or equal to stepFrameCount + stepFrameOverlap and needs to be a power of 2
		Return values:
			S_OK			Success
			E_INVALIDARG	Invalid arguments passed in
		Notes:
			output will be produced at sample rate of pInputType.SampleRate / (stepSampleCount - stepSampleOverlap)
			If this method is not called before the operation E_NOT_VALID_STATE can be returned as status
		*/
		HRESULT Configure(IMFMediaType *pInputType, unsigned stepSampleCount, unsigned stepSampleOverlap,unsigned fftLength);
		/* Queue media sample for analysis */
		HRESULT QueueInput(IMFSample *pSample);
		/* 
		Produce next spectrum data from the data that is queued at the input
		Arguments:
			pTimeStamp: Pointer to a variable that will recieve the timestamp of the analyzed audio data, based on time info in media samples
			pOutput:	Expects a float array with at least the length of fftLength (see Configure) times audio channels.
						Upon exit will contain spectrum data in an interleaved format (f1c1,f1c2,f2c1,f2,c2,f3c1,f3c2) etc.
		Return value:
			S_OK		Success
			S_FALSE		There is not enough data to complete the analyzes, you need to queue more samples to produce output
			E_NOT_VALID_STATE	Analyzer is not initialized properly, call Configure
		*/
		/*HRESULT Step(REFERENCE_TIME *pTimeStamp, float *pOutput);*/
		HRESULT Step(IMFSample **pSample);
		/* Removes all pending samples in the input queue and resets the analyzer */
		void Reset();
		/*
		Skips samples in the input queue up to the timeStamp. Next data returned from Step will be positioned at the timestamp
		Any queued data before the timeStamp will be removed from the queue
		If no source data can be found, this function will return S_FALSE
		*/
		HRESULT Skip(REFERENCE_TIME timeStamp);

		void SetLogFScale(float lowFrequency, float highFrequency, size_t numberOfBins);
		void SetLinearFScale();
	};
}

