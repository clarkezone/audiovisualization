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

        void DrawSwapChain(CanvasSwapChain swapChain)
        {
            using (var ds = swapChain.CreateDrawingSession(Colors.Transparent))
            {
                var size = swapChain.Size.ToVector2();
                var radius = (Math.Min(size.X, size.Y) / 2.0f) - 4.0f;

                var center = size / 2;

                // yuck
                //lock (PassthroughEffect.GetBadLock())
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
                }
            }

            swapChain.Present();
        }

        private bool hasNextValue;
        private Tuple<double, double> nextvalue;
        private Queue<Tuple<double, double>> dataQueue;
        private long delayTotal = 5;
        private long delayCurrent = 0;
        private bool delayStart = false;
        private bool weAreVisualizing = false;

        void DrawVU(CanvasDrawingSession ds, double volumeLeft, double volumeRight)
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

            int[] vuValues = { -60, -57, -54, -51, -48, -45, -42, -39, -36, -33, -30, -27, -24, -21, -18, -15, -12, -9, -6, -3, 0 };
            Size segmentSize = new Size(segmentWidth, segmentHeight);
            Point positionLeft = new Point(channelGap * 4, channelGap * 2);
            Point positionRight = new Point(positionLeft.X + segmentWidth + channelGap, positionLeft.Y);
            Rect segmentLeft = new Rect(positionLeft, segmentSize);
            Rect segmentRight = new Rect(positionRight, segmentSize);

            int leftActiveIndex = -1;
            int rightActiveIndex = -1;

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
                Debug.WriteLine("Not found");
            }

            for (int i = vuValues.Length - 2; i > 1; i--)
            {
                if (volumeRight > vuValues[i - 1] && volumeRight < vuValues[i + 1])
                {
                    rightActiveIndex = i;
                    break;
                }
            }

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

                if (i <= leftActiveIndex)
                {
                    ds.FillRectangle(segmentLeft, litColor);
                }
                else
                {
                    ds.FillRectangle(segmentLeft, unLitColor);
                }
                positionLeft = new Point(positionLeft.X, positionLeft.Y + gap + segmentSize.Height);

                segmentRight = new Rect(positionRight, segmentSize);

                if (i <= rightActiveIndex)
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
