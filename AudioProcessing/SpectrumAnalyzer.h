#pragma once
#include<iterator>
#include<vector>
#include<DirectXMath.h>
#include <wrl.h>

namespace AudioProcessing
{
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

		std::queue<Microsoft::WRL::ComPtr<IMFSample>> m_InputQueue;
		Microsoft::WRL::ComPtr<IMFMediaBuffer> m_spCurrentBuffer;
		REFERENCE_TIME m_hnsCurrentBufferTime;	// Timestamp of current buffer being copied
		unsigned m_CurrentBufferSampleIndex;			// Sample index into the m_spCurrentBuffer
		REFERENCE_TIME m_hnsOutputFrameTime;	// Timestamp for the output frame

		HRESULT GetNextSample();
		HRESULT AllocateBuffers();
		void FreeBuffers();
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
		HRESULT Step(REFERENCE_TIME *pTimeStamp, float *pOutput);
		/* Removes all pending samples in the input queue and resets the analyzer */
		void Reset();
		/*
		Skips samples in the input queue up to the timeStamp. Next data returned from Step will be positioned at the timestamp
		Any queued data before the timeStamp will be removed from the queue
		If no source data can be found, this function will return S_FALSE
		*/
		HRESULT Skip(REFERENCE_TIME timeStamp);
	};
}

