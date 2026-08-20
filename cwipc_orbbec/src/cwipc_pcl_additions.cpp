#define PCL_NO_PRECOMPILE
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/common/transforms.h>
#include <pcl/common/common_headers.h>

#include <pcl/filters/impl/voxel_grid.hpp>

#include "cwipc_util/api_pcl.h"
#include "cwipc_util/api.h"

PCL_INSTANTIATE_PCLBase(cwipc_pcl_point)
PCL_INSTANTIATE_VoxelGrid(cwipc_pcl_point)
