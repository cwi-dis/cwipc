using System;
using System.Runtime.InteropServices;

namespace cwipc
{
    using Timestamp = System.Int64;
    using Timedelta = System.Int64;

    /// <summary>
    /// C# interface to cwipc dynamic libraries. 
    /// </summary>
    public class cwipc
    {
        /// <summary>
        /// Low-level compatible structure with encoder parameters.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct encoder_params
        {
            public bool do_inter_frame;    /**< (unused in this version, must be false) do inter-frame compression */
            public int gop_size;           /**< (unused in this version, ignored) spacing of I frames for inter-frame compression */
            public float exp_factor;       /**< (unused in this version, ignored). Bounding box expansion factor for inter-frame coding */
            public int octree_bits;        /**< Octree depth: a fully populated octree will have 8**octree_bits points */
            public int jpeg_quality;       /**< JPEG encoding quality */
            public int macroblock_size;    /**< (unused in this version, ignored) macroblock size for inter-frame prediction */
            public int tilenumber;         /**< 0 for encoding full pointclouds, > 0 for selecting a single tile to encode */
            public float voxelsize;        /**< If non-zero run voxelizer with this cell size to get better tiled pointcloud */
            public int n_parallel;          /**< If greater than 1 use multiple threads to encode successive pointclouds in parallel */
        };

        /// <summary>
        /// Low-level compatible 3D vector.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct vector
        {
            public double x;
            public double y;
            public double z;
        };

        /// <summary>
        /// Low-level compatible point structure.
        /// 16 bytes: 3 4-byte float coordinates, 3 1-byte colors, 1 1-byte tile mask.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct point
        {
            public float x;
            public float y;
            public float z;
            public byte r;
            public byte g;
            public byte b;
            public byte tile;
        };

        /// <summary>
        /// Low-level compatible structure with tiling information.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct tileinfo
        {
            public vector normal;
            public IntPtr cameraName;
            public byte ncamera;
            public byte cameraMask;
        };

        /// <summary>
        /// Low level interface to cwipc_util native dynamic library. See C++ documentation for details.
        /// </summary>
        private class _API_cwipc_util
        {
            const string myDllName = "cwipc_util";
            public const ulong CWIPC_API_VERSION = 0x20220126;

            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_read([MarshalAs(UnmanagedType.LPStr)]string filename, Timestamp timestamp, ref IntPtr errorMessage, ulong apiVersion = CWIPC_API_VERSION);
			[DllImport(myDllName)]
			internal extern static IntPtr cwipc_read_debugdump([MarshalAs(UnmanagedType.LPStr)]string filename, ref System.IntPtr errorMessage, System.UInt64 apiVersion = CWIPC_API_VERSION);
            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_from_packet(IntPtr packet, IntPtr size, ref System.IntPtr errorMessage, System.UInt64 apiVersion = CWIPC_API_VERSION);
            [DllImport(myDllName)]
            internal extern static void cwipc_free(IntPtr pc);
            [DllImport(myDllName)]
            internal extern static Timestamp cwipc_timestamp(IntPtr pc);
            [DllImport(myDllName)]
            internal extern static int cwipc_count(IntPtr pc);
            [DllImport(myDllName)]
            internal extern static float cwipc_cellsize(IntPtr pc);
            [DllImport(myDllName)]
            internal extern static void cwipc__set_cellsize(IntPtr pc, float cellsize);
            [DllImport(myDllName)]
            internal extern static void cwipc__set_timestamp(IntPtr pc, Timestamp timestamp);
            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_get_uncompressed_size(IntPtr pc);
            [DllImport(myDllName)]
            internal extern static int cwipc_copy_uncompressed(IntPtr pc, IntPtr data, IntPtr size);

            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_copy_packet(IntPtr pc, IntPtr packet, IntPtr size);

            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_source_get(IntPtr src);
            [DllImport(myDllName)]
            internal extern static bool cwipc_source_eof(IntPtr src);
            [DllImport(myDllName)]
            internal extern static bool cwipc_source_available(IntPtr src, bool available);
            [DllImport(myDllName)]
            internal extern static void cwipc_source_free(IntPtr src);

            [DllImport(myDllName)]
            internal extern static int cwipc_tiledsource_maxtile(IntPtr src);
            [DllImport(myDllName)]
            internal extern static bool cwipc_tiledsource_get_tileinfo(IntPtr src, int tileNum, [Out] out tileinfo _tileinfo);

            [DllImport(myDllName)]
            internal extern static int cwipc_sink_free(IntPtr sink);
            [DllImport(myDllName)]
            internal extern static int cwipc_sink_feed(IntPtr sink, IntPtr pc, bool clear);
            [DllImport(myDllName)]
            internal extern static int cwipc_sink_caption(IntPtr sink, [MarshalAs(UnmanagedType.LPStr)]string caption);
            [DllImport(myDllName)]
            internal extern static int cwipc_sink_interact(IntPtr sink, [MarshalAs(UnmanagedType.LPStr)]string prompt, [MarshalAs(UnmanagedType.LPStr)]string responses, int millis);

            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_synthetic(int fps, int npoints, ref IntPtr errorMessage, ulong apiVersion = CWIPC_API_VERSION);

            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_from_certh(IntPtr certhPC, float[] origin, float[] bbox, Timestamp timestamp, ref IntPtr errorMessage, ulong apiVersion = CWIPC_API_VERSION);

            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_proxy([MarshalAs(UnmanagedType.LPStr)]string ip, int port, ref IntPtr errorMessage, ulong apiVersion = CWIPC_API_VERSION);

            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_downsample(IntPtr pc, float voxelSize);

            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_tilefilter(IntPtr pc, int tilenum);

            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_tilemap(IntPtr pc, byte[] map);

            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_colormap(IntPtr pc, UInt32 clearBits, UInt32 setBits);

            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_crop(IntPtr pc, float[] bbox);

            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_join(IntPtr pc1, IntPtr pc2);

        }

        /// <summary>
        /// Low level interface to cwipc_realsense2 native dynamic library. See C++ documentation for details.
        /// </summary>
        private class _API_cwipc_realsense2
        {
            const string myDllName = "cwipc_realsense2";

            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_realsense2([MarshalAs(UnmanagedType.LPStr)]string filename, ref IntPtr errorMessage, ulong apiVersion = _API_cwipc_util.CWIPC_API_VERSION);
        }

        /// <summary>
        /// Low level interface to cwipc_kinect native dynamic library. See C++ documentation for details.
        /// </summary>
        private class _API_cwipc_kinect
        {
            const string myDllName = "cwipc_kinect";

            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_kinect([MarshalAs(UnmanagedType.LPStr)]string filename, ref IntPtr errorMessage, ulong apiVersion = _API_cwipc_util.CWIPC_API_VERSION);
        }

        /// <summary>
        /// Low level interface to cwipc_codec native dynamic library. See C++ documentation for details.
        /// </summary>
        private class _API_cwipc_codec
        {
            const string myDllName = "cwipc_codec";
            public const int CWIPC_ENCODER_PARAM_VERSION = 0x20220607;

            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_new_decoder(ref IntPtr errorMessage, ulong apiVersion = _API_cwipc_util.CWIPC_API_VERSION);

            [DllImport(myDllName)]
            internal extern static void cwipc_decoder_feed(IntPtr dec, IntPtr compFrame, int len);

            [DllImport(myDllName)]
            internal extern static void cwipc_decoder_close(IntPtr dec);

            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_new_encoder(int paramVersion, ref encoder_params encParams, ref IntPtr errorMessage, ulong apiVersion = _API_cwipc_util.CWIPC_API_VERSION);

            [DllImport(myDllName)]
            internal extern static void cwipc_encoder_free(IntPtr enc);

            [DllImport(myDllName)]
            internal extern static void cwipc_encoder_close(IntPtr enc);

            [DllImport(myDllName)]
            internal extern static void cwipc_encoder_feed(IntPtr enc, IntPtr pc);

            [DllImport(myDllName)]
            internal extern static bool cwipc_encoder_available(IntPtr enc, bool wait);

            [DllImport(myDllName)]
            internal extern static bool cwipc_encoder_eof(IntPtr enc);

            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_encoder_get_encoded_size(IntPtr enc);

            [DllImport(myDllName)]
            internal extern static bool cwipc_encoder_copy_data(IntPtr enc, IntPtr data, IntPtr size);

            [DllImport(myDllName)]
            internal extern static bool cwipc_encoder_at_gop_boundary(IntPtr enc);

            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_new_encodergroup(ref IntPtr errorMessage, ulong apiVersion = _API_cwipc_util.CWIPC_API_VERSION);

            [DllImport(myDllName)]
            internal extern static void cwipc_encodergroup_free(IntPtr enc);

            [DllImport(myDllName)]
            internal extern static void cwipc_encodergroup_close(IntPtr enc);

            [DllImport(myDllName)]
            internal extern static IntPtr cwipc_encodergroup_addencoder(IntPtr enc, int paramVersion, ref encoder_params encParams, ref IntPtr errorMessage);

            [DllImport(myDllName)]
            internal extern static void cwipc_encodergroup_feed(IntPtr enc, IntPtr pc);

        }

        /// <summary>
        /// C# wrapper around a pointcloud.
        /// Pointcloud data is stored in a native buffer, to forestall copying when moving across inplementation language boundaries.
        /// </summary>
        public class pointcloud : BaseMemoryChunk
        {
            internal pointcloud(IntPtr _pointer) : base(_pointer)
            {
                if (_pointer == IntPtr.Zero)
                    throw new Exception("cwipc.pointcloud called with NULL pointer argument");
                // This is a hack. We copy the timestamp from the cwipc data to our info structure.
                info.timestamp = timestamp();
            }

            ~pointcloud()
            {
                free();
            }

            protected override void onfree()
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.pointcloud.onfree called with NULL pointer");
                _API_cwipc_util.cwipc_free(pointer);
            }

            /// <summary>
            /// Return timestamp of the pointcloud.
            /// </summary>
            /// <returns>millisecond timestamp</returns>
            public Timestamp timestamp()
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.pointcloud.timestamp called with NULL pointer");
                return _API_cwipc_util.cwipc_timestamp(pointer);
            }

            /// <summary>
            /// Set timestamp of the pointcloud. Internal use only.
            /// </summary>
            /// <param name="timestamp">timestamp in milliseconds</param>
            public void _set_timestamp(Timestamp timestamp)
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.pointcloud._set_timestamp called with NULL pointer");
                _API_cwipc_util.cwipc__set_timestamp(pointer, timestamp);
                info.timestamp = timestamp;
            }

            /// <summary>
            /// Returns number of points in the pointcloud.
            /// </summary>
            /// <returns>Number of points</returns>
            public int count()
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.pointcloud.count called with NULL pointer");
                return _API_cwipc_util.cwipc_count(pointer);

            }

            /// <summary>
            /// Return size of a single point. Each point conceptually occupies a cube with edges of this size.
            /// </summary>
            /// <returns>edge size (meters)</returns>
            public float cellsize()
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.pointcloud.cellsize called with NULL pointer");
                return _API_cwipc_util.cwipc_cellsize(pointer);
            }

            /// <summary>
            /// Set cellsize. Internal use only.
            /// </summary>
            /// <param name="cellsize">edge size in meters</param>
            public void _set_cellsize(float cellsize)
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.pointcloud._set_cellsize called with NULL pointer");
                _API_cwipc_util.cwipc__set_cellsize(pointer, cellsize);
            }

            /// <summary>
            /// Size of pointcloud data buffer.
            /// </summary>
            /// <returns>size in bytes</returns>
            public int get_uncompressed_size()
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.pointcloud.get_uncompressed_size called with NULL pointer");
                return (int)_API_cwipc_util.cwipc_get_uncompressed_size(pointer);
            }

            /// <summary>
            /// Copy pointcloud data buffer to another data buffer, as array of points.
            /// </summary>
            /// <param name="data">Native pointer to destination</param>
            /// <param name="size">size of <c>data</c> in bytes</param>
            /// <returns>number of points copied</returns>
            public int copy_uncompressed(IntPtr data, int size)
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.pointcloud.copy_uncompressed called with NULL pointer");
                return _API_cwipc_util.cwipc_copy_uncompressed(pointer, data, (IntPtr)size);
            }

            /// <summary>
            /// Copy pointcloud into a data buffer, in serialized form.
            /// Call with <c>data = 0</c> and <c>size = 0</c> to obtain data buffer size needed.
            /// <seealso cref="get_packet"/>
            /// </summary>
            /// <param name="data">Native pointer to destination</param>
            /// <param name="size">Size of <c>data</c> in bytes</param>
            /// <returns>Number of bytes copied</returns>
            public int copy_packet(IntPtr data, int size)
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.pointcloud.copy_uncompressed called with NULL pointer");
                return (int)_API_cwipc_util.cwipc_copy_packet(pointer, data, (IntPtr)size);
            }

            /// <summary>
            /// Return pointcloud in serialized form.
            /// </summary>
            /// <returns>Byte array with serialized pointcloud</returns>
            public byte[] get_packet()
            {
                int size = copy_packet(IntPtr.Zero, 0);
                byte[] rv = new byte[size];
                int actualSize = 0;
                unsafe
                {
                    fixed (byte* rvPtr = rv)
                    {
                        actualSize = copy_packet((IntPtr)rvPtr, size);
                    }
                }
                if (actualSize != size)
                {
                    throw new System.Exception($"cwipc.get_packet: size={actualSize} after promising {size}");
                }
                return rv;
            }

            internal IntPtr _intptr()
            {
                return pointer;
            }
        }

        /// <summary>
        /// Interface provided by pointcloud sources.
        /// </summary>
        public class source : BaseMemoryChunk
        {
            internal source(IntPtr _pointer) : base(_pointer)
            {
                if (_pointer == IntPtr.Zero) throw new Exception("cwipc.source called with NULL pointer argument");
            }

            protected override void onfree()
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.source.onfree called with NULL pointer");
                _API_cwipc_util.cwipc_source_free(pointer);
            }

            /// <summary>
            /// Obtain a new pointcloud.
            /// This is a non-blocking call, it will not wait for a pointcloud to become available.
            /// </summary>
            /// <returns>The pointcloud (or null if none could be produced)</returns>
            public pointcloud get()
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.source.get called with NULL pointer");
                IntPtr pc = _API_cwipc_util.cwipc_source_get(pointer);
                if (pc == IntPtr.Zero) return null;
                return new pointcloud(pc);
            }

            /// <summary>
            /// Test whether more pointclouds can ever be read from this source.
            /// </summary>
            /// <returns>True if at end of pointclouds</returns>
            public bool eof()
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.source.eof called with NULL pointer");
                return _API_cwipc_util.cwipc_source_eof(pointer);
            }

            /// <summary>
            /// Test whether a pointcloud can be read now.
            /// </summary>
            /// <param name="wait">True if the caller is willing to wait a short while for a pointcloud to become available</param>
            /// <returns>True if a subsequent call to <see cref="get"/> will return a valid pointcloud</returns>
            public bool available(bool wait)
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.source.available called with NULL pointer");
                return _API_cwipc_util.cwipc_source_available(pointer, wait);
            }

            /// <summary>
            /// Get the tiling information for a tiled pointcloud source.
            /// Returns null if the source produces untiled pointclouds.
            /// </summary>
            /// <returns>tile information array</returns>
            public tileinfo[] get_tileinfo()
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.source.get_tileinfo called with NULL pointer");
                int maxTile = _API_cwipc_util.cwipc_tiledsource_maxtile(pointer);
                if (maxTile == 0) return null;
                tileinfo[] rv = new tileinfo[maxTile];
                for (int i = 0; i < maxTile; i++)
                {
                    bool ok = _API_cwipc_util.cwipc_tiledsource_get_tileinfo(pointer, i, out rv[i]);
                }
                return rv;
            }
        }

        /// <summary>
        /// Pointcloud decoder.
        /// You feed data buffers with compressed pointcloud data into an object with this interface, and it then (asynchronously) produces pointclouds.
        /// </summary>
        public class decoder : source
        {
            internal decoder(IntPtr _obj) : base(_obj)
            {
                if (_obj == IntPtr.Zero) throw new Exception("cwipc.decoder: constructor called with null pointer");
            }

            /// <summary>
            /// Feed a compressed pointcloud databuffer into the decoder.
            /// </summary>
            /// <param name="compFrame">Native pointer to the data buffer</param>
            /// <param name="len">Size in bytes of <c>compFrame</c></param>
            public void feed(IntPtr compFrame, int len)
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.decoder.feed called with NULL pointer");
                _API_cwipc_codec.cwipc_decoder_feed(pointer, compFrame, len);
            }

            /// <summary>
            /// Signal that no more data buffers will be fed into this decoder.
            /// The <c>source.eof()</c> method will return <c>true</c> after the last pointcloud has been consumed.
            /// </summary>
            public void close()
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.decoder.close called with NULL pointer");
                _API_cwipc_codec.cwipc_decoder_close(pointer);
            }

        }

        /// <summary>
        /// Pointcloud encoder.
        /// You feed pointclouds into an object with this interface, and it then asynchronously produces compressed pointcloud data buffers.
        ///
        /// Note: these objects are thread-safe between producer and consumer, but <c>available()</c>,
        /// <c>get_encoded_size()</c> and <c>copy_data()</c> should be called from a single thread.
        /// 
        /// Note: you should not call the <c>get()</c> method on objects of this type.
        /// </summary>
        public class encoder : source
        {
            internal encoder(IntPtr _obj) : base(_obj)
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.encoder called with NULL pointer argument");
            }

            /* xxxjack need to check how this works with BaseMemoryChunk
                    ~encoder() {
                        free();
                    }
            */
            /// <summary>
            /// Feed a pointcloud into the compressor.
            /// The compressor will add a refcount on the native data buffer, so it is safe to delete to <c>pc</c> object after this call.
            /// </summary>
            /// <param name="pc">The pointcloud</param>
            public void feed(pointcloud pc)
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.encoder.feed called with NULL pointer argument");
                _API_cwipc_codec.cwipc_encoder_feed(pointer, pc.pointer);
            }

            /// <summary>
            /// Signal that no more pointclouds will be fed into this encoder.
            /// The <c>source.eof()</c> method will return <c>true</c> after the last data buffer has been consumed.
            /// </summary>
            public void close()
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.encoder.close called with NULL pointer argument");
                _API_cwipc_codec.cwipc_encoder_close(pointer);
            }

            /// <summary>
            /// Test whether more data buffers can ever be read from this source.
            /// </summary>
            /// <returns>True if at end of pointclouds</returns>
            public new bool eof()
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.encoder.eof called with NULL pointer argument");
                return _API_cwipc_codec.cwipc_encoder_eof(pointer);
            }

            /// <summary>
            /// Test whether a compressed pointcloud data buffer can be read now.
            /// </summary>
            /// <param name="wait">True if the caller is willing to wait a short while for a data buffer to become available</param>
            /// <returns>True if a data buffer is available</returns>
            new public bool available(bool wait)
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.encoder.available called with NULL pointer argument");
                return _API_cwipc_codec.cwipc_encoder_available(pointer, wait);
            }

            /// <summary>
            /// Returns the size of the native buffer that needs to be allocated to store the current compressed pointcloud data.
            /// </summary>
            /// <returns>Size in bytes</returns>
            public int get_encoded_size()
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.encoder.get_encoded_size called with NULL pointer argument");
                return (int)_API_cwipc_codec.cwipc_encoder_get_encoded_size(pointer);
            }

            /// <summary>
            /// Copy the compressed pointcloud data to a native buffer.
            /// </summary>
            /// <param name="data"></param>
            /// <param name="size"></param>
            /// <returns></returns>
            public bool copy_data(IntPtr data, int size)
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.encoder.copy_data called with NULL pointer argument");
                return _API_cwipc_codec.cwipc_encoder_copy_data(pointer, data, (IntPtr)size);
            }

            /// <summary>
            /// Returns true if the next data buffer returns is the start of a Group-of-pictures, for encoders that do inter-frame coding.
            /// Always return true for coders without inter-frame support.
            /// </summary>
            /// <returns>True when at a GOP boundary</returns>
            public bool at_gop_boundary()
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.encoder.at_gop_boundary called with NULL pointer argument");
                return _API_cwipc_codec.cwipc_encoder_at_gop_boundary(pointer);
            }

        }

        /// <summary>
        /// Interface to the feeder side of a group of encoders.
        /// Grouped encoders have a single procducer side (accepting pointclouds) and multiple consumer sides
        /// (producing compressed pointcloud data buffers). Grouped encoders are used for multi-quality encoding and
        /// tiled encoding of pointclouds.
        /// </summary>
        public class encodergroup : BaseMemoryChunk
        {
            internal encodergroup(IntPtr _obj) : base(_obj)
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.encodergroup called with NULL pointer argument");
            }

            /* xxxjack need to check how this works with BaseMemoryChunk
                    ~encodergroup() {
                        free();
                    }
            */
            /// <summary>
            /// Feed a pointcloud into the compressors.
            /// The compressors will add a refcount on the native data buffer, so it is safe to delete to <c>pc</c> object after this call.
            /// Filtering of the pointclouds (for tiling) and duplicating the pointclouds (for multi-quality compression) is done internally.
            /// </summary>
            /// <param name="pc">The pointcloud</param>
            public void feed(pointcloud pc)
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.encodergroup.feed called with NULL pointer argument");
                _API_cwipc_codec.cwipc_encodergroup_feed(pointer, pc.pointer);
            }

            /// <summary>
            /// Signal that no more pointclouds will be fed into this encoder.
            /// The <c>source.eof()</c> method will return <c>true</c> after the last data buffer has been consumed.
            /// </summary>
            public void close()
            {
                if (pointer == IntPtr.Zero) throw new Exception("cwipc.encodergroup.close called with NULL pointer argument");
                _API_cwipc_codec.cwipc_encodergroup_close(pointer);
            }

            /// <summary>
            /// Add another (new) encoder to this group.
            /// </summary>
            /// <param name="par">The parameters for the new encoder</param>
            /// <returns>The consumer side of the new encoder</returns>
            public encoder addencoder(encoder_params par)
            {
                IntPtr errorPtr = IntPtr.Zero;
                IntPtr enc = _API_cwipc_codec.cwipc_encodergroup_addencoder(pointer, _API_cwipc_codec.CWIPC_ENCODER_PARAM_VERSION, ref par, ref errorPtr);
                if (enc == IntPtr.Zero)
                {
                    if (errorPtr == IntPtr.Zero)
                    {
                        throw new Exception("cwipc.encodergroup.addencoder: returned null without setting error message");
                    }
                    throw new Exception($"cwipc_encoder_addencoder: {Marshal.PtrToStringAnsi(errorPtr)} ");
                }
                if (errorPtr != IntPtr.Zero)
                {
                    UnityEngine.Debug.LogError($"cwipc_encoder_addencoder: {Marshal.PtrToStringAnsi(errorPtr)}. Attempting to continue.");
                }
                return new encoder(enc);

            }
        }

        /// <summary>
        /// Return a synthetic source of pointclouds.
        /// The pointclouds produced have a roughly human size, and can be used for debugging.
        /// </summary>
        /// <param name="fps">Number of pointclouds produced per second</param>
        /// <param name="npoints">Number of points in each pointcloud</param>
        /// <returns></returns>
        public static source synthetic(int fps = 0, int npoints = 0)
        {
            IntPtr errorPtr = IntPtr.Zero;
            IntPtr rdr = IntPtr.Zero;
            try
            {
                rdr = _API_cwipc_util.cwipc_synthetic(fps, npoints, ref errorPtr);
            }
            catch (System.DllNotFoundException e)
            {
                UnityEngine.Debug.LogError($"cwipc.synthetic: cannot load cwipc_util DLL.");
                UnityEngine.Debug.LogError($"cwipc.synthetic: Exception: {e.ToString()}");
                UnityEngine.Debug.LogError($"cwipc.synthetic: see https://github.com/cwi-dis/cwipc for installation instructions.");
                throw new Exception("cwipc.synthetic: cwipc DLLs not installed correctly. See log.");
            }
            if (rdr == IntPtr.Zero)
            {
                if (errorPtr == IntPtr.Zero)
                {
                    throw new Exception("cwipc.synthetic: returned null without setting error message");
                }
                throw new Exception($"cwipc.synthetic: {Marshal.PtrToStringAnsi(errorPtr)} ");
            }
            if (errorPtr != IntPtr.Zero)
            {
                UnityEngine.Debug.LogError($"cwipc.synthetic: {Marshal.PtrToStringAnsi(errorPtr)}. Attempting to continue.");
            }
            return new source(rdr);
        }

        /// <summary>
        /// Produce pointclouds from a proxy RGBD camera.
        ///
        /// Note: this is a hack, don't use.
        /// </summary>
        /// <param name="ip">IP address of the proxy camera</param>
        /// <param name="port">TCP port of the proxy camera</param>
        /// <returns></returns>
        public static source proxy(string ip, int port)
        {
            IntPtr errorPtr = IntPtr.Zero;
            IntPtr rdr = IntPtr.Zero;
            try
            {
                rdr = _API_cwipc_util.cwipc_proxy(ip, port, ref errorPtr);
            }
            catch (System.DllNotFoundException e)
            {
                UnityEngine.Debug.LogError($"cwipc.proxy: cannot load cwipc_util DLL.");
                UnityEngine.Debug.LogError($"cwipc.proxy: Exception: {e.ToString()}");
                UnityEngine.Debug.LogError($"cwipc.proxy: see https://github.com/cwi-dis/cwipc for installation instructions.");
                throw new Exception("cwipc.proxy: cwipc DLLs not installed correctly. See log.");
            }
            if (rdr == IntPtr.Zero)
            {
                if (errorPtr == IntPtr.Zero)
                {
                    throw new Exception("cwipc.proxy: returned null without setting error message");
                }
                throw new Exception($"cwipc.proxy: {Marshal.PtrToStringAnsi(errorPtr)} ");
            }
            if (errorPtr != IntPtr.Zero)
            {
                UnityEngine.Debug.LogError($"cwipc.proxy: {Marshal.PtrToStringAnsi(errorPtr)}. Attempting to continue.");
            }
            return new source(rdr);
        }

        /// <summary>
        /// Produce pointclouds from one or more Intel RealSense2 cameras.
        ///
        /// For details on the configuration file, how to create it and how to calibrate the fusion of multiple cameras
        /// please see the documentation at <see href="http://github.com/cwi-dis/cwipc"/>
        /// </summary>
        /// <param name="filename">The <c>cameraconfig.xml</c> configuration file.</param>
        /// <returns>the pointcloud source</returns>
        public static source realsense2(string filename)
        {
            IntPtr errorPtr = IntPtr.Zero;
            IntPtr rdr = IntPtr.Zero;
            try
            {
                rdr = _API_cwipc_realsense2.cwipc_realsense2(filename, ref errorPtr);
            }
            catch (System.DllNotFoundException e)
            {
                UnityEngine.Debug.LogError($"cwipc.realsense2: cannot load cwipc_realsense2 DLL.");
                UnityEngine.Debug.LogError($"cwipc.realsense2: Exception: {e.ToString()}");
                UnityEngine.Debug.LogError($"cwipc.realsense2: see https://github.com/cwi-dis/cwipc for installation instructions.");
                throw new Exception("cwipc.realsense2: cwipc DLLs not installed correctly. See log.");
            }
            if (rdr == IntPtr.Zero)
            {
                if (errorPtr == IntPtr.Zero)
                {
                    throw new Exception("cwipc.realsense2: returned null without setting error message");
                }
                throw new Exception($"cwipc.realsense2: {Marshal.PtrToStringAnsi(errorPtr)} ");
            }
            if (errorPtr != IntPtr.Zero)
            {
                UnityEngine.Debug.LogError($"cwipc.realsense2: {Marshal.PtrToStringAnsi(errorPtr)}. Attempting to continue.");
            }
            return new source(rdr);
        }

        /// <summary>
        /// Produce pointclouds from one or more Microsoft Azure Kinect cameras.
        ///
        /// For details on the configuration file, how to create it and how to calibrate the fusion of multiple cameras
        /// please see the documentation at <see href="http://github.com/cwi-dis/cwipc"/>
        /// </summary>
        /// <param name="filename">The <c>cameraconfig.xml</c> configuration file.</param>
        /// <returns></returns>
        public static source kinect(string filename)
        {
            IntPtr errorPtr = IntPtr.Zero;
            IntPtr rdr = IntPtr.Zero;
            try
            {
                rdr = _API_cwipc_kinect.cwipc_kinect(filename, ref errorPtr);
            }
            catch (System.DllNotFoundException e)
            {
                UnityEngine.Debug.LogError($"cwipc.kinect: cannot load cwipc_kinect DLL.");
                UnityEngine.Debug.LogError($"cwipc.kinect: Exception: {e.ToString()}");
                UnityEngine.Debug.LogError($"cwipc.kinect: see https://github.com/cwi-dis/cwipc for installation instructions.");
                throw new Exception("cwipc.kinect: cwipc DLLs not installed correctly. See log.");
            }
          
            if (rdr == IntPtr.Zero)
            {
                if (errorPtr == IntPtr.Zero)
                {
                    throw new Exception("cwipc.kinect: returned null without setting error message");
                }
                throw new Exception($"cwipc.kinect: {Marshal.PtrToStringAnsi(errorPtr)} ");
            }
            if (errorPtr != IntPtr.Zero)
            {
                UnityEngine.Debug.LogError($"cwipc.kinect: {Marshal.PtrToStringAnsi(errorPtr)}. Attempting to continue.");
            }
            return new source(rdr);
        }

        /// <summary>
        /// Create a pointcloud decoder for the MPEG Anchor pointcloud codec.
        /// </summary>
        /// <returns>the decoder</returns>
        public static decoder new_decoder()
        {
            IntPtr errorPtr = IntPtr.Zero;
            IntPtr dec = IntPtr.Zero;
            try
            {
                dec = _API_cwipc_codec.cwipc_new_decoder(ref errorPtr);
            }
            catch (System.DllNotFoundException e)
            {
                UnityEngine.Debug.LogError($"cwipc.new_decoder: cannot load cwipc_codec DLL.");
                UnityEngine.Debug.LogError($"cwipc.new_decoder: Exception: {e.ToString()}");
                UnityEngine.Debug.LogError($"cwipc.new_decoder: see https://github.com/cwi-dis/cwipc for installation instructions.");
                throw new Exception("cwipc.new_decoder: cwipc DLLs not installed correctly. See log.");
            }
            if (dec == IntPtr.Zero)
            {
                if (errorPtr == IntPtr.Zero)
                {
                    throw new Exception("cwipc.new_decoder: returned null without setting error message");
                }
                throw new Exception($"cwipc_new_decoder: {Marshal.PtrToStringAnsi(errorPtr)} ");
            }
            if (errorPtr != IntPtr.Zero)
            {
                UnityEngine.Debug.LogError($"cwipc_new_decoder: {Marshal.PtrToStringAnsi(errorPtr)}. Attempting to continue.");
            }
            return new decoder(dec);

        }

        /// <summary>
        /// Create a pointcloud encoder for the MPEG Anchor pointcloud codec.
        /// </summary>
        /// <param name="par">The parameters for the encoder</param>
        /// <returns></returns>
        public static encoder new_encoder(encoder_params par)
        {
            IntPtr errorPtr = IntPtr.Zero;
            IntPtr enc = IntPtr.Zero;
            try
            {
                enc = _API_cwipc_codec.cwipc_new_encoder(_API_cwipc_codec.CWIPC_ENCODER_PARAM_VERSION, ref par, ref errorPtr);
            }
            catch (System.DllNotFoundException e)
            {
                UnityEngine.Debug.LogError($"cwipc.new_encoder: cannot load cwipc_codec DLL.");
                UnityEngine.Debug.LogError($"cwipc.new_encoder: Exception: {e.ToString()}");
                UnityEngine.Debug.LogError($"cwipc.new_encoder: see https://github.com/cwi-dis/cwipc for installation instructions.");
                throw new Exception("cwipc.new_encoder: cwipc DLLs not installed correctly. See log.");
            }
            if (enc == IntPtr.Zero)
            {
                if (errorPtr == IntPtr.Zero)
                {
                    throw new Exception("cwipc.new_encoder: returned null without setting error message");
                }
                throw new Exception($"cwipc_new_encoder: {Marshal.PtrToStringAnsi(errorPtr)} ");
            }
            if (errorPtr != IntPtr.Zero)
            {
                UnityEngine.Debug.LogError($"cwipc_new_encoder: {Marshal.PtrToStringAnsi(errorPtr)}. Attempting to continue.");
            }
            return new encoder(enc);

        }

        /// <summary>
        /// Create a new pointcloud encoder group for the MPEG Anchor pointcloud codec.
        /// The group is initially empty, you add encoders with the <see cref="add_encoder"/> method.
        /// </summary>
        /// <returns>an empty encodergroup</returns>
        public static encodergroup new_encodergroup()
        {
            IntPtr errorPtr = IntPtr.Zero;
            IntPtr enc = IntPtr.Zero;
            try
            {
                enc = _API_cwipc_codec.cwipc_new_encodergroup(ref errorPtr);
            }
            catch (System.DllNotFoundException e)
            {
                UnityEngine.Debug.LogError($"cwipc.new_encodergroup: cannot load cwipc_codec DLL.");
                UnityEngine.Debug.LogError($"cwipc.new_encodergroup: Exception: {e.ToString()}");
                UnityEngine.Debug.LogError($"cwipc.new_encodergroup: see https://github.com/cwi-dis/cwipc for installation instructions.");
                throw new Exception("cwipc.new_encodergroup: cwipc DLLs not installed correctly. See log.");
            }
            if (enc == IntPtr.Zero)
            {
                if (errorPtr == IntPtr.Zero)
                {
                    throw new Exception("cwipc.new_encodergroup: returned null without setting error message");
                }
                throw new Exception($"cwipc_new_encodergroup: {Marshal.PtrToStringAnsi(errorPtr)} ");
            }
            if (errorPtr != IntPtr.Zero)
            {
                UnityEngine.Debug.LogError($"cwipc_new_encodergroup: {Marshal.PtrToStringAnsi(errorPtr)}. Attempting to continue.");
            }
            return new encodergroup(enc);

        }

        /// <summary>
        /// Downsample a pointcloud.
        /// Returns a new pointcloud with possibly fewer points and a larger cellsize.
        ///
        /// Note: this method is currently not very efficient for large pointclouds.
        /// </summary>
        /// <param name="pc">The source pointcloud</param>
        /// <param name="voxelSize">Voxel size (in meters)</param>
        /// <returns>A new downsampled pointcloud</returns>
        public static pointcloud downsample(pointcloud pc, float voxelSize)
        {
            IntPtr pcPtr = pc._intptr();
            IntPtr rvPtr = _API_cwipc_util.cwipc_downsample(pcPtr, voxelSize);
            if (rvPtr == IntPtr.Zero) return null;
            return new pointcloud(rvPtr);
        }

        /// <summary>
        /// Filter a single tile out of a pointcloud.
        /// Returns a new pointcloud with only the points belonging to the given tile.
        /// </summary>
        /// <param name="pc">The source pointcloud</param>
        /// <param name="tileNum">The wanted tile number</param>
        /// <returns>A new pointcloud</returns>
        public static pointcloud tilefilter(pointcloud pc, int tileNum)
        {
            IntPtr pcPtr = pc._intptr();
            IntPtr rvPtr = _API_cwipc_util.cwipc_tilefilter(pcPtr, tileNum);
            if (rvPtr == IntPtr.Zero) return null;
            return new pointcloud(rvPtr);
        }

        /// <summary>
        /// Renumber or combine tiles.
        /// The tile number for each point is looked up in the map and replaced by the
        /// resulting tile number.
        /// </summary>
        /// <param name="pc">Source pointcloud</param>
        /// <param name="map">Tile number mapping</param>
        /// <returns>A new pointcloud</returns>
        public static pointcloud tilemap(pointcloud pc, byte[] map)
        {
            IntPtr pcPtr = pc._intptr();
            IntPtr rvPtr = _API_cwipc_util.cwipc_tilemap(pcPtr, map);
            if (rvPtr == IntPtr.Zero) return null;
            return new pointcloud(rvPtr);
        }

        /// <summary>
        /// Change colors of the points in a pointcloud.
        /// Bits in each color are set and cleared according to the masks.
        /// Note: this is mainly a debugging method, to allow some visual color
        /// feedback.
        /// </summary>
        /// <param name="pc">Source pointcloud</param>
        /// <param name="clearMask">32 bit mask with bits to clear</param>
        /// <param name="setMask">32 bit mask with bits to set</param>
        /// <returns>A new pointcloud</returns>
        public static pointcloud colormap(pointcloud pc, uint clearMask, uint setMask)
        {
            IntPtr pcPtr = pc._intptr();
            IntPtr rvPtr = _API_cwipc_util.cwipc_colormap(pcPtr, clearMask, setMask);
            if (rvPtr == IntPtr.Zero) return null;
            return new pointcloud(rvPtr);
        }

        /// <summary>
        /// Crop a pointcloud to a bounding cube.
        /// A new pointcloud is returned containing only the points that fall within the
        /// bounding box.
        /// </summary>
        /// <param name="pc">Source pointcloud</param>
        /// <param name="bbox">6 floats specifying the cube</param>
        /// <returns>A new pointcloud</returns>
        public static pointcloud crop(pointcloud pc, float[] bbox)
        {
            IntPtr pcPtr = pc._intptr();
            IntPtr rvPtr = _API_cwipc_util.cwipc_crop(pcPtr, bbox);
            if (rvPtr == IntPtr.Zero) return null;
            return new pointcloud(rvPtr);
        }

        /// <summary>
        /// Combine two pointclouds.
        /// Returns a pointcloud that is the union of two pointclouds.
        /// The metadata is taken from the first pointcloud.
        /// </summary>
        /// <param name="pc1">Source 1</param>
        /// <param name="pc2">Source 2</param>
        /// <returns>A new pointcloud</returns>
        public static pointcloud join(pointcloud pc1, pointcloud pc2)
        {
            IntPtr pc1Ptr = pc1._intptr();
            IntPtr pc2Ptr = pc2._intptr();
            IntPtr rvPtr = _API_cwipc_util.cwipc_join(pc1Ptr, pc2Ptr);
            if (rvPtr == IntPtr.Zero) return null;
            return new pointcloud(rvPtr);
        }

        /// <summary>
        /// Legacy method.
        /// </summary>
        /// <param name="certhPC"></param>
        /// <param name="move"></param>
        /// <param name="bbox"></param>
        /// <param name="timestamp"></param>
        /// <returns></returns>
        public static pointcloud from_certh(IntPtr certhPC, float[] move, float[] bbox, Timestamp timestamp)
        {
            IntPtr errorPtr = IntPtr.Zero;
            // Need to pass origin and bbox as array pointers.
            IntPtr rvPtr = IntPtr.Zero;
            try
            {
                rvPtr = _API_cwipc_util.cwipc_from_certh(certhPC, move, bbox, timestamp, ref errorPtr);
            }
            catch (System.DllNotFoundException e)
            {
                UnityEngine.Debug.LogError($"cwipc.from_certh: cannot load cwipc_util DLL.");
                UnityEngine.Debug.LogError($"cwipc.from_certh: Exception: {e.ToString()}");
                UnityEngine.Debug.LogError($"cwipc.from_certh: see https://github.com/cwi-dis/cwipc for installation instructions.");
                throw new Exception("cwipc.from_certh: cwipc DLLs not installed correctly. See log.");
            }
            if (rvPtr == IntPtr.Zero)
            {
                if (errorPtr == IntPtr.Zero)
                {
                    throw new Exception("cwipc.from_certh: returned null without setting error message");
                }
                throw new Exception($"cwipc_from_certh: {Marshal.PtrToStringAnsi(errorPtr)} ");
            }
            if (errorPtr != IntPtr.Zero)
            {
                UnityEngine.Debug.LogError($"cwipc_from_certh: {Marshal.PtrToStringAnsi(errorPtr)}. Attempting to continue.");
            }
            return new pointcloud(rvPtr);
        }

        /// <summary>
        /// Read a pointcloud from a <c>.ply</c> file.
        /// As plyfiles do not contain a timestamp you pass in the timestamp on the call.
        /// </summary>
        /// <param name="filename">The file to read</param>
        /// <param name="timestamp"></param>
        /// <returns>The pointcloud read</returns>
        public static pointcloud read(string filename, Timestamp timestamp)
        {
            System.IntPtr errorPtr = System.IntPtr.Zero;
            System.IntPtr rvPtr = IntPtr.Zero;
            try
            {
                rvPtr = _API_cwipc_util.cwipc_read(filename, timestamp, ref errorPtr);
            }
            catch (System.DllNotFoundException e)
            {
                UnityEngine.Debug.LogError($"cwipc.read: cannot load cwipc_util DLL.");
                UnityEngine.Debug.LogError($"cwipc.read: Exception: {e.ToString()}");
                UnityEngine.Debug.LogError($"cwipc.read: see https://github.com/cwi-dis/cwipc for installation instructions.");
                throw new Exception("cwipc.read: cwipc DLLs not installed correctly. See log.");
            }
            if (rvPtr == System.IntPtr.Zero)
            {
                if (errorPtr == System.IntPtr.Zero)
                {
                    throw new System.Exception("cwipc.read: returned null without setting error message");
                }
                throw new System.Exception($"cwipc_read: {System.Runtime.InteropServices.Marshal.PtrToStringAnsi(errorPtr)} ");
            }
            return new pointcloud(rvPtr);
        }

        /// <summary>
        /// Read a pointcloud from a <c>.cwipcdump</c> file.
        /// This is a file format that closely follows the in-memory layout of cwipc pointclouds, and can therefore
        /// be read and written relatively fast.
        /// </summary>
        /// <param name="filename">The file to read</param>
        /// <returns>The pointcloud read</returns>
        public static pointcloud readdump(string filename)
        {
            System.IntPtr errorPtr = System.IntPtr.Zero;
            System.IntPtr rvPtr = IntPtr.Zero;
            try
            {
                rvPtr = _API_cwipc_util.cwipc_read_debugdump(filename, ref errorPtr);
            }
            catch (System.DllNotFoundException e)
            {
                UnityEngine.Debug.LogError($"cwipc.readdump: cannot load cwipc_util DLL.");
                UnityEngine.Debug.LogError($"cwipc.readdump: Exception: {e.ToString()}");
                UnityEngine.Debug.LogError($"cwipc.readdump: see https://github.com/cwi-dis/cwipc for installation instructions.");
                throw new Exception("cwipc.readdump: cwipc DLLs not installed correctly. See log.");
            }
            if (rvPtr == System.IntPtr.Zero)
            {
                if (errorPtr == System.IntPtr.Zero)
                {
                    throw new System.Exception("cwipc.read: returned null without setting error message");
                }
                throw new System.Exception($"cwipc_read: {System.Runtime.InteropServices.Marshal.PtrToStringAnsi(errorPtr)} ");
            }
            return new pointcloud(rvPtr);
        }

        /// <summary>
        /// Read a pointcloud from a serialized data buffer.
        /// This is a file format that closely follows the in-memory layout of cwipc pointclouds, and can therefore
        /// be read and written relatively fast.
        /// </summary>
        /// <param name="packet">The native data buffer pointer</param>
        /// <param name="size">Buffer size in bytes</param>
        /// <returns>The pointcloud read</returns>
        /// <seealso cref="pointcloud.get_packet"/>
        public static pointcloud from_packet(IntPtr packet, IntPtr size)
        {
            System.IntPtr errorPtr = System.IntPtr.Zero;
            System.IntPtr rvPtr = IntPtr.Zero;
            try
            {
                rvPtr = _API_cwipc_util.cwipc_from_packet(packet, size, ref errorPtr);
            }
            catch (System.DllNotFoundException e)
            {
                UnityEngine.Debug.LogError($"cwipc.from_packet: cannot load cwipc_util DLL.");
                UnityEngine.Debug.LogError($"cwipc.from_packet: Exception: {e.ToString()}");
                UnityEngine.Debug.LogError($"cwipc.from_packet: see https://github.com/cwi-dis/cwipc for installation instructions.");
                throw new Exception("cwipc.from_packet: cwipc DLLs not installed correctly. See log.");
            }
            if (rvPtr == System.IntPtr.Zero)
            {
                if (errorPtr == System.IntPtr.Zero)
                {
                    throw new System.Exception("cwipc.from_packet: returned null without setting error message");
                }
                throw new System.Exception($"cwipc_from_packet: {System.Runtime.InteropServices.Marshal.PtrToStringAnsi(errorPtr)} ");
            }
            return new pointcloud(rvPtr);
        }

        /// <summary>
        /// Read a pointcloud from a serialized data buffer.
        /// This is a file format that closely follows the in-memory layout of cwipc pointclouds, and can therefore
        /// be read and written relatively fast.
        /// </summary>
        /// <param name="packet">C# byte array with packet data</param>
        /// <returns>The pointcloud read</returns>
        public static pointcloud from_packet(byte[] packet)
        {
            IntPtr size = (IntPtr)packet.Length;
            pointcloud rv = null;
            unsafe
            {
                fixed(byte *packetPtr = packet)
                {
                    rv = from_packet((IntPtr)packetPtr, size);
                }
            }
            return rv;
        }
    }
}