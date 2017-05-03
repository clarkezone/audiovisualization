using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Windows.Foundation.Diagnostics;
using Windows.Media;

namespace AudioVisualization.Extensions
{
    // Using the COM interface IMemoryBufferByteAccess allows us to access the underlying byte array in an AudioFrame
    [ComImport]
    [Guid("5B0D3235-4DBA-4D44-865E-8F1D0E4FD04D")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    internal unsafe interface IMemoryBufferByteAccess
    {
        void GetBuffer(out byte* buffer, out uint capacity);
    }

    static internal class VisualizationExtensions
    {
        public static VisualizationData AsVisualizationData(this Windows.Media.AudioFrame frame)
        {
            return new VisualizationData(frame);
        }
    }

    internal sealed unsafe class VisualizationData : IDisposable
    {
        public void Dispose()
        {
            if (m_Buffer != null)
            {
                m_Buffer.Dispose();
                m_Buffer = null;
            }
            if (m_BufferReference != null)
            {
                m_BufferReference.Dispose();
                m_BufferReference = null;
            }
            GC.SuppressFinalize(this);
        }

        AudioBuffer m_Buffer;
        Windows.Foundation.IMemoryBufferReference m_BufferReference;
        unsafe float* m_pData;
        uint m_DataCapacity;
        uint m_DataStep;

        internal unsafe VisualizationData(AudioFrame frame)
        {
            m_Buffer = frame.LockBuffer(AudioBufferAccessMode.Read);
            m_BufferReference = m_Buffer.CreateReference();
            byte* pData;
            uint capacity;

            ((IMemoryBufferByteAccess)m_BufferReference).GetBuffer(out pData, out capacity);
            m_pData = (float*)pData;
            m_DataCapacity = m_Buffer.Length / sizeof(float);

            m_DataStep = (uint) frame.ExtendedProperties["{3F692E37-FC20-48DD-93D2-2234E1B1AA23}"];
        }

        public uint GetChannelOffset(uint channel) { return m_DataStep * channel; }
        public uint GetRMSOffset(uint channels) { return m_DataStep * channels; }

        public uint Length { get { return m_DataCapacity; } }

        public float GetRMS(uint channel)
        {
            return this[GetRMSOffset(2) + channel];
        }

        unsafe public float this[uint index]
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
#if DEBUG
                if (index >= Length || index < 0)
                    throw new IndexOutOfRangeException();
#endif
                return m_pData[index];
            }
        }
    }
}


