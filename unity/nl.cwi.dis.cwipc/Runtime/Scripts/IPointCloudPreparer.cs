using UnityEngine;

namespace cwipc
{
    using Timestamp = System.Int64;
    using Timedelta = System.Int64;

    abstract public class IPointCloudPreparer : MonoBehaviour
    {
        abstract public void Synchronize();
        abstract public bool LatchFrame();
        abstract public int GetComputeBuffer(ref ComputeBuffer computeBuffer);
        abstract public float GetPointSize();
        abstract public Timedelta getQueueDuration();
    }
}