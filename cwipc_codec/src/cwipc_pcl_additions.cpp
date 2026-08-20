#define PCL_NO_PRECOMPILE

#include "cwipc_codec_config.h"

#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/common/transforms.h>
#include <pcl/common/common_headers.h>

#define PCL_INSTALLED // xxxjack temp
#include <pcl/cloud_codec_v2/point_cloud_codec_v2.h>
#include <pcl/cloud_codec_v2/impl/point_cloud_codec_v2_impl.hpp>

#include "cwipc_util/api_pcl.h"
#include "cwipc_util/api.h"

#ifdef CWIPC_CODEC_WITH_SINGLE_BUF
typedef pcl::octree::OctreeBase<pcl::octree::OctreeContainerPointIndices,pcl::octree::OctreeContainerEmpty> cwipc_octree_type;
#else
typedef pcl::octree::Octree2BufBase<pcl::octree::OctreeContainerPointIndices,pcl::octree::OctreeContainerEmpty> cwipc_octree_type;
#endif

namespace pcl {
    namespace io {
        template class OctreePointCloudCodecV2<
            cwipc_pcl_point,
            pcl::octree::OctreeContainerPointIndices,
            pcl::octree::OctreeContainerEmpty,
            cwipc_octree_type
        >;
    }

    namespace octree {
        // This template declaration is needed for XCode build for profileing...
        template class OctreePointCloud<
            cwipc_pcl_point,
            pcl::octree::OctreeContainerPointIndices,
            pcl::octree::OctreeContainerEmpty,
            cwipc_octree_type
        >;
    }

    template class RadiusOutlierRemoval<cwipc_pcl_point>;
}
