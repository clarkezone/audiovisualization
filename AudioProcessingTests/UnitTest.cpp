#include "pch.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Microsoft::WRL;

namespace AudioProcessingTests
{
	TEST_CLASS(UnitTest1)
	{
	public:
		TEST_METHOD(VerifyPushPop)
		{
			std::queue<float> test;
			test.push(10.0f);
			test.push(11.0f);
			test.push(12.0f);
			auto one = test.front();
			test.pop();
			auto two = test.front();
			test.pop();
			auto three = test.front();
		}

		TEST_METHOD(ActivateFilter)
		{
			ComPtr<IInspectable> object;

			HRESULT hr = ABI::Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);
			__abi_ThrowIfFailed(hr);

			auto refr = Microsoft::WRL::Wrappers::HStringReference(L"SampleGrabber.SampleGrabberTransform");

			ComPtr<IInspectable> uriFactory;
			hr = ABI::Windows::Foundation::GetActivationFactory(refr.Get(), &uriFactory);
			__abi_ThrowIfFailed(hr);
		}

		TEST_METHOD(VerifyRMS)
		{
			ComPtr<IMFSample> sample;
			ComPtr<IUnknown> object;

			auto url = L"assets//LevelTest_mixdown.mp3";

			auto hr = MFStartup(MF_VERSION);
			auto mfStarted = SUCCEEDED(hr);

			ComPtr<IMFSourceReader> reader;
			if (SUCCEEDED(hr))
			{
				hr = MFCreateSourceReaderFromURL(url, nullptr, &reader);
			}

			ComPtr<IMFMediaType> nativeType;

			hr = reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &nativeType);

			UINT32 sampleRate = 0, channelCount = 0;
			hr = nativeType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sampleRate);
			if (FAILED(hr))
				throw ref new Platform::COMException(hr);

			hr = nativeType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channelCount);

			// Select the first audio stream, and deselect all other streams.
			if (SUCCEEDED(hr))
			{
				hr = reader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE);
			}

			if (SUCCEEDED(hr))
			{
				hr = reader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), TRUE);
			}

			ComPtr<IMFMediaType> currentType;
			hr = reader->GetCurrentMediaType(0, &currentType);

			// Create a partial media type that specifies uncompressed PCM audio.
			ComPtr<IMFMediaType> partialType;

			if (SUCCEEDED(hr)) {
				hr = ConvertAudioTypeToFloat32(currentType.Get(), &partialType);
			}

			// Set this type on the source reader. The source reader will load the necessary decoder.
			if (SUCCEEDED(hr))
			{
				hr = reader->SetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), nullptr, partialType.Get());
			}


			// Get the complete uncompressed format
			ComPtr<IMFMediaType> uncompressedAudioType;
			if (SUCCEEDED(hr))
			{
				hr = reader->GetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), &uncompressedAudioType);
			}

			// Ensure the stream is selected.
			if (SUCCEEDED(hr))
			{
				hr = reader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), TRUE);
			}

			AudioProcessor p;
			std::queue<float> buffer;
			std::queue<double> rmsList;

			while (SUCCEEDED(hr))
			{
				DWORD dwFlags = 0;

				// Read the next sample.
				ComPtr<IMFSample> sample;
				hr = reader->ReadSample(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), 0, nullptr, &dwFlags, nullptr, &sample);

				if (SUCCEEDED(hr) && (dwFlags & MF_SOURCE_READERF_ENDOFSTREAM) != 0)
				{
					// End of stream
					break;
				}

				if (SUCCEEDED(hr) && sample == nullptr)
				{
					// No sample, keep going
					continue;
				}

				DirectX::XMVECTOR* pBuffer;

				if (SUCCEEDED(hr)) {
					//TODO: test perf

					double sampleDuration = 0;

					hr = p.BuildFloatBuffer(sample, channelCount, sampleRate, &buffer, &sampleDuration);

					auto length = buffer.size();

					Assert::AreEqual(14448, (int)length);

					/*Duration 3276190 units of 100 nanoseconds
					327619000 nanoseconds
					1 millisecond = 1000000 nanoseconds
					=> Duration == 327.61900 miliseconds

					327.61900ms worth of audio samples => 327.61900 / (1000/60) = 19.65714 vblanks worth of samples

					14,448 samples / 19.65714 vblanks = 735.0001 samples per vbank
					*/

					int samplesPerVBlank = buffer.size() / (sampleDuration / (1000.0f / 60.0f));

					p.CalcRMS(&buffer, samplesPerVBlank, &rmsList);

					auto rmsLength = rmsList.size();

					Assert::AreEqual(19, (int)rmsLength);

					//TODO: 1 Check for accuracy: single sample only
					//TODO: 2 do it for stereo
					//TODO: 3 Get present time
					//TODO: 4 test with visualization -> communication pipe
					//TODO: Optimize perf
				}
			}

			//TODO: 1c verify final number of RMS values per filelength
			//TODO: 1d table based verification here for all the RMS
		}
	};
}