using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using AudioVisualization.AutoProcessing;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Media;
using Windows.Media.Effects;
using Windows.Media.MediaProperties;
using Windows.UI.Composition;
using System.Collections.Concurrent;

namespace SampleGrabberCS.Reference
{
    public sealed class PassthroughEffect : IBasicAudioEffect
    {
        // This is bad and I should feel bad. Remove after testing is done.
        private static object badLock = new object();

        public static object GetBadLock() { return badLock; }

        private AudioEncodingProperties currentEncodingProperties;
        private List<AudioEncodingProperties> supportedEncodingProperties;

        private IPropertySet propertySet;
        private CompositionPropertySet compositionPropertySet;

        public bool UseInputFrameForOutput { get { return true; } }
        public bool TimeIndependent { get { return false; } }
        public bool IsReadyOnly { get { return true; } }

        Stopwatch sw = new Stopwatch();

        // Set up constant members in the constructor
        public PassthroughEffect()
        {
            Debug.WriteLine("constructor");
            // tried changing these to stereo, likely need to dig in more to the media type stuff.

            // Support 44.1kHz and 48kHz stereo float
            supportedEncodingProperties = new List<AudioEncodingProperties>();
            AudioEncodingProperties encodingProps1 = AudioEncodingProperties.CreatePcm(44100, 2, 32);
            encodingProps1.Subtype = MediaEncodingSubtypes.Float;
            AudioEncodingProperties encodingProps2 = AudioEncodingProperties.CreatePcm(48000, 2, 32);
            encodingProps2.Subtype = MediaEncodingSubtypes.Float;

            supportedEncodingProperties.Add(encodingProps1);
            supportedEncodingProperties.Add(encodingProps2);
            sw.Start();
        }

        public IReadOnlyList<AudioEncodingProperties> SupportedEncodingProperties
        {
            get
            {
                return supportedEncodingProperties;
            }
        }

        public double VolumeInDecibels { get; private set; }

        public void SetEncodingProperties(AudioEncodingProperties encodingProperties)
        {
            currentEncodingProperties = encodingProperties;
        }

        unsafe public void ProcessFrame(ProcessAudioFrameContext context)
        {
            //foreach (var item in context.InputFrame.ExtendedProperties.Keys)
            //{
            //    Debug.WriteLine(item);
            //}


            const int videoFrameRate = 60; // TODO: we should probably measure this

            //Debug.WriteLine(sw.ElapsedMilliseconds.ToString());
            AudioFrame inputFrame = context.InputFrame;

            using (AudioBuffer inputBuffer = inputFrame.LockBuffer(AudioBufferAccessMode.Read))
            using (IMemoryBufferReference inputReference = inputBuffer.CreateReference())
            {
                byte* inputInBytes;
                uint inputCapacity;
                float* inputInFloats;

                ((IMemoryBufferByteAccess)inputReference).GetBuffer(out inputInBytes, out inputCapacity);

                inputInFloats = (float*)inputInBytes;
                int inputLengthSamples = (int)inputBuffer.Length / sizeof(float);

                int samplesPervBlank = (int)((float)currentEncodingProperties.SampleRate / (float)videoFrameRate);

                int numVBlanksForCurrentAudioBuffer = (int)Math.Ceiling(((float)context.InputFrame.Duration.Value.Milliseconds / ((1.0f / (float)videoFrameRate) * 1000)));

                var volumeSetLeft = new double[numVBlanksForCurrentAudioBuffer];
                var volumeSetRight = new double[numVBlanksForCurrentAudioBuffer];

                //Left Channel
                CalcAudioVolumedBPerVBlank(inputInFloats, inputLengthSamples, samplesPervBlank, volumeSetLeft, 0, (int)currentEncodingProperties.ChannelCount);

                if (currentEncodingProperties.ChannelCount == 2)
                {
                    //Right Channel
                    CalcAudioVolumedBPerVBlank(inputInFloats, inputLengthSamples, samplesPervBlank, volumeSetRight, 1, (int)currentEncodingProperties.ChannelCount);
                }

                lock (PassthroughEffect.GetBadLock())
                {
                    //((Queue<Double[]>)this.propertySet["AudioVolumeLeftQueue"]).Enqueue(volumeSetLeft);
                    //((Queue<Double[]>)this.propertySet["AudioVolumeRightQueue"]).Enqueue(volumeSetRight);
                    this.propertySet["VolumeLeft"] = volumeSetLeft;
                    this.propertySet["VolumeRight"] = volumeSetRight;
                }
            }
        }

        private static unsafe void CalcAudioVolumedBPerVBlank(float* inputInFloats, int inputLengthSamples,
            int samplesPervBlank, double[] volumeSet, int startOffset, int numChannels)
        {
            int vblankSampleIndex = 0;
            float sum = 0;
            var volumeIndex = 0;
            int extra = 0;

            for (int i = startOffset; i < inputLengthSamples; i += 2)
            {
                sum += (inputInFloats[i] * inputInFloats[i]);
                vblankSampleIndex++;
                if (vblankSampleIndex == samplesPervBlank)
                {
                    double rms = Math.Sqrt(sum / (samplesPervBlank));
                    volumeIndex = (i / samplesPervBlank / numChannels);
                    if (volumeIndex < volumeSet.Length)
                    {
                        volumeSet[volumeIndex] = 20 * Math.Log10(rms);
                    }
                    else
                    {
                        extra++;
                    }

                    vblankSampleIndex = 0;
                    sum = 0;
                }
            }
            if (extra > 0)
            {
                throw new Exception("Bug");
            }
        }

        public void Close(MediaEffectClosedReason reason)
        {
            // Clean-up any effect resources
            // This effect doesn't care about close, so there's nothing to do
        }

        public void DiscardQueuedFrames()
        {

        }

        public void SetProperties(IPropertySet configuration)
        {
            this.propertySet = configuration;

            if (propertySet.ContainsKey("compositionPropertySet"))
            {
                compositionPropertySet = (CompositionPropertySet)propertySet["compositionPropertySet"];
            }

            //propertySet.Add("AudioVolumeLeftQueue", new Queue<Double[]>());
            //propertySet.Add("AudioVolumeRightQueue", new Queue<Double[]>());
        }
    }
}
