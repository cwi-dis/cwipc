/*
* Software License Agreement (BSD License)
*
*  Point Cloud Library (PCL) - www.pointclouds.org
*  Copyright (c) 2014, Stichting Centrum Wiskunde en Informatica.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of Willow Garage, Inc. nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
*/
#ifndef POINT_CLOUD_CODECV2_H
#define POINT_CLOUD_CODECV2_H

// from PCL library
#include <pcl/io/pcd_io.h>
#include <pcl/io/ply_io.h>
#include <pcl/console/print.h>
#include <pcl/console/parse.h>
#include <pcl/console/time.h>
#include <pcl/compression/octree_pointcloud_compression.h>
#include <pcl/features/normal_3d.h>
#include <pcl/octree/octree_iterator.h>
#include <pcl/octree/impl/octree_iterator.hpp>

// added for cloud codec v2
#include <pcl/cloud_codec_v2/point_coding_v2.h>
#include <pcl/cloud_codec_v2/color_coding_jpeg.h>

#if defined(__clang_major__) && __clang_major__ <= 11
// Older clang releases have trouble with deleted virtual functions
#define workaround_no_deleted_virtual_functions
#endif

namespace pcl{
  
  namespace io{
  using std::uint64_t;
  using std::uint32_t;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /**! 
    * \brief class for computation and compression of time varying cloud frames, extends original PCL cloud codec
    * \author Rufael Mekuria (rufael.mekuria@cwi.nl)
    */
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    struct BoundingBox
    {
      Eigen::Vector4f min_xyz;
      Eigen::Vector4f max_xyz;
    };

    template<typename PointT, typename LeafT = OctreeContainerPointIndices,
      typename BranchT = OctreeContainerEmpty,
      typename OctreeT = Octree2BufBase<LeafT, BranchT> >
    class OctreePointCloudCodecV2 : public OctreePointCloudCompression<PointT,LeafT,BranchT,OctreeT>
    {
		// Important basetype names that we need in a lot of places.
		typedef OctreePointCloudCompression<PointT,LeafT,BranchT,OctreeT> _BaseCodecT;
		typedef OctreePointCloud<PointT, LeafT, BranchT, OctreeT> _PointCloudT;
      public:
        // public typedefs, copied from original implementation by Julius Kammerl
        typedef typename _BaseCodecT::PointCloud PointCloud;
        typedef typename _BaseCodecT::PointCloudPtr PointCloudPtr;
        typedef typename _BaseCodecT::PointCloudConstPtr PointCloudConstPtr;
        typedef _BaseCodecT MacroBlockTree;

        typedef typename OctreeT::LeafNode LeafNode;
        typedef typename OctreeT::BranchNode BranchNode;


        /** \brief Constructor
        * \param compressionProfile_arg:  define compression profile
        * \param pointResolution_arg:  precision of point coordinates
        * \param octreeResolution_arg:  octree resolution at lowest octree level
        * \param doVoxelGridDownDownSampling_arg:  voxel grid filtering
        * \param iFrameRate_arg:  i-frame encoding rate
        * \param doColorEncoding_arg:  enable/disable color coding
        * \param colorBitResolution_arg:  color bit depth
        * \param showStatistics_arg:  output compression statistics
        * \param colorCodingType_arg:  jpeg or pcl dpcm
        * \param doVoxelGridCentroid_arg:  keep voxel grid positions or not 
        * \param createScalebleStream_arg:  scalable bitstream (not yet implemented)
        * \param codeConnectivity_arg:  connectivity coding (not yet implemented)
        * \param jpeg_quality_arg:  quality of the jpeg encoder (jpeg quality)
        */
		OctreePointCloudCodecV2(compression_Profiles_e compressionProfile_arg = MED_RES_ONLINE_COMPRESSION_WITH_COLOR,
			bool showStatistics_arg = false,
			const double pointResolution_arg = 0.001,
			const double octreeResolution_arg = 0.01,
			bool doVoxelGridDownDownSampling_arg = false,
			const unsigned int iFrameRate_arg = 0, /* NO PCL P Frames in this version of the codec !! */
			bool doColorEncoding_arg = true,
			const unsigned char colorBitResolution_arg = 6,
			const unsigned char colorCodingType_arg = 0,
		  bool doVoxelGridCentroid_arg = true, 
          bool createScalableStream_arg = true, 
          bool codeConnectivity_arg = false,
          int jpeg_quality_arg = 75,
          int num_threads=0) :
        _BaseCodecT(
          compressionProfile_arg,
          showStatistics_arg,
          pointResolution_arg,
          octreeResolution_arg,
          doVoxelGridDownDownSampling_arg,
          iFrameRate_arg,
          doColorEncoding_arg,
          colorBitResolution_arg), 
          color_coding_type_(colorCodingType_arg), 
          octreeResolution(octreeResolution_arg),
          colorBitResolution(colorBitResolution_arg),
          jpeg_quality(jpeg_quality_arg),

          do_voxel_centroid_enDecoding_(doVoxelGridCentroid_arg),
          create_scalable_bitstream_(createScalableStream_arg),
          do_connectivity_encoding_(codeConnectivity_arg),
          jp_color_coder_(jpeg_quality_arg, colorCodingType_arg),
          num_threads_(num_threads)
        {
          macroblock_size_ = 16; // default macroblock size is 16x16x16
          icp_var_threshold_ = 100;
          icp_max_iterations_=50;
          do_icp_color_offset_ = false;
          transformationepsilon_=1e-8;
        }

        void initialization ()
        {
        }

        void setMacroblockSize(int size)
        {
          macroblock_size_ = size;
        }

        void setColorVarThreshold(int size)
        {
          macroblock_size_ = size;
        }

        void setMaxIterations(int max_in)
        {
          icp_max_iterations_ = max_in;
        }

        void setDoICPColorOffset(bool doit)
        {
          do_icp_color_offset_ = doit;
        }

        void setDoICPColorOffset(float tfeps)
        {
          transformationepsilon_ = tfeps;
        }

#ifndef workaround_no_deleted_virtual_functions
        void
            encodePointCloud(const PointCloudConstPtr& cloud_arg, std::ostream& compressed_tree_data_out_arg) = delete;
#endif
        void
            encodePointCloud(const PointCloudConstPtr& cloud_arg, std::ostream& compressed_tree_data_out_arg, uint64_t timeStamp);

#ifndef workaround_no_deleted_virtual_functions
        bool
            decodePointCloud(std::istream& compressed_tree_data_in_arg, PointCloudPtr& cloud_arg) = delete;
#endif
        bool
            decodePointCloud(std::istream& compressed_tree_data_in_arg, PointCloudPtr& cloud_arg, uint64_t& timeStamp);

#ifndef workaround_no_deleted_virtual_functions
        void generatePointCloudDeltaFrame(const PointCloudConstPtr& icloud_arg, const PointCloudConstPtr& pcloud_arg, PointCloudPtr& out_cloud_arg,
                std::ostream& i_coded_data, std::ostream& p_coded_data, bool icp_on_original, bool write_out_cloud) = delete;
#endif
        virtual void
            generatePointCloudDeltaFrame(const PointCloudConstPtr& icloud_arg, const PointCloudConstPtr& pcloud_arg, PointCloudPtr& out_cloud_arg,
                std::ostream& i_coded_data, std::ostream& p_coded_data, bool icp_on_original, bool write_out_cloud, uint64_t timeStamp);

#ifndef workaround_no_deleted_virtual_functions
        virtual void
            encodePointCloudDeltaFrame(const PointCloudConstPtr& icloud_arg, const PointCloudConstPtr& pcloud_arg, PointCloudPtr& out_cloud_arg,
                std::ostream& i_coded_data, std::ostream& p_coded_data, bool icp_on_original, bool write_out_cloud) = delete;
#endif
        virtual void
            encodePointCloudDeltaFrame(const PointCloudConstPtr& icloud_arg, const PointCloudConstPtr& pcloud_arg, PointCloudPtr& out_cloud_arg,
                std::ostream& i_coded_data, std::ostream& p_coded_data, bool icp_on_original, bool write_out_cloud, uint64_t timeStamp);

#ifndef workaround_no_deleted_virtual_functions
        virtual bool
            decodePointCloudDeltaFrame(const PointCloudConstPtr& icloud_arg, PointCloudPtr& out_cloud_arg,
                std::istream& i_coded_data, std::istream& p_coded_data) = delete;
#endif

        virtual bool
            decodePointCloudDeltaFrame(const PointCloudConstPtr& icloud_arg, PointCloudPtr& out_cloud_arg,
                std::istream& i_coded_data, std::istream& p_coded_data, uint64_t& timeStamp);

        //! function to return performance metric
        uint64_t *
        getPerformanceMetrics()
        {
          return compression_performance_metrics;
        };

        //! helper function to return
        float
        getMacroBlockPercentage()
        {
          return shared_macroblock_percentage_;
        };

        //! helper function to return convergence percentage
        float
        getMacroBlockConvergencePercentage(){
          return shared_macroblock_convergence_percentage_;
        };
      /** \brief
       *  \param point_clouds: an array of pointers to point_clouds to be inspected and modified
       * by \ref pcl_outlier_filter. A point in a cloud is considered an outlier, if there are
       * less than 'num_points' other points in that cloud within distance 'radius'
       */
      static void
      remove_outliers (std::vector<PointCloudPtr> &point_clouds, int num_points, double radius, unsigned int debug_level=0);
      /** \brief
       *  \param point_clouds: an vector of suitably aligned pointers to point_clouds to be inspected and modified
       * to normalize their bouding boxes s.t. they effectivly can be used for interframe coding.
       * \\returns the common bounding box for \\ref point clouds
       */
      static BoundingBox
      normalize_pointclouds (std::vector<PointCloudPtr> &point_clouds, std::vector<BoundingBox, Eigen::aligned_allocator<BoundingBox> > &bouding_boxes, double bb_expand_factor, std::vector<float> dyn_range, std::vector<float> offset, unsigned int debug_level=0);
        
      static void
      restore_scaling (PointCloudPtr &point_clouds, const BoundingBox& bb);
      
      //! function for coding an enhancement layer
      // virtual void
      // encodeEnhancementLayer(const PointCloudConstPtr &cloud_arg, std::ostream& compressed_tree_data_out_arg);

      //! function for coding an enhancment layer
      // virtual void
      // decodeEnhancementLayer(std::istream& compressed_tree_data_in_arg, PointCloudPtr &cloud_arg, PointCloudPtr &cloud_arg_enh);

      protected: 

        // protected functions for cloud_codec_v2

        virtual void
        simplifyPCloud(const PointCloudConstPtr &pcloud_arg_in, PointCloudPtr &out_cloud );

        MacroBlockTree *
        generate_macroblock_tree(PointCloudConstPtr in_cloud);
 
		void 
        do_icp_prediction(
          Eigen::Matrix4f &rigid_transform,
          PointCloudPtr i_cloud,
          PointCloudPtr p_cloud,
          bool & has_converged,
          char *rgb_offsets
        );

        // protected functions overriding OctreePointCloudCompression
        
#ifndef workaround_no_deleted_virtual_functions        
        void writeFrameHeader(std::ostream& compressed_tree_data_out_arg) = delete;
#endif
        void
        writeFrameHeader(std::ostream& compressed_tree_data_out_arg, uint64_t timeStamp);

#ifndef workaround_no_deleted_virtual_functions
        void readFrameHeader(std::istream& compressed_tree_data_in_arg) = delete;
#endif

        void
        readFrameHeader(std::istream& compressed_tree_data_in_arg, uint64_t& timeStamp);

        virtual void
        serializeTreeCallback (LeafT &leaf_arg, const OctreeKey& key_arg);

        virtual void 
        deserializeTreeCallback (LeafT&, const OctreeKey& key_arg);

        bool
        syncToHeader (std::istream& compressed_tree_data_in_arg);

        void
        entropyEncoding (std::ostream& compressed_tree_data_out_arg1, 
                         std::ostream& compressed_tree_data_out_arg2);

        void
        entropyDecoding (std::istream& compressed_tree_data_in_arg1, 
                         std::istream& compressed_tree_data_in_arg2);

        // protected variables cloud codec v2

        uint32_t color_coding_type_; //! color coding with jpeg, graph transform, or differential encodings
    public:
		double  octreeResolution;
		unsigned char colorBitResolution;
		int jpeg_quality;
    protected:
        bool do_voxel_centroid_enDecoding_;  //! encode the centroid in addition

        bool create_scalable_bitstream_;  //! create a scalable bitstream (not yet implemented)

        bool do_connectivity_encoding_;   //! encode the connectivity (not yet implemented)

        PointCodingV2<PointT> centroid_coder_; //! centroid encoding

        ColorCodingJPEG<PointT> jp_color_coder_; //! new color coding via jpeg

        uint64_t compression_performance_metrics[3]; //! octree_bytes, centroid_bytes, color_bytes, monitor the buildup of compressed data

        static const char* frame_header_identifier_; //! new frame header identifier

        int macroblock_size_;  //! macroblock size for inter predictive frames

        float shared_macroblock_percentage_;

        float shared_macroblock_convergence_percentage_;

        float icp_var_threshold_;

        int icp_max_iterations_;

        float transformationepsilon_;

        bool do_icp_color_offset_;

        int conv_count_;

        // store the colors for usage in the enhancement layers
        std::vector<char> decoded_colors_;
        
        //! number of omp threads
        int num_threads_;

        // inherited protected members needed
        using OctreeT::deserializeTree; // does not work in windows
        using OctreeT::leaf_count_;
        using OctreeT::serializeTree;
#ifndef CWIPC_CODEC_WITH_SINGLE_BUF
        using OctreeT::switchBuffers;
        using OctreeT::deleteCurrentBuffer;
#endif
        using _PointCloudT::addPointsFromInputCloud;
//        using _PointCloudT::deleteCurrentBuffer;
        using _PointCloudT::deleteTree;
        using _PointCloudT::getTreeDepth;
        using _PointCloudT::input_;
        using _PointCloudT::min_x_;
        using _PointCloudT::min_y_;
        using _PointCloudT::min_z_;
        using _PointCloudT::resolution_;
        using _PointCloudT::setInputCloud;
        using _BaseCodecT::b_show_statistics_;
        using _BaseCodecT::binary_tree_data_vector_;
        using _BaseCodecT::cloud_with_color_;
        using _BaseCodecT::color_bit_resolution_ ;
        using _BaseCodecT::compressed_color_data_len_;
        using _BaseCodecT::compressed_point_data_len_;
        using _BaseCodecT::color_coder_;
        using _BaseCodecT::data_with_color_;
        using _BaseCodecT::do_color_encoding_;
        using _BaseCodecT::do_voxel_grid_enDecoding_;
        using _BaseCodecT::entropy_coder_;
        using _BaseCodecT::frame_ID_;
        using _BaseCodecT::i_frame_;
        using _BaseCodecT::i_frame_counter_;
        using _BaseCodecT::i_frame_rate_;
        using _BaseCodecT::object_count_;
        using _BaseCodecT::octree_resolution_;
        using _BaseCodecT::output_;
        using _BaseCodecT::point_coder_;
        using _BaseCodecT::point_color_offset_;
        using _BaseCodecT::point_count_;
        using _BaseCodecT::point_count_data_vector_;
        using _BaseCodecT::point_count_data_vector_iterator_;
        using _BaseCodecT::point_resolution_;
        using _BaseCodecT::setOutputCloud;
        using _BaseCodecT::writeFrameHeader;
 
    };
    // define frame identifier for cloud codec v2
    template<typename PointT, typename LeafT, typename BranchT, typename OctreeT>
    const char* OctreePointCloudCodecV2<PointT, LeafT, BranchT, OctreeT>::frame_header_identifier_ = "<PCL-OCT-CODECV2-COMPRESSED>";
  }
}
#endif


