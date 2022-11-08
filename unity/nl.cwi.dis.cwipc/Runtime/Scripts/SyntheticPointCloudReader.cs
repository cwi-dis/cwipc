using System;
using UnityEngine;

namespace cwipc
{
    using Timestamp = System.Int64;
    using Timedelta = System.Int64;

    public class SyntheticPointCloudReader : BasePointCloudReader
    {
        [Header("Synthetic reader specific fields")]
        [Tooltip("Produce pointclouds at this number of frames per second (if nonzero)")]
        public int frameRate = 0;
        [Tooltip("Approximate number of points per cloud (if nonzero)")]
        public int nPoints = 0;

        override public void _AllocateReader()
        {
            if (frameRate == 0)
            {
                frameInterval = System.TimeSpan.Zero;
            }
            else
            {
                frameInterval = System.TimeSpan.FromSeconds(1 / frameRate);
            }
            try
            {
                reader = cwipc.synthetic((int)frameRate, nPoints);
                if (reader != null)
                {
#if CWIPC_WITH_LOGGING
                    Debug.Log($"{Name()}: Started.");
#endif
                }
                else
                    throw new System.Exception($"{Name()}: cwipc_synthetic could not be created"); // Should not happen, should throw exception
            }
            catch (System.Exception e)
            {
                Debug.Log($"{Name()}: Exception: {e.Message}");
                throw;
            }
        }
    }
}
