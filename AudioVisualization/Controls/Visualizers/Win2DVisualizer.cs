using System;
using System.Threading.Tasks;
using Windows.UI.Xaml;
using Microsoft.Graphics.Canvas;
using System.Numerics;
using Microsoft.Graphics.Canvas.UI.Composition;
using Windows.UI.Composition;
using Windows.UI.Core;
using System.Threading;
using Windows.Foundation;
using Windows.UI;
using AudioVisualization.Services;
using System.Diagnostics;
using System.Collections.Generic;
using Microsoft.Graphics.Canvas.Text;
using AudioVisualization.Extensions;

namespace AudioVisualization.Controls.Visualizers
{
    internal class Win2DVisualizer : BaseVisualizer, IDisposable
    {
        CanvasDevice _device;
        CompositionGraphicsDevice _compositionGraphicsDevice;

        CanvasSwapChain _swapChain;
        SpriteVisual _swapChainVisual;
        CancellationTokenSource _drawLoopCancellationTokenSource;


        public Win2DVisualizer()
        {
            // Assume here stereo audio
            m_VolumeData = new float[2] { -100.0f, -100.0f };
            m_PeakVolumeData = new float[2] { -100.0f, -100.0f };

            // Assume stereo with 800 data points, 10 times less for peak data
            m_SpectralData = new float[][] { new float[800], new float[800] };
            m_SpectralPeakData = new float[][] { new float[80], new float[800] };

            CreateDevice();
            _swapChainVisual = _compositor.CreateSpriteVisual();
            _rootVisual.Children.InsertAtTop(_swapChainVisual);
        }

        protected override void OnSizeChanged(object sender, SizeChangedEventArgs e)
        {
            if (e != null && e.NewSize.Width > 0 && e.NewSize.Height > 0)
            {
                SetDevice(_device, e.NewSize);
                _swapChainVisual.Size = new Vector2((float)e.NewSize.Width, (float)e.NewSize.Height);
            }
        }

        internal override void OnUnloaded(object sender, RoutedEventArgs e)
        {
            rightCurrent = null;
            leftCurrent = null;
            base.OnUnloaded(sender, e);
            this.Dispose();
        }

        void SetDevice(CanvasDevice device, Size windowSize)
        {
            _drawLoopCancellationTokenSource?.Cancel();

            _swapChain = new CanvasSwapChain(device, (float)this.ActualWidth, (float)this.ActualHeight, 96);
            _swapChainVisual.Brush = _compositor.CreateSurfaceBrush(CanvasComposition.CreateCompositionSurfaceForSwapChain(_compositor, _swapChain));

            _drawLoopCancellationTokenSource = new CancellationTokenSource();
            Task.Factory.StartNew(
                DrawLoop,
                _drawLoopCancellationTokenSource.Token,
                TaskCreationOptions.LongRunning,
                TaskScheduler.Default);
        }


        void CreateDevice()
        {
            _device = CanvasDevice.GetSharedDevice();
            _device.DeviceLost += Device_DeviceLost;

            if (_compositionGraphicsDevice == null)
            {
                _compositionGraphicsDevice = CanvasComposition.CreateCompositionGraphicsDevice(_compositor, _device);
            }
            else
            {
                CanvasComposition.SetCanvasDevice(_compositionGraphicsDevice, _device);
            }
        }

        void Device_DeviceLost(CanvasDevice sender, object args)
        {
            _device.DeviceLost -= Device_DeviceLost;

            var unwaitedTask = Window.Current.Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () => CreateDevice());
        }

        public void Dispose()
        {
            _drawLoopCancellationTokenSource?.Cancel();
            _swapChain?.Dispose();
        }

        Stopwatch sw = new Stopwatch();

        void DrawLoop()
        {
            var canceled = _drawLoopCancellationTokenSource.Token;

            try
            {

                //sw.Start();
                while (!canceled.IsCancellationRequested)
                {
                    DrawSwapChain(_swapChain);
                    _swapChain.WaitForVerticalBlank();
                }
                //sw.Stop();

                _swapChain.Dispose();
            }
            catch (Exception e) when (_swapChain.Device.IsDeviceLost(e.HResult))
            {
                _swapChain.Device.RaiseDeviceLost();
            }
        }

        Queue<Double[]> LeftVolumeQueue = new Queue<double[]>();
        Queue<Double[]> RightVolumeQueue = new Queue<double[]>();
        double[] leftCurrent;
        double[] rightCurrent;

        float[][] m_SpectralData;
        float[][] m_SpectralPeakData;
        float[] m_VolumeData;
        float[] m_PeakVolumeData;
        const float meterRiseTime = 0.9f;           // Every 1/60 frame rise 90% of the diff value
        const float meterFallTime = 0.3f;
        const float peakMeterRiseTime = 1.0f;       // Fast rise
        const float peakMeterFallTime = 0.003f;     // Slow fall time

        void DrawSwapChain(CanvasSwapChain swapChain)
        {
            using (var ds = swapChain.CreateDrawingSession(Colors.Transparent))
            {
                var size = swapChain.Size.ToVector2();
                var radius = (Math.Min(size.X, size.Y) / 2.0f) - 4.0f;

                var center = size / 2;

                if (PlayerService.Current.ReferencePropertySet?.ContainsKey("samplegrabber") == true)
                {
                    SampleGrabber.IMyInterface mft = (SampleGrabber.IMyInterface)PlayerService.Current.ReferencePropertySet["samplegrabber"];

                    var dataFrame = mft.GetFrame();
                    if (dataFrame != null)
                    {
                        using (var data = dataFrame.AsVisualizationData())
                        {
                            DrawVU(ds, data.GetRMS(0), data.GetRMS(1));
                            DrawSpectrogram(data, ds);
                        }
                        dataFrame.Dispose();
                    }
                    else
                    {                        
                        DrawVU(ds, -100.0f, -100.0f);
                        DrawSpectrogram(null, ds);
                    }
                    // yuck
                    //lock (PassthroughEffect.GetBadLock())
                    /*
                    {
                        if (PlayerService.Current.ReferencePropertySet != null &&
                            PlayerService.Current.ReferencePropertySet.ContainsKey("dataQueue") &&
                            this.dataQueue == null)
                        {
                            this.dataQueue = PlayerService.Current.ReferencePropertySet["dataQueue"] as Queue<Tuple<double, double>>;
                        }

                        if (this.dataQueue != null && !hasNextValue && this.dataQueue.Count>0)
                        {
                            nextvalue = this.dataQueue.Dequeue();
                            hasNextValue = true;
                        } else if (this.dataQueue != null && this.dataQueue.Count == 0)
                        {
                            hasNextValue = false;

                        }
                    }

                    if (dataQueue != null)
                    {
                        Debug.WriteLine(dataQueue.Count);
                    }

                    if (!weAreVisualizing && hasNextValue)
                    {
                        weAreVisualizing = true;
                        delayStart = true;
                    }

                    if (weAreVisualizing && delayStart && delayCurrent < delayTotal)
                    {
                        delayCurrent++;
                    }
                    else
                    {
                        delayStart = false;
                        delayCurrent = 0;
                    }

                    if (weAreVisualizing && delayStart)
                    {
                        DrawVU(ds, -100, -100);
                    } else if (weAreVisualizing && !delayStart && nextvalue != null)
                    {
                        DrawVU(ds, nextvalue.Item1, nextvalue.Item2);
                        hasNextValue = false;
                    } else
                    {
                        Debug.WriteLine("miss");
                        DrawVU(ds, -100, -100);
                    }*/
                }

                swapChain.Present();
            }
        }

        private void DrawSpectrogram(VisualizationData data, CanvasDrawingSession ds)
        {
            // Draw grid

            ds.DrawRectangle(200, 25, 800, 200, Colors.Gray);
            ds.DrawRectangle(200, 250, 800, 200, Colors.Gray);

            for (int x = 0; x < 800; x+=16)
            {
                ds.DrawLine(x+200, 25, x+200, 225, Colors.LightGray);
                ds.DrawLine(x+200, 250, x+200, 450, Colors.LightGray);
            }

            for (int y=0;y<200;y+=20)
            {
                ds.DrawLine(200, y + 25, 1000, y + 25, Colors.LightGray);
                ds.DrawLine(200, y + 250, 1000, y + 250, Colors.LightGray);
            }

            if (data == null)
                return;

            if (data.Length != 112)
                return;

            // Draw 50 bars of width 16 pixels
            for (uint i = 0; i < 50; i++)
            {
                float xPos = i * 16 + 200;
                float leftHeight = 2*data[data.GetChannelOffset(0) + i]+200;
                ds.FillRectangle(xPos, 225 - leftHeight, 16, leftHeight, Colors.Orange);

                float rightHeight = 2*data[data.GetChannelOffset(1) + i] + 200;
                ds.FillRectangle(xPos, 250, 16, rightHeight, Colors.LimeGreen);
            }
        }

        private bool hasNextValue;
        private Tuple<double, double> nextvalue;
        private Queue<Tuple<double, double>> dataQueue;
        private long delayTotal = 5;
        private long delayCurrent = 0;
        private bool delayStart = false;
        private bool weAreVisualizing = false;

        void DrawVU(CanvasDrawingSession ds, float volumeLeft, float volumeRight)
        {
            //TODO: move consts out of here

            Color GreenLit = Color.FromArgb(255, 0, 255, 0);
            Color GreenDim = Color.FromArgb(255, 0, 153, 0);
            Color YellowLit = Color.FromArgb(255, 255, 255, 0);
            Color YellowDim = Color.FromArgb(255, 153, 153, 0);
            Color RedLit = Color.FromArgb(255, 255, 0, 0);
            Color RedDim = Color.FromArgb(255, 153, 0, 0);

            const float gap = 3.0f;
            const float channelGap = 10.0f;
            const float segmentHeight = 15.0f;
            const float segmentWidth = 50.0f;

            float[] vuValues = { -60, -57, -54, -51, -48, -45, -42, -39, -36, -33, -30, -27, -24, -21, -18, -15, -12, -9, -6, -3, 0 };
            Size segmentSize = new Size(segmentWidth, segmentHeight);
            Point positionLeft = new Point(channelGap * 4, channelGap * 2);
            Point positionRight = new Point(positionLeft.X + segmentWidth + channelGap, positionLeft.Y);
            Rect segmentLeft = new Rect(positionLeft, segmentSize);
            Rect segmentRight = new Rect(positionRight, segmentSize);

            // Calculate real VU meter values with rise and fall times
            m_VolumeData[0] -= (m_VolumeData[0] - volumeLeft) * (m_VolumeData[0] < volumeLeft ? meterRiseTime : meterFallTime);
            m_VolumeData[1] -= (m_VolumeData[1] - volumeRight) * (m_VolumeData[1] < volumeRight ? meterRiseTime : meterFallTime);

            m_PeakVolumeData[0] -= (m_PeakVolumeData[0] - volumeLeft) * (m_PeakVolumeData[0] < volumeLeft ? peakMeterRiseTime : peakMeterFallTime);
            m_PeakVolumeData[1] -= (m_PeakVolumeData[1] - volumeRight) * (m_PeakVolumeData[1] < volumeRight ? peakMeterRiseTime : peakMeterFallTime);

            // Match meter values to meter indexes
            int foundIndex = Array.BinarySearch<float>(vuValues, m_VolumeData[0]);
            int leftActiveIndex = foundIndex != -1 ? (foundIndex < 0 ? ~foundIndex : foundIndex) : -1;
            foundIndex = Array.BinarySearch<float>(vuValues, m_VolumeData[1]);
            int rightActiveIndex = foundIndex != -1 ? (foundIndex < 0 ? ~foundIndex : foundIndex) : -1;

            foundIndex = Array.BinarySearch<float>(vuValues, m_PeakVolumeData[0]);
            int leftPeakIndex = foundIndex != -1 ? (foundIndex < 0 ? ~foundIndex : foundIndex) : -1;
            foundIndex = Array.BinarySearch<float>(vuValues, m_PeakVolumeData[1]);
            int rightPeakIndex = foundIndex != -1 ? (foundIndex < 0 ? ~foundIndex : foundIndex) : -1;

            /* TODO - Remove replaced with Array.BinarySearch above
            bool found = false;
            for (int i = vuValues.Length - 2; i > 1; i--)
            {
                if (volumeLeft > vuValues[i - 1] && volumeLeft < vuValues[i + 1])
                {
                    leftActiveIndex = i;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                // Debug.WriteLine("Not found");
            }

            for (int i = vuValues.Length - 2; i > 1; i--)
            {
                if (volumeRight > vuValues[i - 1] && volumeRight < vuValues[i + 1])
                {
                    rightActiveIndex = i;
                    break;
                }
            }*/

            Color litColor;
            Color unLitColor;

            CanvasTextFormat formatleft = new CanvasTextFormat();
            formatleft.FontSize = 9;
            formatleft.HorizontalAlignment = CanvasHorizontalAlignment.Right;

            CanvasTextFormat formatright = new CanvasTextFormat();
            formatright.FontSize = 9;
            formatright.HorizontalAlignment = CanvasHorizontalAlignment.Left;

            for (int i = vuValues.Length - 1; i >= 0; i--)
            {
                CanvasTextLayout text = new CanvasTextLayout(CanvasDevice.GetSharedDevice(), vuValues[i] + " dB", formatleft, 30, 50);
                ds.DrawTextLayout(text, new Vector2((float)positionLeft.X - 34.0f, (float)positionLeft.Y), Color.FromArgb(255, 0, 0, 0));

                CanvasTextLayout text2 = new CanvasTextLayout(CanvasDevice.GetSharedDevice(), vuValues[i] + " dB", formatright, 30, 50);
                ds.DrawTextLayout(text2, new Vector2((float)positionRight.X + (float)segmentSize.Width + 4.0f, (float)positionLeft.Y), Color.FromArgb(255, 0, 0, 0));
                if (i >= vuValues.Length - 3)
                {
                    litColor = RedLit;
                    unLitColor = RedDim;
                }
                else if (i >= vuValues.Length - 6 && i < vuValues.Length - 3)
                {
                    litColor = YellowLit;
                    unLitColor = YellowDim;
                }
                else
                {
                    litColor = GreenLit;
                    unLitColor = GreenDim;
                }

                segmentLeft = new Rect(positionLeft, segmentSize);

                if (i <= leftActiveIndex || i == leftPeakIndex)
                {
                    ds.FillRectangle(segmentLeft, litColor);
                }
                else
                {
                    ds.FillRectangle(segmentLeft, unLitColor);
                }
                positionLeft = new Point(positionLeft.X, positionLeft.Y + gap + segmentSize.Height);

                segmentRight = new Rect(positionRight, segmentSize);

                if (i <= rightActiveIndex || i == rightPeakIndex)
                {
                    ds.FillRectangle(segmentRight, litColor);
                }
                else
                {
                    ds.FillRectangle(segmentRight, unLitColor);
                }

                positionRight = new Point(positionRight.X, positionRight.Y + gap + segmentSize.Height);
            }


        }
    }
}
