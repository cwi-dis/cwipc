using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace cwipc
{
    using Timestamp = System.Int64;
    using Timedelta = System.Int64;

    public class PointCloudRenderer : MonoBehaviour
    {
        // For reasons I don't understand pointclouds need to be mirrored in the X direction.
        // Doing this on the GameObject.transform has the drawback that coordinate systems
        // become mirrored, for example when cropping a pointcloud. Therefore, we mirror here,
        // by adjusting the matrix.
        ComputeBuffer pointBuffer = null;
        int pointCount = 0;
        [Header("Settings")]
        [Tooltip("Source of pointclouds")]
        public IPointCloudPreparer preparer;
        [Tooltip("Material (to be cloned) to use to render pointclouds")]
        public Material baseMaterial;
        [Tooltip("After how many seconds without data pointcloud becomes ghosted")]
        [SerializeField] protected int timeoutBeforeGhosting = 5; // seconds
        [Tooltip("Mirror pointclouds because they use a right-hand coordinate system (usually true)")]
        [SerializeField] protected bool pcMirrorX = true;

        [Header("Introspection (for debugging)")]
        [Tooltip("Private clone of Material used by this renderer instance")]
        [SerializeField] protected Material material;
        [SerializeField] protected MaterialPropertyBlock block;
        [Tooltip("True if no pointcloud data is being received")]
        [SerializeField] bool dataIsMissing = false;
        [Tooltip("Timestamp of most recent pointcloud (system clock)")]
        [SerializeField] Timestamp lastDataReceived;

        static int instanceCounter = 0;
        int instanceNumber = instanceCounter++;

        public string Name()
        {
            return $"{GetType().Name}#{instanceNumber}";
        }

        public bool isSupported()
        {
            if (baseMaterial == null)
            {
                baseMaterial = Resources.Load<Material>("PointCloud");
            }
            if (baseMaterial == null) return false;
            return baseMaterial.shader.isSupported;
        }

        // Start is called before the first frame update
        void Start()
        {
            if (!isSupported())
            {
                Debug.LogError($"{Name()}: uses shader that is not supported on this graphics card.");
            }
            material = new Material(baseMaterial);
            block = new MaterialPropertyBlock();
            pointBuffer = new ComputeBuffer(1, sizeof(float) * 4);
        }

        private void Update()
        {
            if (preparer == null) return;
            preparer.Synchronize();
        }

        private void LateUpdate()
        {
            if (preparer == null) return;
            bool fresh = preparer.LatchFrame();
            float pointSize = 0;
            System.TimeSpan sinceEpoch = System.DateTime.UtcNow - new System.DateTime(1970, 1, 1);
            Timestamp now = (Timestamp)sinceEpoch.TotalMilliseconds;

            if (fresh)
            {
                lastDataReceived = now;
                if (dataIsMissing)
                {
#if CWIPC_WITH_LOGGING
                    Debug.Log($"{Name()}: Data received again, set pointsize=1");
#endif
                    // Was missing previously. Reset pointsize.
                    block.SetFloat("_PointSizeFactor", 1.0f);
                }
                dataIsMissing = false;
                pointCount = preparer.GetComputeBuffer(ref pointBuffer);
                pointSize = preparer.GetPointSize();
                if (pointBuffer == null || !pointBuffer.IsValid())
                {
                    Debug.LogError($"{Name()}: Invalid pointBuffer");
                    return;
                }
                block.SetBuffer("_PointBuffer", pointBuffer);
                block.SetFloat("_PointSize", pointSize);
            } 
            else
            {
                if (timeoutBeforeGhosting != 0 && now > lastDataReceived + (int)(timeoutBeforeGhosting*1000) && !dataIsMissing)
                {
#if CWIPC_WITH_LOGGING
                    Debug.Log($"{Name()}: No pointcloud received for {timeoutBeforeGhosting} seconds, ghosting with pointsize=0.2");
#endif
                    block.SetFloat("_PointSizeFactor", 0.2f);
                    dataIsMissing = true;
                }
            }
            if (pointBuffer == null || !pointBuffer.IsValid())
            {
                return;
            }
            Matrix4x4 pcMatrix = transform.localToWorldMatrix;
            if (pcMirrorX)
            {
                pcMatrix = pcMatrix * Matrix4x4.Scale(new Vector3(-1, 1, 1));
            }
            block.SetMatrix("_Transform", pcMatrix);
            Graphics.DrawProcedural(material, new Bounds(transform.position, Vector3.one * 2), MeshTopology.Points, pointCount, 1, null, block);
        }

        public void OnDestroy()
        {
            if (pointBuffer != null) { pointBuffer.Release(); pointBuffer = null; }
            if (material != null) { material = null; }
        }
    }
}
