#include "pch.h"
#include "CppUnitTest.h"
#include"..\AudioProcessing\SpectrumAnalyzer.h"
#include"..\SampleGrabber\cubic_spline.h"
#include"..\SampleGrabber\ring_buffer.h"
#include"..\SampleGrabber\SampleGrabber_h.h"
#include "MFHelpers.h"
#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <DirectXMath.h>
#include "FakeClock.h"
#include "SignalGenerator.h"
#include <windows.foundation.h>
#include <MemoryBuffer.h>
#include <concurrent_queue.h>

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

	template <class T, class U> ComPtr<T> As(Microsoft::WRL::ComPtr<U> spU)
	{
		ComPtr<T> spT;
		spU.As(&spT);
		return spT;
	}

	class CAudioBufferHelper
	{
		Microsoft::WRL::ComPtr<ABI::Windows::Media::IAudioBuffer> m_spBuffer;
		Microsoft::WRL::ComPtr<ABI::Windows::Foundation::IMemoryBufferReference> m_spRef;
		float *m_pData;
		UINT32 m_Length;
	public:
		UINT32 GetLength() { return m_Length; }
		float *GetBuffer() { return m_pData; }
		CAudioBufferHelper(ABI::Windows::Media::IAudioFrame *pFrame, ABI::Windows::Media::AudioBufferAccessMode mode = ABI::Windows::Media::AudioBufferAccessMode::AudioBufferAccessMode_Read)
		{
			using namespace ABI::Windows::Foundation;
			HRESULT hr = pFrame->LockBuffer(mode, &m_spBuffer);
			Assert::IsTrue(hr == S_OK, L"LockBuffer", LINE_INFO());
			hr = m_spBuffer->get_Length(&m_Length);
			Assert::IsTrue(hr == S_OK, L"GetLength", LINE_INFO());

			As<IMemoryBuffer>(m_spBuffer)->CreateReference(&m_spRef);

			ComPtr<Windows::Foundation::IMemoryBufferByteAccess> spByteAccess;
			hr = m_spRef.As(&spByteAccess);
			Assert::IsTrue(hr == S_OK, L"AsByteAccess", LINE_INFO());

			UINT32 cbBufferLength = 0;
			hr = spByteAccess->GetBuffer((BYTE **)&m_pData, &cbBufferLength);
			Assert::IsTrue(hr == S_OK, L"GetBuffer", LINE_INFO());
		}
		~CAudioBufferHelper()
		{
			using namespace ABI::Windows::Foundation;
			As<IClosable>(m_spRef)->Close();
			As<IClosable>(m_spBuffer)->Close();
		}
	};

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
			vector<std::function<float(int)>> generators)
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

		void Create_SampleGrabber_MFT(IMFTransform **ppMFTObject, ABI::SampleGrabber::IMyInterface **ppMyInterface)
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

		void Configure_MediaType_MFT(IMFTransform *pMFT, unsigned sampleRate, unsigned channels)
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
		void Pump_MFT(IMFTransform *pMFT, IMFSample *pInputSample)
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
		void Test_GetFrame_Timings(ABI::SampleGrabber::IMyInterface *pAnalyzer, CFakeClock *pClock, size_t testCount, REFERENCE_TIME *pClockTimes, REFERENCE_TIME *pFrameTimes, REFERENCE_TIME expectedDuration)
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
				Assert::IsTrue(spOutFrame != nullptr, L"Analyzer out frame is nullptr",LINE_INFO());
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

				Assert::AreEqual(expectedDuration, tsDuration.Duration, L"Incorrect duration",LINE_INFO());
				Assert::AreEqual(pFrameTimes[testIndex], tsTime.Duration, L"Incorrect time",LINE_INFO());
			}

		}

		REFERENCE_TIME Test_GetFrame_Continuity(ABI::SampleGrabber::IMyInterface *pAnalyzer, CFakeClock *pClock, REFERENCE_TIME fromTime, REFERENCE_TIME step)
		{
			using namespace ABI::Windows::Media;
			using namespace ABI::Windows::Foundation;
			REFERENCE_TIME time = fromTime;
			int computedFrameOffset = -1;
			ComPtr <IAudioFrame> spFrame;

			do
			{
				pClock->SetTime(time);
				HRESULT hr = pAnalyzer->GetFrame(&spFrame);
				ValidateSuccess(hr, L"Failed to get audio frame");
				if (spFrame != nullptr)
				{
					ComPtr<IMediaFrame> spMediaFrame;
					hr = spFrame.As(&spMediaFrame);
					ValidateSuccess(hr, L"Failed to cast to IMediaFrame");
					ComPtr<IReference<TimeSpan>> spTime;
					hr = spMediaFrame->get_RelativeTime(&spTime);
					ValidateSuccess(hr, L"Failed to get time from output frame");
					Assert::IsNotNull(spTime.Get(), L"RelativeTime property is null");
					ComPtr<IReference<TimeSpan>> spDuration;
					hr = spMediaFrame->get_Duration(&spDuration);
					ValidateSuccess(hr, L"Failed to get duration from output frame");
					Assert::IsNotNull(spTime.Get(), L"Duration property is null");
					TimeSpan time, duration;
					spTime->get_Value(&time);
					spDuration->get_Value(&duration);

					// Do the math in number of frames to avoid int math rounding errors
					// Does not matter what the frame rate is really so use 48k
					// This is effectively round with precision 1/48000
					unsigned numberOfFrames = (unsigned)((48000u * duration.Duration + 5000000) / 10000000L);
					unsigned frameOffset = (unsigned)((48000u * time.Duration + 5000000) / 10000000L);

					if (computedFrameOffset != -1)
					{
						Assert::AreEqual(frameOffset, (unsigned)computedFrameOffset, L"Discontinuity in frames");
					}
					computedFrameOffset = frameOffset + numberOfFrames;
				}
				time += step;
			} while (spFrame != nullptr);

			return 10000000L * (long long)computedFrameOffset / 48000L;
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

		void Test_Zero_Levels(ABI::SampleGrabber::IMyInterface *pAnalyzer, size_t fftLength, size_t channels)
		{
			using namespace ABI::Windows::Media;
			using namespace ABI::Windows::Foundation;
			ComPtr <IAudioFrame> spFrame;
			HRESULT hr = pAnalyzer->GetFrame(&spFrame);
			ValidateSuccess(hr, L"Failed to get audio frame");

			CAudioBufferHelper b(spFrame.Get());
			UINT32 length = b.GetLength();
			UINT32 expectedLength = (fftLength * channels) + 8;
			Assert::AreEqual(expectedLength * sizeof(float), length, L"Unexpected buffer length", LINE_INFO());

			float *pBuffer = b.GetBuffer();

			// Test output for silence
			for (size_t channelIndex = 0; channelIndex < channels; channelIndex++)
			{
				float *pOut = pBuffer + channelIndex * fftLength;
				for (size_t index = 0; index < fftLength; index++)
				{
					if (pOut[index] != -100.0f)
					{
						wchar_t szMessage[1024];
						swprintf_s(szMessage, L"FFT output not -100db at index %d channel %d", index, channelIndex);
						Assert::AreEqual(-100.0f, pBuffer[index], szMessage);
					}
				}
				float *pRMS = pBuffer + channels * fftLength + channelIndex;
				Assert::AreEqual(-100.0f, *pRMS, L"Invalid RMS value");
			}
		}
		void Test_LogFOutput(IMFTransform *pMft, ABI::SampleGrabber::IMyInterface *pAnalyzer, CFakeClock *pClock,float outputFrameRate,size_t sampleRate, size_t channels)
		{
			size_t outputSize = 1000;
			using namespace ABI::Windows::Media;
			pAnalyzer->SetLogFScale(20.0f, 20000.0f, outputSize);
			

			// Generate two signals - one at 100Hz and the other at 1000Hz multiplied by 1.5 for each channel
			// With amplitude of  1 1/4 1/16 etc.
			std::vector<float> w1(channels),w2(channels),f1(channels),f2(channels);
			std::vector<float> a(channels),db(channels);
			float base_f1 = 100.0f, base_f2 = 1000.0f;
			float base_amp = 1.0;
			const float pi = 3.14159f;
			for (size_t channelIndex = 0; channelIndex < channels; channelIndex++,base_f1*=1.5f,base_f2*=1.5,base_amp*=0.25)
			{
				w1[channelIndex] = 2.0f*pi*base_f1 / (float) sampleRate;
				w2[channelIndex] = 2.0f*pi*base_f2 / (float) sampleRate;
				a[channelIndex] = base_amp;
				f1[channelIndex] = base_f1;
				f2[channelIndex] = base_f2;
				db[channelIndex] = 20.0f * log10f(base_amp);
			}
			CGenerator g(sampleRate, channels);

			const size_t iterationSampleCount = 16000;
			const size_t preRollIterations = 6;

			for (size_t iterations = 0; iterations < preRollIterations; iterations++)
			{
				ComPtr<IMFSample> spInputSample;
				g.GetSample(&spInputSample, iterationSampleCount,
					[w1, w2, a](unsigned long sampleIndex, unsigned channel)
				{
					return 0.5f*a[channel] * sinf(w1[channel] * sampleIndex) + a[channel] * sinf(w2[channel] * sampleIndex);
				}
				);
				Pump_MFT(pMft, spInputSample.Get());
			}

			// Allow for background processing
			Sleep(300);	// Sleep for 300ms to allow processing

			// Now simulate the operation
			REFERENCE_TIME clockStep = (1e7/ outputFrameRate);
			REFERENCE_TIME clock = 1200 + clockStep;	// Some offset

			for (size_t iteration = 0; iteration < 120; iteration++,clock += clockStep)
			{
				pClock->SetTime(clock);

				ComPtr<IAudioFrame> spFrame;
				HRESULT hr = pAnalyzer->GetFrame(&spFrame);
				Assert::IsTrue(hr == S_OK, L"AsByteAccess", LINE_INFO());
				Assert::IsNotNull(spFrame.Get(), L"Frame null", LINE_INFO());
				CAudioBufferHelper buffer(spFrame.Get());

				// Find peaks
				std::vector<size_t> peak_index(channels);
				std::vector<float> peak_value(channels);
				float *pBufferData = buffer.GetBuffer();
				size_t bufferSize = buffer.GetLength();

				Assert::AreEqual(sizeof(float) * (outputSize * channels + 8), bufferSize, L"BufferSize", LINE_INFO());
				struct Peak
				{
				public:
					size_t index;
					float frequency;
					float value;
					float prominence;
				};

				for (size_t channelIndex = 0; channelIndex < channels; channelIndex++)
				{
					std::list<Peak> peaks;
					float *pData = pBufferData + channelIndex * 1000;
					float *pRMS = pBufferData + channels * 1000 + channelIndex;
					// Expect 0 channel at 1.5 -> 20*log10(1.5) = 3.52, channel 1 would be 0.25 times less -> 3.52-12
					Assert::AreEqual(channelIndex ? -8.52f : 3.52f, *pRMS, 1.0f, L"RMS values off", LINE_INFO());
					float trough_value = pData[0];
					size_t trough_index = 0;
					float fStep = powf(1000.0f, 1 / 1000.0f);
					float f = 20.0f * fStep * fStep;
					for (size_t index = 2; index < outputSize; index++,f*=fStep)
					{
						float delta1 = pData[index - 1] - pData[index - 2];
						float delta2 = pData[index] - pData[index - 1];
						if (delta1 >= 0 && delta2 < 0)
						{
							float prominence = pData[index - 1] - trough_value;
							if (prominence > 10 && pData[index - 1] > -50)	// Remove noise and only count peaks over 10db and vol > -50db
							{
								// Peak detected
								Peak p;
								p.index = index - 1;
								p.value = pData[index - 1];
								p.prominence = p.value - trough_value;
								p.frequency = f;

								peaks.push_back(p);
							}
						}
						// Trough found
						if (delta1 <= 0 && delta2 > 0)
						{
							trough_index = index;
							trough_value = pData[index - 1];
						}
					}
					if (peaks.size() == 2)	// Skip first iterations
					{
						Assert::AreEqual(peaks.size(), 2u, L"Expecting 2 peaks", LINE_INFO());
						Peak p1 = peaks.front(),p2 = peaks.back();
						// The higher tone should be about 6db (0.5 times) stronger than lower one
						Assert::AreEqual(6.0f, p2.value - p1.value, 1.0f, L"Expect 6db off", LINE_INFO());

						wchar_t szMessage[1024];
						swprintf_s(szMessage, L"Peak check iteration %d, channel %d, p1=(%fHz %fdb) p2=(%fHz %fdb)\n", iteration, channelIndex, p1.frequency, p1.value, p2.frequency, p2.value);
						OutputDebugString(szMessage);
						float expectedValue = db[channelIndex] - 14.0f;
						// 1.0db tolerance for volume
						Assert::AreEqual(expectedValue - 6.0f, p1.value, 1.0f, szMessage, LINE_INFO());
						Assert::AreEqual(expectedValue, p2.value, 1.0f, szMessage, LINE_INFO());

						// 10% tolerance for frequency
						Assert::AreEqual(f1[channelIndex],p1.frequency,p1.frequency*0.1f,szMessage,LINE_INFO());
						Assert::AreEqual(f2[channelIndex], p2.frequency, p2.frequency*0.1f, szMessage, LINE_INFO());

					}

				}

			}
		}


		void Run_MFT_Test(unsigned sampleRate, unsigned channels, float outputFrameRate)
		{
			using namespace ABI::Windows::Media;
			using namespace ABI::Windows::Foundation;

			const size_t fftLength = 2048;

			REFERENCE_TIME expectedFrameDuration = (REFERENCE_TIME)(1e7 / outputFrameRate);

			ComPtr<ABI::SampleGrabber::IMyInterface> spAnalyzerOut;
			ComPtr<IMFTransform> spTransform;

			Create_SampleGrabber_MFT(&spTransform, &spAnalyzerOut);

			GetFrame_Before_Configure(spAnalyzerOut.Get());

			Configure_MediaType_MFT(spTransform.Get(), sampleRate, channels);

			ComPtr<CFakeClock> spFakeClock;
			Create_And_Assign_Clock(&spFakeClock, spTransform.Get());

			HRESULT hr = spAnalyzerOut->Configure(outputFrameRate, 0.5f, fftLength);
			hr = spAnalyzerOut->SetLinearFScale();

			CGenerator g(sampleRate, channels);

			// Now imitate the initial feed of input by sending 3 frames with total length of 1s
			for (size_t i = 0; i < 3; i++)
			{
				ComPtr<IMFSample> spInputSample;
				g.GetSample(&spInputSample, 16000,
					[](unsigned long sampleIndex, unsigned channel)
				{
					return 0.0f;	// Test silence
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
				(expectedFrameDuration * 7) >> 1 // Set in the middle of 3rd frame (at 3,5 * duration)
			};
			REFERENCE_TIME expectedTimes[] = { 0,0,expectedFrameDuration,500000 };

			Test_GetFrame_Timings(spAnalyzerOut.Get(),
				spFakeClock.Get(),
				sizeof(setPresentationTimes) / sizeof(REFERENCE_TIME),
				setPresentationTimes,
				expectedTimes,
				expectedFrameDuration);

			size_t expectedOutput = sampleRate / outputFrameRate;
			Test_Zero_Levels(spAnalyzerOut.Get(), fftLength, channels);

			Test_GetFrame_Continuity(spAnalyzerOut.Get(), spFakeClock.Get(), setPresentationTimes[3], expectedFrameDuration);

			// Drain the analyzer
			spFakeClock->SetTime(0);
			ComPtr<IAudioFrame> spFrame;
			spAnalyzerOut->GetFrame(&spFrame);
			Assert::IsNull(spFrame.Get(), L"Frame not null");

			Test_LogFOutput(spTransform.Get(), spAnalyzerOut.Get(),spFakeClock.Get(), outputFrameRate, sampleRate, channels);

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
			Run_MFT_Test(48000, 2, 60.0f);
		}

		BEGIN_TEST_METHOD_ATTRIBUTE(SampleGrabber_44100_Stereo)
			TEST_METHOD_ATTRIBUTE(L"Category", L"Analyzer")
			END_TEST_METHOD_ATTRIBUTE()
			TEST_METHOD(SampleGrabber_44100_Stereo)
		{
			Run_MFT_Test(44100, 2, 60.0f);
		}

		/*
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
		*/

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
		BEGIN_TEST_METHOD_ATTRIBUTE(RingBuffer)
			TEST_METHOD_ATTRIBUTE(L"Category", L"Analyzer")
			END_TEST_METHOD_ATTRIBUTE()
			TEST_METHOD(RingBuffer)
		{
			buffers::ring_buffer<float, 8> buffer;
			for (size_t i = 0; i < 9; i++)
			{
				*(buffer.writer()++) = (float)i;
			}
			auto rdr1 = buffer.reader() - 2;
			Assert::AreEqual(6.0f, *rdr1);
			rdr1++; rdr1++;
			Assert::AreEqual(8.0f, *rdr1);
			auto rdr2 = buffer.reader() + 10;
			Assert::AreEqual(2.0f, *rdr2);
		}
	};
}