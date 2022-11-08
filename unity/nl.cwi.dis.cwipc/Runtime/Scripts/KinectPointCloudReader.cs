using System;
using UnityEngine;

namespace cwipc
{
    using Timestamp = System.Int64;
    using Timedelta = System.Int64;

    public class KinectPointCloudReader : BasePointCloudReader
    {
        [Header("Kinect reader specific fields")]
        [Tooltip("Filename of cameraconfig.xml file")]
        public string configFileName;
       
        override public void _AllocateReader()
        {
            
            reader = cwipc.kinect(configFileName);
            if (reader != null)
            {
#if CWIPC_WITH_LOGGING
                Debug.Log($"{Name()}: Started.");
#endif
            }
            else
                throw new System.Exception($"{Name()}: cwipc_kinect could not be created"); // Should not happen, should throw exception
        }
    }
}
