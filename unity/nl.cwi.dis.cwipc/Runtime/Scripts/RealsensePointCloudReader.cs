using System;
using UnityEngine;

namespace cwipc
{
    using Timestamp = System.Int64;
    using Timedelta = System.Int64;

    public class RealsensePointCloudReader : BasePointCloudReader
    {
        [Tooltip("Filename of cameraconfig.xml file")]
        public string configFileName;
       
        override public void _AllocateReader()
        {
            
            try
            {
                reader = cwipc.realsense2(configFileName);
                if (reader != null)
                {
                    Debug.Log("{Name()}: Started.");
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
