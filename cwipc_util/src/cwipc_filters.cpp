// cwi_encode.cpp : Defines the exported functions for the DLL application.
//
#include <cstdint>
#include <chrono>
#include <sstream>

#ifdef WIN32
#define _CWIPC_UTIL_EXPORT __declspec(dllexport)
#else
#define _CWIPC_UTIL_EXPORT
#endif

#define PCL_NO_PRECOMPILE

#include "cwipc_util/api_pcl.h"
#include "cwipc_util/api.h"
#include "cwipc_util/internal/logging.hpp"

#include <pcl/point_cloud.h>
#include <pcl/exceptions.h>

#include <pcl/octree/octree.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/statistical_outlier_removal.h>

namespace pcl {
    template class VoxelGrid<cwipc_pcl_point>;
}

cwipc_pointcloud* cwipc_downsample_voxelgrid(cwipc_pointcloud *pc, float cellsize) {
    if (pc == NULL) {
        return NULL;
    }

    cwipc_pcl_pointcloud src = pc->access_pcl_pointcloud();

    if (src == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_downsample_voxelgrid", "pcl_pointcloud is NULL");
        return NULL;
    }

    float oldcellsize = pc->cellsize();

    if (oldcellsize >= cellsize) {
        cellsize = oldcellsize;
    }

    cwipc_pcl_pointcloud dst = new_cwipc_pcl_pointcloud();

    // Step 1 - Voxelize
    try {
        pcl::VoxelGrid<cwipc_pcl_point> grid;
        grid.setInputCloud(src);
        grid.setLeafSize(cellsize, cellsize, cellsize);
        grid.setSaveLeafLayout(true);
        grid.filter(*dst);

        if (dst->empty()) {
            // No points, algorithm apparently failed.
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_downsample", "VoxelGrid filter produced empty pointcloud");
            return NULL;
        }

        // Step 2 - Clear tile numbers in destination
        for (auto& dstpt : dst->points) {
            dstpt.a = 0;
        }

        // Step 3 - Do OR of all contribution point tile numbers in destination.
        for (auto& srcpt : src->points) {
            int dstIndex = grid.getCentroidIndex(srcpt);
            auto& dstpt = dst->points[dstIndex];
            dstpt.a |= srcpt.a;
        }
    } catch (pcl::PCLException& e) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_downsample", std::string("VoxelGrid PCL exception: ") + e.detailedMessage());
        return NULL;
    } catch (std::exception& e) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_downsample", std::string("VoxelGrid std exception: ") + e.what());
        return NULL;
    }

    cwipc_pointcloud *rv = cwipc_from_pcl(dst, pc->timestamp(), NULL, CWIPC_API_VERSION);
    rv->_set_cellsize(cellsize);

    return rv;
}

cwipc_pointcloud* cwipc_downsample(cwipc_pointcloud *pc, float cellsize) {
    if (cellsize < 0) {
        return cwipc_downsample_voxelgrid(pc, -cellsize);
    }
    if (pc == NULL) {
        return NULL;
    }

    cwipc_pcl_pointcloud src = pc->access_pcl_pointcloud();

    if (src == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_downsample", "pcl_pointcloud is NULL");
        return NULL;
    }
    float oldcellsize = pc->cellsize();

    if (oldcellsize >= cellsize) {
        cellsize = oldcellsize;
    }

    cwipc_pcl_pointcloud dst = new_cwipc_pcl_pointcloud();
    cwipc_pcl_pointcloud grid_input = new_cwipc_pcl_pointcloud();
    cwipc_pcl_pointcloud grid_output = new_cwipc_pcl_pointcloud();

    const int octree_count = 8*8; // we will first split the point cloud into 8x8 cells
    float octree_cellsize = octree_count * cellsize;
    
    try {
        // At the upper levels we will use an octree to split the work into smaller parts.
        pcl::octree::OctreePointCloud<cwipc_pcl_point> octree(octree_cellsize);
        
        octree.setInputCloud(src);
        octree.addPointsFromInputCloud();
        
        // Upper level: get all leaf nodes from the octree
        for(auto it_leaf = octree.leaf_depth_begin(); it_leaf != octree.leaf_depth_end(); it_leaf++) {
            // Upper level: for each octree leaf put all points in a temporary pointcloud
            auto& points = it_leaf.getLeafContainer();
            grid_input->clear();
            auto& indices = points.getPointIndicesVector();
            for(auto point_idx : indices) {
                grid_input->push_back(src->at(point_idx));
            }
            // Lower level step 1 - Voxelize the points in the temporary pointcloud
            grid_output->clear();
            // At the lower levels we will use a VoxelGrid to do the averaging.
            pcl::VoxelGrid<cwipc_pcl_point> grid;
            grid.setLeafSize(cellsize, cellsize, cellsize);
      
            grid.setSaveLeafLayout(true);
            grid.setInputCloud(grid_input);
            grid.filter(*grid_output);
            if(grid_output->empty()) {
                continue;
            }

            // Lower level step 2 - Clear tile numbers in destination
            for (auto& dstpt : grid_output->points) {
                dstpt.a = 0;
            }

            // Lower level step 3 - Do OR of all contribution point tile numbers in destination.
            for (auto& srcpt : grid.getInputCloud()->points) {
                int dstIndex = grid.getCentroidIndex(srcpt);
                auto& dstpt = grid_output->points[dstIndex];
                dstpt.a |= srcpt.a;
            }
            // Final step: add the results to the final destination pointcloud
            *dst += *grid_output;
        }
    } catch (pcl::PCLException& e) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_downsample", std::string("PCL exception: ") + e.detailedMessage());
        return NULL;
    } catch (std::exception& e) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_downsample", std::string("std exception: ") + e.what());
        return NULL;
    }
    // Now create the cwipc point cloud from the pcl point cloud
    cwipc_pointcloud *rv = cwipc_from_pcl(dst, pc->timestamp(), NULL, CWIPC_API_VERSION);
    // And copy the metadata
    rv->_set_cellsize(cellsize);

    return rv;
}

/// <summary>
/// All points who have a distance larger than StddevMulThresh standard deviation of the mean distance to the query point will be marked as outliers and removed
/// </summary>
/// <param name="pc">Source point cloud to be cleaned</param>
/// <param name="kNeighbors">number of neighbors to analyze for each point</param>
/// <param name="stddevMulThresh">standard deviation multiplier</param>
/// <returns>Cleaned point cloud</returns>
cwipc_pcl_pointcloud cwipc_remove_outliers(cwipc_pointcloud* pc, int kNeighbors, float stddevMulThresh) {
    if (pc == NULL) {
        return NULL;
    }

    cwipc_pcl_pointcloud src = pc->access_pcl_pointcloud();

    if (src == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_remove_outliers", "pcl_pointcloud is NULL");
        return NULL;
    }

    cwipc_pcl_pointcloud dst = new_cwipc_pcl_pointcloud();

    // Apply statistical outlier removal
    try {
        pcl::StatisticalOutlierRemoval<cwipc_pcl_point> sor;
        sor.setInputCloud(src);
        sor.setMeanK(kNeighbors);
        sor.setStddevMulThresh(stddevMulThresh);
        sor.filter(*dst);
    } catch (pcl::PCLException& e) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_remove_outliers", std::string("PCL exception: ") + e.detailedMessage());
        return NULL;
    } catch (std::exception& e) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_remove_outliers", std::string("std exception: ") + e.what());
        return NULL;
    }

    return dst;
}


/// <summary>
/// All points who have a distance larger than StddevMulThresh standard deviation of the mean distance to the query point will be marked as outliers and removed
/// </summary>
/// <param name="pc">Source point cloud to be cleaned</param>
/// <param name="kNeighbors">number of neighbors to analyze for each point</param>
/// <param name="stddevMulThresh">standard deviation multiplier</param>
/// <param name="perTile">decides if apply the filter per tile or to the full pointcloud</param>
/// <returns>Cleaned point cloud</returns>
cwipc_pointcloud* cwipc_remove_outliers(cwipc_pointcloud* pc, int kNeighbors, float stddevMulThresh, bool perTile) {
    if (pc == NULL) {
        return NULL;
    }

    cwipc_pcl_pointcloud src = pc->access_pcl_pointcloud();

    if (src == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_remove_outliers", "pcl_pointcloud is NULL");
        return NULL;
    }

    cwipc_pcl_pointcloud dst = new_cwipc_pcl_pointcloud();

    // Apply statistical outlier removal
    try {
        if (perTile) {
            std::vector< int > tiles;

            for (auto pt : src->points) {
                int tile = pt.a;

                if (std::find(tiles.begin(), tiles.end(), tile) != tiles.end()) {
                    continue;
                } else {
                    tiles.push_back(tile);
                }
            }

            for (int tile : tiles) {
                cwipc_pointcloud* aux_pc = cwipc_tilefilter(pc, tile);
                cwipc_pcl_pointcloud aux_dst = cwipc_remove_outliers(aux_pc, kNeighbors, stddevMulThresh);
                *dst += *aux_dst;
                aux_pc->free();
            }

            cwipc_pointcloud* rv = cwipc_from_pcl(dst, pc->timestamp(), NULL, CWIPC_API_VERSION);
            rv->_set_cellsize(pc->cellsize());

            return rv;
        } else {
            dst = cwipc_remove_outliers(pc, kNeighbors, stddevMulThresh);
            cwipc_pointcloud* rv = cwipc_from_pcl(dst, pc->timestamp(), NULL, CWIPC_API_VERSION);
            rv->_set_cellsize(pc->cellsize());

            return rv;
        }
    } catch (pcl::PCLException& e) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_remove_outliers", std::string("PCL exception: ") + e.detailedMessage());
        return NULL;
    } catch (std::exception& e) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_remove_outliers", std::string("std exception: ") + e.what());
        return NULL;
    }

    return pc;
}


cwipc_pointcloud* cwipc_tilefilter(cwipc_pointcloud *pc, int tile) {
    if (pc == NULL) {
        return NULL;
    }

    cwipc_pcl_pointcloud src = pc->access_pcl_pointcloud();

    if (src == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_tilefilter", "pcl_pointcloud is NULL");
        return NULL;
    }

    cwipc_pcl_pointcloud dst = new_cwipc_pcl_pointcloud();

    for (auto pt : src->points) {
        if (tile == 0 || tile == pt.a) {
            dst->points.push_back(pt);
        }
    }

    cwipc_pointcloud *rv = cwipc_from_pcl(dst, pc->timestamp(), NULL, CWIPC_API_VERSION);
    rv->_set_cellsize(pc->cellsize());


    return rv;
}

cwipc_pointcloud* cwipc_tilemap(cwipc_pointcloud *pc, uint8_t map[256]) {
    if (pc == NULL) {
        return NULL;
    }

    cwipc_pcl_pointcloud src = pc->access_pcl_pointcloud();

    if (src == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_tilemap", "pcl_pointcloud is NULL");
        return NULL;
    }

    cwipc_pcl_pointcloud dst = new_cwipc_pcl_pointcloud();

    for (auto pt : src->points) {
        pt.a = map[pt.a];
        dst->points.push_back(pt);
    }

    cwipc_pointcloud *rv = cwipc_from_pcl(dst, pc->timestamp(), NULL, CWIPC_API_VERSION);
    rv->_set_cellsize(pc->cellsize());

    return rv;
}

cwipc_pointcloud* cwipc_crop(cwipc_pointcloud *pc, float bbox[6]) {
    if (pc == NULL) {
        return NULL;
    }

    cwipc_pcl_pointcloud src = pc->access_pcl_pointcloud();

    if (src == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_crop", "pcl_pointcloud is NULL");
        return NULL;
    }

    cwipc_pcl_pointcloud dst = new_cwipc_pcl_pointcloud();

    for (auto pt : src->points) {
        if (bbox[0] <= pt.x && pt.x < bbox[1] &&
            bbox[2] <= pt.y && pt.y < bbox[3] &&
            bbox[4] <= pt.z && pt.z < bbox[5]) {

            dst->points.push_back(pt);
        }
    }

    cwipc_pointcloud *rv = cwipc_from_pcl(dst, pc->timestamp(), NULL, CWIPC_API_VERSION);
    rv->_set_cellsize(pc->cellsize());

    return rv;
}

cwipc_pointcloud* cwipc_colormap(cwipc_pointcloud *pc, uint32_t clearBits, uint32_t setBits) {
    if (pc == NULL) {
        return NULL;
    }

    cwipc_pcl_pointcloud src = pc->access_pcl_pointcloud();

    if (src == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_colormap", "pcl_pointcloud is NULL");
        return NULL;
    }

    cwipc_pcl_pointcloud dst = new_cwipc_pcl_pointcloud();

    for (auto pt : src->points) {
        pt.rgba &= ~clearBits;
        pt.rgba |= setBits;
        dst->points.push_back(pt);
    }

    cwipc_pointcloud *rv = cwipc_from_pcl(dst, pc->timestamp(), NULL, CWIPC_API_VERSION);
    rv->_set_cellsize(pc->cellsize());

    return rv;
}

cwipc_pointcloud* cwipc_join(cwipc_pointcloud *pc1, cwipc_pointcloud *pc2) {
    if (pc1 == NULL || pc2 == NULL) {
        return NULL;
    }

    cwipc_pcl_pointcloud src1 = pc1->access_pcl_pointcloud();
    cwipc_pcl_pointcloud src2 = pc2->access_pcl_pointcloud();

    if (src1 == NULL || src2 == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_join", "some pcl_pointcloud is NULL");
        return NULL;
    }

    cwipc_pcl_pointcloud dst = new_cwipc_pcl_pointcloud();

    for (auto pt : src1->points) {
        dst->points.push_back(pt);
    }

    for (auto pt : src2->points) {
        dst->points.push_back(pt);
    }

    uint64_t timestamp = std::min(pc1->timestamp(), pc2->timestamp());
    cwipc_pointcloud *rv = cwipc_from_pcl(dst, timestamp, NULL, CWIPC_API_VERSION);
    float cellsize = std::min(pc1->cellsize(), pc2->cellsize());
    rv->_set_cellsize(cellsize);

    // xxxNacho. how do we deal with aux data when we do a join?
    return rv;
}

