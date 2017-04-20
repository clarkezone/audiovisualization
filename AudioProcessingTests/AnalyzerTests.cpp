#include "pch.h"
#include "CppUnitTest.h"
#include"..\AudioProcessing\SpectrumAnalyzer.h"
#include"..\SampleGrabber\SampleGrabber_h.h"
#include "MFHelpers.h"
#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <DirectXMath.h>
#include "FakeClock.h"
#include "SignalGenerator.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Microsoft::WRL;
using namespace std;
using namespace Wrappers;


namespace Microsoft {
	namespace VisualStudio {
		namespace CppUnitTestFramework
		{
			template<> std::wstring ToString<REFERENCE_TIME>(const REFERENCE_TIME& time)
			{
				std::wstringstream str;
				str << time;
				return str.str();
			}
		}
	}
}


namespace AudioProcessingTests
{
	// Function to generate signal
	typedef float (SignalFunc)(size_t);

	TEST_CLASS(AnalyzerTests)
	{
	private:
		void ValidateSuccess(HRESULT hr, wchar_t *pszContext)
		{
			if (FAILED(hr))
			{
				wchar_t wszMessage[1024];
				swprintf_s(wszMessage, L"%s (hr = 0x%08X)", pszContext, hr);
				Assert::Fail(wszMessage);
			}
		}

		HRESULT CreateSignal(IMFSample **ppSample,
								size_t sampleRate,
								size_t sampleCount, 
								size_t sampleOffset, 
								vector<std::function<float (int)>> generators)
		{
			ComPtr<IMFSample> sample;
			HRESULT hr = MFCreateSample(&sample);
			sample->SetSampleTime((10000000L * (long long)sampleOffset) / sampleRate);	// Need to cast to long long to avoid overflow
			sample->SetSampleDuration((10000000L * (long long)sampleCount) / sampleRate);

			size_t channels = generators.size();
			ComPtr<IMFMediaBuffer> buffer;
			hr = MFCreateMemoryBuffer(sampleCount * sizeof(float) * channels, &buffer);	// 2 for stereo
			if (FAILED(hr))
				return hr;
			float *pBuffer;
			hr = buffer->Lock((BYTE **)&pBuffer, nullptr, nullptr);

			for (size_t sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++)
			{
				for (size_t channelIndex = 0; channelIndex < channels; channelIndex++)
				{
					pBuffer[sampleIndex * channels + channelIndex] = generators[channelIndex](sampleIndex + sampleOffset);
				}
			}

			hr = buffer->Unlock();
			hr = buffer->SetCurrentLength(sampleCount * sizeof(float) * channels);
			hr = sample->AddBuffer(buffer.Get());
			sample.CopyTo(ppSample);
			return hr;
		}

		void Create_SampleGrabber_MFT(IMFTransform **ppMFTObject,ABI::SampleGrabber::IMyInterface **ppMyInterface)
		{
			ComPtr<IActivationFactory> mftFactory;
			HRESULT hr = ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_SampleGrabber_SampleGrabberTransform).Get(), &mftFactory);
			ValidateSuccess(hr, L"Failed to get activation factory");

			ComPtr<IInspectable> object;
			hr = mftFactory->ActivateInstance(&object);
			ValidateSuccess(hr, L"Failed to get create SampleGrabberTranform object");

			ComPtr<ABI::SampleGrabber::IMyInterface> spAnalyzerOut;
			hr = object.As(&spAnalyzerOut);
			ValidateSuccess(hr, L"Unable to cast to IMyInterface");

			ComPtr<IMFTransform> spTransform;
			hr = object.As(&spTransform);
			ValidateSuccess(hr, L"Unable to cast to IMFTransform");

			spTransform.CopyTo(ppMFTObject);
			spAnalyzerOut.CopyTo(ppMyInterface);
		}

		void Configure_MediaType_MFT(IMFTransform *pMFT, unsigned sampleRate,unsigned channels)
		{
			ComPtr<IMFMediaType> spMediaType;
			CreateFloat32AudioType(sampleRate, 32, channels, &spMediaType);

			HRESULT hr = pMFT->SetInputType(0, spMediaType.Get(), 0);
			ValidateSuccess(hr, L"Failed to set input type");

			hr = pMFT->SetOutputType(0, spMediaType.Get(), 0);
			ValidateSuccess(hr, L"Failed to set output type");
		}
		void GetFrame_Before_Configure(ABI::SampleGrabber::IMyInterface *pAnalyzer)
		{		
			ComPtr<ABI::Windows::Media::IAudioFrame> spNullAudioFrame;
			HRESULT hr = pAnalyzer->GetFrame(&spNullAudioFrame);
			ValidateSuccess(hr, L"Call to getframe before adding data");
			Assert::IsNull(spNullAudioFrame.Get());
		}
		void Create_And_Assign_Clock(CFakeClock **ppClock, IMFTransform *pMFT)
		{
			ComPtr<CFakeClock> spFakeClock = Make<CFakeClock>();
			spFakeClock.CopyTo(ppClock);

			ComPtr<IMFTransform> spTransform = pMFT;
			ComPtr<IMFClockConsumer> spClockConsumer;
			HRESULT hr = spTransform.As(&spClockConsumer);
			ValidateSuccess(hr, L"Failed to cast MFT to IMFClockConsumer");

			hr = spClockConsumer->SetPresentationClock(spFakeClock.Get());
			ValidateSuccess(hr, L"Failed to set clock");
		}
		void Pump_MFT(IMFTransform *pMFT,IMFSample *pInputSample)
		{
			HRESULT hr = pMFT->ProcessInput(0, pInputSample, 0);
			ValidateSuccess(hr, L"ProcessInput failed");
			DWORD dwStatus = 0;
			MFT_OUTPUT_DATA_BUFFER outData;
			outData.dwStatus = 0;
			outData.dwStreamID = 0;
			outData.pEvents = nullptr;
			outData.pSample = nullptr;

			hr = pMFT->ProcessOutput(0, 1, &outData, &dwStatus);
			ValidateSuccess(hr, L"ProcessOutput failed");
			Assert::IsTrue(pInputSample == outData.pSample, L"MFT output sample not copied on ProcessOutput");
			if (outData.pSample != nullptr)
			{
				outData.pSample->Release();
			}
		}
		void Test_GetFrame_Timings(ABI::SampleGrabber::IMyInterface *pAnalyzer,CFakeClock *pClock,size_t testCount, REFERENCE_TIME *pClockTimes, REFERENCE_TIME *pFrameTimes,REFERENCE_TIME expectedDuration)
		{
			using namespace ABI::Windows::Media;
			using namespace ABI::Windows::Foundation;
			for (size_t testIndex = 0; testIndex < testCount; testIndex++)
			{
				if (pClockTimes[testIndex] != -1)
				{
					pClock->SetTime(pClockTimes[testIndex]);
				}
				ComPtr<IAudioFrame> spOutFrame;
				HRESULT hr = pAnalyzer->GetFrame(&spOutFrame);
				ValidateSuccess(hr, L"Unable to get frame");
				Assert::IsTrue(spOutFrame != nullptr, L"Analyzer out frame is nullptr");
				ComPtr<IMediaFrame> spMediaFrame;
				hr = spOutFrame.As(&spMediaFrame);
				ValidateSuccess(hr, L"Unable to cast to IMediaFrame");
				ComPtr<IReference<TimeSpan>> frameDuration;
				hr = spMediaFrame->get_Duration(&frameDuration);
				ValidateSuccess(hr, L"Failed to get duration");
				Assert::IsNotNull(frameDuration.Get());
				ComPtr<IReference<TimeSpan>> frameTime;
				hr = spMediaFrame->get_RelativeTime(&frameTime);
				ValidateSuccess(hr, L"Failed to get time");
				Assert::IsNotNull(frameTime.Get());

				TimeSpan tsTime, tsDuration;
				frameDuration->get_Value(&tsDuration);
				frameTime->get_Value(&tsTime);

				Assert::AreEqual(tsDuration.Duration,expectedDuration, L"Incorrect duration");
				Assert::AreEqual(tsTime.Duration,pFrameTimes[testIndex], L"Incorrect time");

			}

		}

		// Test the output of the analyzer
		void Test_AnalyzerOutput(ABI::SampleGrabber::IMyInterface *pAnalyzer)
		{
			using namespace ABI::Windows::Media;
			ComPtr <IAudioFrame> spFrame;
			HRESULT hr = pAnalyzer->GetFrame(&spFrame);
			ValidateSuccess(hr, L"Failed to get audio frame");

			ComPtr<IAudioBuffer> spBuffer;
			hr = spFrame->LockBuffer(AudioBufferAccessMode::AudioBufferAccessMode_Read, &spBuffer);

			ValidateSuccess(hr, L"Failed to get audio buffer");
		}
		void Run_MFT_Test(unsigned sampleRate, unsigned channels)
		{
			using namespace ABI::Windows::Media;
			using namespace ABI::Windows::Foundation;

			REFERENCE_TIME expectedFrameDuration = 166666;

			ComPtr<ABI::SampleGrabber::IMyInterface> spAnalyzerOut;
			ComPtr<IMFTransform> spTransform;

			Create_SampleGrabber_MFT(&spTransform, &spAnalyzerOut);

			GetFrame_Before_Configure(spAnalyzerOut.Get());

			Configure_MediaType_MFT(spTransform.Get(), sampleRate, channels);

			ComPtr<CFakeClock> spFakeClock;
			Create_And_Assign_Clock(&spFakeClock, spTransform.Get());

			HRESULT hr = spAnalyzerOut->Configure(60.0f, 0.5f, 2048);
			hr = spAnalyzerOut->SetLinearFScale();

			const unsigned T1 = 32, T2 = 128;	// Indicates one period length in samples
			const float pi = 3.14159f;
			const float w1 = 2.0f * pi * (1.0f / T1), w2 = 2.0f * pi * (1.0f / T2);

			CGenerator g(sampleRate, channels);

			// Now imitate the initial feed of input by sending 3 frames with total length of 1s
			for (size_t i = 0; i < 3; i++)
			{
				ComPtr<IMFSample> spInputSample;
				g.GetSample(&spInputSample, 16000, 
					[w1,w2] (unsigned long sampleIndex,unsigned channel)
					{
					return channel == 0 ? sinf(w1*sampleIndex) : 0.1f*cosf(w2*sampleIndex);
					}
				);
				Pump_MFT(spTransform.Get(), spInputSample.Get());
			}
			// Allow for background processing
			Sleep(300);	// Sleep for 300ms to allow processing

			// Now test for output timings
			REFERENCE_TIME setPresentationTimes[] = 
			{ 
				-1,							// Test initial condition - no time set
				expectedFrameDuration - 1,	// Test border condition -> should give frame at 0
				expectedFrameDuration,		// Test border condition -> should give frame at expectedFrameDuration
				(expectedFrameDuration * 7)>>1 // Set in the middle of 3rd frame (at 3,5 * duration)
			};
			REFERENCE_TIME expectedTimes[] = { 0,0,expectedFrameDuration,500000 };

			Test_GetFrame_Timings(spAnalyzerOut.Get(),
				spFakeClock.Get(),
				sizeof(setPresentationTimes) / sizeof(REFERENCE_TIME),
				setPresentationTimes,
				expectedTimes,
				expectedFrameDuration);

			Test_AnalyzerOutput(spAnalyzerOut.Get());
		}
	public:

		TEST_CLASS_INITIALIZE(SampleGrabber_Tests_Initialize)
		{
			HRESULT hr = MFStartup(MF_VERSION);
		}

		TEST_CLASS_CLEANUP(SampleGrabber_Tests_Cleanup)
		{
			HRESULT hr = MFShutdown();
		}

		BEGIN_TEST_METHOD_ATTRIBUTE(SampleGrabber_48000_Stereo)
			TEST_METHOD_ATTRIBUTE(L"Category", L"Analyzer")
			END_TEST_METHOD_ATTRIBUTE()
			TEST_METHOD(SampleGrabber_48000_Stereo)
		{
			Run_MFT_Test(48000, 2);
		}

		BEGIN_TEST_METHOD_ATTRIBUTE(SampleGrabber_44100_Stereo)
			TEST_METHOD_ATTRIBUTE(L"Category", L"Analyzer")
			END_TEST_METHOD_ATTRIBUTE()
			TEST_METHOD(SampleGrabber_44100_Stereo)
		{
			Run_MFT_Test(44100, 2);
		}


		BEGIN_TEST_METHOD_ATTRIBUTE(SpectrumAnalyzer_Configure)
			TEST_METHOD_ATTRIBUTE(L"Category",L"Analyzer")
		END_TEST_METHOD_ATTRIBUTE()
		TEST_METHOD(SpectrumAnalyzer_Configure)
		{
			using namespace DirectX;
			XMVECTOR vResult = XMVectorSet(0.0f, 1.0f, 0.001f, -2.0f);
			XMVECTOR vLog10Scaler = XMVectorReplicate(0.434294f); // This is 1/LogE(10)
			XMVECTOR vLogMin = XMVectorReplicate(-4.816479931f);	// This is Log10(1/2^15), dynamic range for 16 bit audio
			
			XMVECTOR vLog = XMVectorLogE(vResult) * vLog10Scaler;
			XMVECTOR vDb = XMVectorScale(XMVectorClamp(vLog, vLogMin, DirectX::g_XMOne),20.0f);

			using namespace AudioProcessing;
			using namespace Microsoft::WRL;
			CSpectrumAnalyzer analyzer;

			ComPtr<IMFMediaType> mediaType;
			MFCreateMediaType(&mediaType);
			// Test for media type error conditions
			mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
			Assert::IsTrue(FAILED(analyzer.Configure(mediaType.Get(), 4, 2, 64)), L"Only audio type should be supported");
			mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
			mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
			Assert::IsTrue(FAILED(analyzer.Configure(mediaType.Get(), 4, 2, 64)), L"Only float type should be supported");
			// Set float type, 48000Hz srate, 2 channels
			mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
			mediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000u);
			mediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2u);

			Assert::IsTrue(FAILED(analyzer.Configure(mediaType.Get(), 32, 48, 64)), L"Overall input length needs to be less than fft");
			Assert::IsTrue(FAILED(analyzer.Configure(mediaType.Get(), 4, 2, 63)), L"FFT length needs to be power of 2");

			Assert::IsTrue(SUCCEEDED(analyzer.Configure(mediaType.Get(), 800, 400, 4096)), L"Configure 60fps output with 50% overlap and 2^12 fft length");
		}


		BEGIN_TEST_METHOD_ATTRIBUTE(SpectrumAnalyzer_Analyze)
			TEST_METHOD_ATTRIBUTE(L"Category", L"Analyzer")
			END_TEST_METHOD_ATTRIBUTE()
		TEST_METHOD(SpectrumAnalyzer_Analyze)
		{
			using namespace AudioProcessing;
			using namespace Microsoft::WRL;
			CSpectrumAnalyzer analyzer;
			ComPtr<IMFMediaType> mediaType;
			const unsigned sampleRate = 48000;
			const unsigned fftLength = 4096;
			const unsigned outFrameLength = 800;
			const unsigned outFrameOverlap = 400;
			const float pi = 3.14159f;

			CreateFloat32AudioType(sampleRate, 32, 2, &mediaType);

			Assert::IsTrue(SUCCEEDED(analyzer.Configure(mediaType.Get(), outFrameLength, outFrameOverlap, fftLength)));
			ComPtr<IMFSample> spOutSample;

			// No samples queued, step needs to return S_FALSE
			Assert::AreEqual(analyzer.Step(&spOutSample), S_FALSE);

			// To test border conditions create 3 samples with lengths set to 1200,400 and 800 samples
			unsigned sampleLengths[5] = { 1200,400,800,1200,1200 };
			unsigned sampleOffset = 0;
			// Set one channel frequency at 1/32th (1500Hz) of sample rate and another one to 1/128th (375Hz)
			const unsigned T1 = 32, T2 = 128;	// Indicates one period length in samples
			const float w1 = 2.0f * pi * (1.0f / T1), w2 = 2.0f * pi * (1.0f / T2);
			for each (unsigned sampleLength in sampleLengths)
			{
				ComPtr<IMFSample> sample;

				CreateSignal(&sample, sampleRate, sampleLength, sampleOffset, vector<function<float (int)>>() = 
				{
					([w1](size_t sampleIndex) 
					{ 
						return sinf(w1*sampleIndex); 
					}),
					([w2](size_t sampleIndex) 
					{ 
						return 0.1f * cosf(w2*sampleIndex); 
					})
				});
				sampleOffset += sampleLength;
				Assert::IsTrue(SUCCEEDED(analyzer.QueueInput(sample.Get())));
			}

			REFERENCE_TIME hnsExpectedTimes[] = { 0L,166666L,333333L };

			// Now run three steps
			for (size_t step = 0; step < 3; step++)
			{
				Assert::AreEqual(analyzer.Step(&spOutSample), S_OK,L"Analyzer step not S_OK");
				float peak_volume[2] = { 0.0f,0.0f };
				int peak_index[2] = { -1, -1 };
				float total[2] = { 0,0 };

				DWORD dwBufferCount = 0;
				Assert::AreEqual(spOutSample->GetBufferCount(&dwBufferCount),S_OK,L"GetBufferCount failed");
				Assert::AreEqual(dwBufferCount, (DWORD) 1,L"Buffer count not 1");
				
				ComPtr<IMFMediaBuffer> spOutBuffer;
				Assert::AreEqual(spOutSample->ConvertToContiguousBuffer(&spOutBuffer),S_OK);

				DWORD cbMaxLength = 0, cbCurrentLength = 0;
				float *pData = nullptr;
				Assert::AreEqual(spOutBuffer->Lock((BYTE **) &pData, &cbMaxLength, &cbCurrentLength),S_OK,L"Failed to lock buffer");
				Assert::IsTrue(cbMaxLength >= sizeof(float) * (fftLength >> 1) * 2,L"Invalid buffer maximum length");
				Assert::IsTrue(cbCurrentLength >= sizeof(float) * (fftLength >> 1) * 2,L"Invalid buffer current length");

				for (size_t channel = 0; channel < 2; channel++)
				{
					float *pChannelData = pData + channel * (fftLength >> 1);

					for (size_t index = 0; index < fftLength >> 1; index ++)
					{
						total[channel] += pChannelData[index];
						// Find maximum
						if (pChannelData[index] > peak_volume[channel])
						{
							peak_index[channel] = (index) >> 1;
							peak_volume[channel] = pChannelData[index];
						}
					}

				}
				REFERENCE_TIME hnsSampleTime;
				spOutSample->GetSampleTime(&hnsSampleTime);
				// Validate the timing info
				Assert::IsTrue(hnsExpectedTimes[step] == hnsSampleTime,L"Invalid timing information");
				REFERENCE_TIME hnsSampleDuration;
				spOutSample->GetSampleDuration(&hnsSampleDuration);
				REFERENCE_TIME hnsExpectedLength = 10000000L * (long long) outFrameLength / sampleRate;
				Assert::IsTrue( hnsExpectedLength == hnsSampleDuration,L"Invalid output sample length");

				// Make sure the spectrum maximums are at defined 
				Assert::AreEqual(peak_index[0], (int)(fftLength / T1)>>1);
				Assert::AreEqual(peak_index[1], (int)(fftLength / T2)>>1);
			}
			REFERENCE_TIME skipTo = 833333;	// This should be frame 4000
			Assert::IsTrue(analyzer.Skip(skipTo) == S_OK);	// Skip to sample 4000
															// This should leave just 800 frames in input queue
			Assert::IsTrue(analyzer.Step(&spOutSample) == S_OK);
			REFERENCE_TIME hnsSampleTime;
			spOutSample->GetSampleTime(&hnsSampleTime);
			Assert::IsTrue(hnsSampleTime == skipTo);
		}	


		BEGIN_TEST_METHOD_ATTRIBUTE(SpectrumAnalyzer_ToLogScale)
			TEST_METHOD_ATTRIBUTE(L"Category", L"Analyzer")
			END_TEST_METHOD_ATTRIBUTE()
			TEST_METHOD(SpectrumAnalyzer_ToLogScale)
		{
			std::vector<float> input(256);	// output from 512 FFT, 48000, bin width 93.75Hz
			// Set some signals
			input[1] = 1.0f;
			input[8] = 0.2f;
			input[9] = 0.6f;
			input[10] = 0.2f;
			// Convert to 20 bins of logarithmic distribution from 20...20000Hz
			std::vector<float> output(20);
			AudioProcessing::mapToLogScale(&input[0],256, &output[0],20, 20.f/24000.f, 20000.f/24000.f);

			float expected[] = { 
				0.138372749f,0.211934686f,0.329434842f,0.517418683f,
				0.812474906f,1.01610470f,0.513137400f,-0.191709042,
				0.f,0.f,0.185964912f,0.122723825f };

			for (size_t i = 0; i < output.size(); i++)
			{
				if (i < 12)
					Assert::AreEqual(expected[i], output[i]);
				else
					Assert::AreEqual(0.0f, output[i]);
			}
		}
	};
}