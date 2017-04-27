#pragma once
#include<iterator>
#include<vector>
#include<queue>
#include<mutex>
#include<math.h>
#include<DirectXMath.h>
#include<mfapi.h>
#include <wrl.h>
#include <concurrent_queue.h>
#include "ring_buffer.h"

// #define COPY_MEDIA_BUFFER



class CSpectrumAnalyzer
{
	struct sample_queue_item
	{
	public:
		Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
		REFERENCE_TIME time;
		sample_queue_item()
		{
			time = 0;
		}
		sample_queue_item(IMFSample *pSample)
		{
			pSample->GetSampleTime(&time);
#ifdef COPY_MEDIA_BUFFER			
			DWORD cbTotalLength = 0;
			pSample->GetTotalLength(&cbTotalLength);
			MFCreateMemoryBuffer(cbTotalLength, &buffer);
			pSample->CopyToBuffer(buffer.Get());
#else
			pSample->ConvertToContiguousBuffer(&buffer);
#endif
		}
	};
	
	const size_t cRingBufferSize = 48000 * 2 * 5;	// Allow 5 seconds of stereo data

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

	buffers::ring_buffer<float, 480000u> m_InputBuffer;		// Allocate 5 seconds stereo for 48kHz rate

	Microsoft::WRL::Wrappers::CriticalSection m_csReadPtr;

	long m_InputReadPtrSampleIndex;
	long m_ExpectedInputSampleOffset;
	float *m_pInputBuffer;


	float *m_pWindow;
	DirectX::XMVECTOR *m_pFftReal;
	DirectX::XMVECTOR *m_pFftBuffers;	// Vector buffer which is 3 times m_FFTLength
	/*DirectX::XMVECTOR *m_pFftImag;
	DirectX::XMVECTOR *m_pFftRealUnswizzled;
	DirectX::XMVECTOR *m_pFftImagUnswizzled;*/
	DirectX::XMVECTOR *m_pFftUnityTable;

	bool m_bUseLogFScale;
	float m_fLogMin;
	float m_fLogMax;
	size_t m_logElementsCount;

	bool m_bUseLogAmpScale;
	DirectX::XMVECTOR m_vClampAmpLow;
	DirectX::XMVECTOR m_vClampAmpHigh;
	
	HRESULT AllocateBuffers();
	void FreeBuffers();
	void CalculateFft(DirectX::XMVECTOR *pInput, DirectX::XMVECTOR *pOutput, float *pRms,DirectX::XMVECTOR *pBuffers);	/// Performs FFT on pInput real values and stores results in pOutput. Total RMS values will be stored in pRms
	HRESULT GetRingBufferData(float *pData,_Outptr_ REFERENCE_TIME *pTimeStamp);	// Deinterleaves and windows data from the ring buffer

	Microsoft::WRL::Wrappers::CriticalSection m_csConfigAccess;
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
	HRESULT Configure(IMFMediaType *pInputType, unsigned stepSampleCount, unsigned stepSampleOverlap, unsigned fftLength);
	
	/* Add input data to the analysis */
	HRESULT AppendInput(IMFSample *pSample);
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
	// HRESULT Step(REFERENCE_TIME *pTimeStamp, float *pOutput);
	
	HRESULT Step(IMFSample **pSample);
	/* Removes all pending samples in the input queue and resets the analyzer */
	void Reset();

	void SetLogFScale(float lowFrequency, float highFrequency, size_t numberOfBins);
	void SetLinearFScale();
	void SetLogAmplitudeScale(float clampToLow, float clampToHigh);
	void SetLinearAmplitudeScale();
};

