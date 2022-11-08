using System;
using UnityEngine;

namespace cwipc
{
    using Timestamp = System.Int64;
    using Timedelta = System.Int64;

    abstract public class BasePointCloudReader : IPointCloudPreparer, IPointCloudReaderImplementation
    {
        protected cwipc.source reader = null;
        protected cwipc.pointcloud currentPointcloud = null;
        protected Unity.Collections.NativeArray<byte> currentByteArray;
        protected System.IntPtr currentBuffer;
        protected bool isReady;

        [Header("Introspection (for debugging)")]
        [Tooltip("Size of current pointcloud (in bytes)")]
        public int currentSize;
        [Tooltip("Timestamp of current pointcloud")]
        [SerializeField] protected Timestamp currentTimestamp;
        [Tooltip("Cell size of current pointcloud cell (in meters)")]
        [SerializeField] protected float currentCellSize = 0;
        [Tooltip("How many pointclouds have been read")]
        [SerializeField] protected int nRead;
        [Tooltip("How many pointclouds have been read and dropped")]
        [SerializeField] protected int nDropped;

        [Header("Fields valid for all reader implementations")]
        [Tooltip("Voxelize pointclouds to this size (if nonzero)")]
        public float voxelSize = 0;
        [Tooltip("Cellsize for pointclouds that don't specify a cellsize")]
        public float defaultCellSize = 0.01f;
        [Tooltip("Multiplication factor for cellsize")]
        public float cellSizeFactor = 1.0f;

        protected System.TimeSpan frameInterval;  // Interval between frame grabs, if maximum framerate specified
        protected System.DateTime earliestNextCapture;    // Earliest time we want to do the next capture, if non-null.
        const bool dontWait = false;
      
        public virtual string Name()
        {
            return $"{GetType().Name}";
        }

        public virtual void _AllocateReader()
        {
            throw new System.Exception($"{Name()}: _AllocateReader must be overridden");
        }

        public void OnDestroy()
        {
            currentPointcloud?.free();
            currentPointcloud = null;
            reader?.free();
            reader = null;
            if (currentByteArray.IsCreated)
            {
                currentByteArray.Dispose();
            }
        }

        public void Start()
        {
            if (reader == null)
            {
                _AllocateReader();
            }
            else
            {
                Debug.LogWarning("${Name()}: Start called twice");
            }
        }

        public void Stop()
        {
            reader?.free();
            reader = null;
#if CWIPC_WITH_LOGGING
            Debug.Log($"{Name()}: Stopped.");
#endif
        }

        protected void Update()
        {
            if (reader == null) return;
            //
            // Limit framerate, if required
            //
            if (earliestNextCapture != null)
            {
                System.TimeSpan sleepDuration = earliestNextCapture - System.DateTime.Now;
                if (sleepDuration > System.TimeSpan.FromSeconds(0))
                {
                    System.Threading.Thread.Sleep(sleepDuration);
                }
            }
            if (frameInterval != null)
            {
                earliestNextCapture = System.DateTime.Now + frameInterval;
            }
            if (dontWait) {
            	if (!reader.available(false)) return;
            }
            cwipc.pointcloud pc = reader.get();
            if (pc == null) return;
            Timedelta downsampleDuration = 0;
            if (voxelSize != 0)
            {
                System.DateTime downsampleStartTime = System.DateTime.Now;
                var newPc = cwipc.downsample(pc, voxelSize);
                if (newPc == null)
                {
                    Debug.LogWarning($"{Name()}: Voxelating pointcloud with {voxelSize} got rid of all points?");
                }
                else
                {
                    pc.free();
                    pc = newPc;
                }
                System.DateTime downsampleStopTime = System.DateTime.Now;
                downsampleDuration = (Timedelta)(downsampleStopTime - downsampleStartTime).TotalMilliseconds;

            }
            lock(this)
            {
                if (currentPointcloud != null)
                {
                    currentPointcloud.free();
                    currentPointcloud = null;
                    nDropped++;
                }
                currentPointcloud = pc;
                nRead++;
            }
        }

        override public void Synchronize()
        {

        }

        override public bool LatchFrame()
        {
            lock (this)
            {
                cwipc.pointcloud pc = currentPointcloud;
                if (pc == null)
                {
                    return false;
                }
                currentPointcloud = null;
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
                            Debug.LogError($"{Name()}: Pointcloud size error");
                        }
                    }
                    pc.free();
                    isReady = true;
                }
            }
            return true;
        }

        override public int GetComputeBuffer(ref ComputeBuffer computeBuffer)
        {
            // xxxjack I don't understand this computation of size, the sizeof(float)*4 below and the currentByteArray.Length below that.
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

        override public float GetPointSize()
        {
            if (currentCellSize > 0.0000f) return currentCellSize * cellSizeFactor;
            else return defaultCellSize * cellSizeFactor;
        }

        override public Timedelta getQueueDuration()
        {
            return 0;
        }
    }
}
