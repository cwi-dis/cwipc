using System;
using UnityEngine;

namespace cwipc
{
    using Timestamp = System.Int64;
    using Timedelta = System.Int64;

    public class RealsensePointCloudReader : BasePointCloudReader
    {
        [Header("Realsense reader specific fields")]
        [Tooltip("Filename of cameraconfig.xml file")]
        public string configFileName;
       
        override public void _AllocateReader()
        {
            
            try
            {
                reader = cwipc.realsense2(configFileName);
                if (reader != null)
                {
#if CWIPC_WITH_LOGGING
                    Debug.Log($"{Name()}: Started.");
#endif
                }
                else
                    throw new System.Exception($"{Name()}: cwipc_realsense2 could not be created"); // Should not happen, should throw exception
            }
            catch (System.Exception e)
            {
#if CWIPC_WITH_LOGGING
                Debug.Log($"{Name()}: exception {e.ToString()}");
#endif
                throw;
            }
        }
    }
}
