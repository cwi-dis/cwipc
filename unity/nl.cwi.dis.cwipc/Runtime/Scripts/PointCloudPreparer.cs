using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace cwipc
{
    using Timestamp = System.Int64;
    using Timedelta = System.Int64;

    public class PointCloudPreparer
    {
        bool isReady = false;
        Unity.Collections.NativeArray<byte> currentByteArray;
        System.IntPtr currentBuffer;
        int currentSize;
        public Timestamp currentTimestamp;
        float currentCellSize = 0.008f;
        float defaultCellSize;
        float cellSizeFactor;
        protected QueueThreadSafe InQueue;

        public PointCloudPreparer(QueueThreadSafe _InQueue, float _defaultCellSize = 0, float _cellSizeFactor = 0)
        {
            InQueue = _InQueue;
            defaultCellSize = _defaultCellSize != 0 ? _defaultCellSize : 0.01f;
            cellSizeFactor = _cellSizeFactor != 0 ? _cellSizeFactor : 1.0f;
            Start();
        }

        public virtual string Name()
        {
            return $"{GetType().Name}";
        }

        public void Start()
        {

        }

        public void Stop()
        {
            if (currentByteArray.Length != 0) currentByteArray.Dispose();
#if CWIPC_WITH_LOGGING
            Debug.Log("PointCloudPreparer Stopped");
#endif
        }

        public void Synchronize()
        {

        }

        public  bool LatchFrame()
        {
           lock (this)
            {
                int dropCount = 0;
                Timestamp bestTimestamp = 0;
               
                // xxxjack Note: we are holding the lock during TryDequeue. Is this a good idea?
                // xxxjack Also: the 0 timeout to TryDecode may need thought.
                if (InQueue.IsClosed()) return false; // We are shutting down
                cwipc.pointcloud pc = (cwipc.pointcloud)InQueue.TryDequeue(0);
                if (pc == null)
                {
                    return false;
                }
                // See if there are more pointclouds in the queue that are no later than bestTimestamp
                while (pc.timestamp() < bestTimestamp)
                {
                    Timestamp nextTimestamp = InQueue._PeekTimestamp();
                    // If there is no next queue entry, or it has no timestamp, or it is after bestTimestamp we break out of the loop
                    if (nextTimestamp == 0 || nextTimestamp > bestTimestamp) break;
                    // We know there is another pointcloud in the queue, and we know it is better than what we have now. Replace it.
                    dropCount++;
                    pc.free();
                    pc = (cwipc.pointcloud)InQueue.Dequeue();
                }
                unsafe
                {
                    currentSize = pc.get_uncompressed_size();
                    currentTimestamp = pc.timestamp();
                    currentCellSize = pc.cellsize();
                    // xxxjack if currentCellsize is != 0 it is the size at which the points should be displayed
                    if (currentSize > currentByteArray.Length)
                    {
                        if (currentByteArray.Length != 0) currentByteArray.Dispose();
                        currentByteArray = new Unity.Collections.NativeArray<byte>(currentSize, Unity.Collections.Allocator.Persistent);
                        currentBuffer = (System.IntPtr)Unity.Collections.LowLevel.Unsafe.NativeArrayUnsafeUtility.GetUnsafePtr(currentByteArray);
                    }
                    if (currentSize > 0)
                    {
                        int ret = pc.copy_uncompressed(currentBuffer, currentSize);
                        if (ret * 16 != currentSize)
                        {
                            Debug.Log($"PointCloudPreparer decompress size problem: currentSize={currentSize}, copySize={ret * 16}, #points={ret}");
                            Debug.LogError($"{Name()}: Pointcloud size error");
                        }
                    }
                    pc.free();
                    isReady = true;
                }
            }
            return true;
        }

        public int GetComputeBuffer(ref ComputeBuffer computeBuffer)
        {
            // xxxjack I don't understand this computation of size, the sizeof(float)*4 below and the byteArray.Length below that.
            int size = currentSize / 16; // Because every Point is a 16bytes sized, so I need to divide the buffer size by 16 to know how many points are.
            lock (this)
            {
                if (isReady && size != 0)
                {
                    unsafe
                    {
                        if (computeBuffer == null || computeBuffer.count < size)
                        {
                            int dampedSize = size + 4 + size / 4; // We allocate 25% (and a bit) more, so we don't see too many reallocations

                            if (computeBuffer != null) computeBuffer.Release();
                            computeBuffer = new ComputeBuffer(dampedSize, sizeof(float) * 4);
                        }
                        computeBuffer.SetData(currentByteArray, 0, 0, currentByteArray.Length);
                    }
                    isReady = false;
                }
            }
            return size;
        }

        public float GetPointSize()
        {
            if (currentCellSize > 0.0000f) return currentCellSize * cellSizeFactor;
            else return defaultCellSize * cellSizeFactor;
        }

        public Timedelta getQueueDuration()
        {
            if (InQueue == null) return 0;
            return InQueue.QueuedDuration();
        }
    }
}
