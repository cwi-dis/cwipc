import unittest
import cwipc
import cwipc.playback
import os
import sys
import tempfile
import struct

#
# Find directories for test inputs and outputs
#
_thisdir=os.path.dirname(__file__)
_topdir=os.path.dirname(_thisdir)
TEST_FIXTURES_DIR=os.path.join(_topdir, "tests", "fixtures")
TEST_OUTPUT_DIR=os.path.join(TEST_FIXTURES_DIR, "output")
if not os.access(TEST_OUTPUT_DIR, os.W_OK):
    TEST_OUTPUT_DIR=tempfile.mkdtemp('cwipc_util_test') # type: ignore
PLY_DIRNAME=os.path.join(TEST_FIXTURES_DIR, "input")
PLY_FILENAME=os.path.join(PLY_DIRNAME, "pcl_frame1.ply")

class TestApi(unittest.TestCase):

    def test_point(self):
        """Can a cwipc_point be created and the values read back?"""
        p = cwipc.cwipc_point(1, 2, 3, 0x10, 0x20, 0x30, 0)
        self.assertEqual(p.x, 1)
        self.assertEqual(p.y, 2)
        self.assertEqual(p.z, 3)
        self.assertEqual(p.r, 0x10)
        self.assertEqual(p.g, 0x20)
        self.assertEqual(p.b, 0x30)
        
    def test_pointarray(self):
        """Can an empty cwipc_point array be created and the values read back?"""
        p = cwipc.cwipc_point_array(count=10)
        self.assertEqual(p[0].x, 0)
        self.assertEqual(p[0].y, 0)
        self.assertEqual(p[0].z, 0)
        self.assertEqual(p[0].r, 0)
        self.assertEqual(p[0].g, 0)
        self.assertEqual(p[0].b, 0)
        self.assertEqual(p[9].x, 0)
        self.assertEqual(p[9].y, 0)
        self.assertEqual(p[9].z, 0)
        self.assertEqual(p[9].r, 0)
        self.assertEqual(p[9].g, 0)
        self.assertEqual(p[9].b, 0)
        with self.assertRaises(IndexError):
            p[10].x
                    
    def test_pointarray_filled(self):
        """Can a cwipc_point array be created with values and the values read back?"""
        p = cwipc.cwipc_point_array(values=[(1, 2, 3, 0x10, 0x20, 0x30, 0), (4, 5, 6, 0x40, 0x50, 0x60, 0)])
        self.assertEqual(len(p), 2)
        self.assertEqual(p[0].x, 1)
        self.assertEqual(p[0].y, 2)
        self.assertEqual(p[0].z, 3)
        self.assertEqual(p[0].r, 0x10)
        self.assertEqual(p[0].g, 0x20)
        self.assertEqual(p[0].b, 0x30)
        self.assertEqual(p[1].x, 4)
        self.assertEqual(p[1].y, 5)
        self.assertEqual(p[1].z, 6)
        self.assertEqual(p[1].r, 0x40)
        self.assertEqual(p[1].g, 0x50)
        self.assertEqual(p[1].b, 0x60)
        with self.assertRaises(IndexError):
            p[2].x
        
    def test_cwipc(self):
        """Can we create and free a cwipc object"""
        pc = cwipc.cwipc_pointcloud_wrapper()
        del pc
        
    def test_cwipc_source(self):
        """Can we create and free a cwipc_source object"""
        pcs = cwipc.cwipc_source_wrapper()
        del pcs
    
    def test_cwipc_from_points_empty(self):
        """Can we create a cwipc object from a empty list of values"""
        points = cwipc.cwipc_point_array(values=[])
        pc = cwipc.cwipc_from_points(points, 0)
        newpoints = pc.get_points()
        self.assertEqual(len(points), 0)
        self.assertEqual(len(newpoints), 0)
        
    def test_cwipc_from_points(self):
        """Can we create a cwipc object from a list of values"""
        points = cwipc.cwipc_point_array(values=[(1, 2, 3, 0x10, 0x20, 0x30, 1), (4, 5, 6, 0x40, 0x50, 0x60, 2)])
        pc = cwipc.cwipc_from_points(points, 0)
        self.assertEqual(pc.count(), len(points))
        newpoints = pc.get_points()
        self.assertEqual(len(points), len(newpoints))
        for i in range(len(points)):
            op = points[i]
            np = newpoints[i]
            self.assertEqual(op.x, np.x)
            self.assertEqual(op.y, np.y)
            self.assertEqual(op.z, np.z)
            self.assertEqual(op.r, np.r)
            self.assertEqual(op.g, np.g)
            self.assertEqual(op.b, np.b)
            self.assertEqual(op.tile, np.tile)
        
    def test_cwipc_numpy_array(self):
        """Can we round-trip between a cwipc and a numpy record array"""
        # Create cwipc from Python list of points.
        points = cwipc.cwipc_point_array(values=[(1, 2, 3, 0x10, 0x20, 0x30, 1), (4, 5, 6, 0x40, 0x50, 0x60, 2)])
        pc = cwipc.cwipc_from_points(points, 0)
        self.assertEqual(pc.count(), len(points))
        # Get the points as a NumPy float matrix
        np_array = pc.get_numpy_array()
        self.assertEqual(np_array.shape[0], pc.count())
        # Create a new pointcloud from the record array
        new_pc = cwipc.cwipc_from_numpy_array(np_array, 0)
        # Get the points from the new pointcloud
        newpoints = new_pc.get_points()
        self.assertEqual(len(points), len(newpoints))
        for i in range(len(points)):
            op = points[i]
            np = newpoints[i]
            self.assertEqual(op.x, np.x)
            self.assertEqual(op.y, np.y)
            self.assertEqual(op.z, np.z)
            self.assertEqual(op.r, np.r)
            self.assertEqual(op.g, np.g)
            self.assertEqual(op.b, np.b)
            self.assertEqual(op.tile, np.tile)

    def test_cwipc_numpy_matrix(self):
        """Can we round-trip between a cwipc and a numpy matrix of floats representing the points"""
        # Create cwipc from Python list of points.
        points = cwipc.cwipc_point_array(values=[(1, 2, 3, 0x10, 0x20, 0x30, 1), (4, 5, 6, 0x40, 0x50, 0x60, 2)])
        pc = cwipc.cwipc_from_points(points, 0)
        self.assertEqual(pc.count(), len(points))
        # Get the points as a NumPy float matrix
        np_matrix = pc.get_numpy_matrix()
        self.assertEqual(np_matrix.shape[0], pc.count())
        self.assertEqual(np_matrix.shape[1], 7)
        # Create a new pointcloud from the record array
        new_pc = cwipc.cwipc_from_numpy_matrix(np_matrix, 0)
        # Get the points from the new pointcloud
        newpoints = new_pc.get_points()
        self.assertEqual(len(points), len(newpoints))
        for i in range(len(points)):
            op = points[i]
            np = newpoints[i]
            self.assertEqual(op.x, np.x)
            self.assertEqual(op.y, np.y)
            self.assertEqual(op.z, np.z)
            self.assertEqual(op.r, np.r)
            self.assertEqual(op.g, np.g)
            self.assertEqual(op.b, np.b)
            self.assertEqual(op.tile, np.tile)

    def test_cwipc_o3d_pointcloud(self):
        """Can we round-trip between a cwipc and an open3d point cloud"""
        # Create cwipc from Python list of points.
        points = cwipc.cwipc_point_array(values=[(1, 2, 3, 0x10, 0x20, 0x30, 1), (4, 5, 6, 0x40, 0x50, 0x60, 2)])
        pc = cwipc.cwipc_from_points(points, 0)
        self.assertEqual(pc.count(), len(points))
        # Get the pointcloud as an open3d point cloud (except tile)
        o3d_pc = pc.get_o3d_pointcloud()
        # xxxjack self.assertEqual(o3d_pc.num_points, pc.count())
        # Create a new pointcloud from the o3d pointcloud
        new_pc = cwipc.cwipc_from_o3d_pointcloud(o3d_pc, 0)
        # Get the points from the new pointcloud
        newpoints = new_pc.get_points()
        self.assertEqual(len(points), len(newpoints))
        for i in range(len(points)):
            op = points[i]
            np = newpoints[i]
            self.assertEqual(op.x, np.x)
            self.assertEqual(op.y, np.y)
            self.assertEqual(op.z, np.z)
            self.assertEqual(op.r, np.r)
            self.assertEqual(op.g, np.g)
            self.assertEqual(op.b, np.b)
            # Not round tripped: self.assertEqual(op.tile, np.tile)

    def test_cwipc_timestamp_cellsize(self):
        """Can we set and retrieve the timestamp and cellsize in a cwipc"""
        timestamp = 0x11223344556677
        pc = cwipc.cwipc_from_points([(0,0,0,0,0,0,1), (1,0,0,0,0,0,1), (2,0,0,0,0,0,1), (3,0,0,0,0,0,1)], timestamp)
        self.assertEqual(pc.timestamp(), timestamp)
        pc._set_timestamp(timestamp+1) # type: ignore
        self.assertEqual(pc.timestamp(), timestamp+1)
        self.assertEqual(pc.cellsize(), 0)
        pc._set_cellsize(0.1) # type: ignore
        self.assertAlmostEqual(pc.cellsize(), 0.1)
        pc._set_cellsize(-1) # type: ignore
        self.assertAlmostEqual(pc.cellsize(), 1.0)

    def test_cwipc_read(self):
        """Can we read a cwipc from a ply file?"""
        pc = cwipc.cwipc_read(PLY_FILENAME, 1234)
        self.assertEqual(pc.timestamp(), 1234)
        self._verify_pointcloud(pc)

    def test_cwipc_dangling_allocations(self):
        """Does the dangling allocation counter work as expected"""
        old_count = cwipc.cwipc_dangling_allocations(True)
        pc = cwipc.cwipc_read(PLY_FILENAME, 1234)
        new_count = cwipc.cwipc_dangling_allocations(True)
        self.assertEqual(old_count+1, new_count)
        pc = None
        newer_count = cwipc.cwipc_dangling_allocations(True)
        self.assertEqual(old_count, newer_count)

    def test_cwipc_clone(self):
        """Does cloning a point cloud work"""
        old_count = cwipc.cwipc_dangling_allocations(False)
        pc = cwipc.cwipc_read(PLY_FILENAME, 1234)
        new_pc = pc.clone()
        int_count = cwipc.cwipc_dangling_allocations(False)
        self.assertEqual(old_count+2, int_count)
        self.assertEqual(pc.count(), new_pc.count())
        self.assertEqual(pc.timestamp(), new_pc.timestamp())
        pc = None
        new_pc = None
        new_count = cwipc.cwipc_dangling_allocations(False)
        self.assertEqual(old_count, new_count)

    def test_cwipc_read_nonexistent(self):
        """When we read a cwipc from a nonexistent ply file do we get an exception?"""
        with self.assertRaises(cwipc.CwipcError):
            pc = cwipc.cwipc_read(PLY_FILENAME + '.nonexistent', 1234) # type: ignore
    
    def test_cwipc_write(self):
        """Can we write a cwipc to a ply file and read it back?"""
        pc = self._build_pointcloud()
        filename = os.path.join(TEST_OUTPUT_DIR, 'test_cwipc_write.ply')
        cwipc.cwipc_write(filename, pc)
        pc2 = cwipc.cwipc_read(filename, 0)
        self.assertEqual(list(pc.get_points()), list(pc2.get_points()))
    
    def test_cwipc_write_binary(self):
        """Can we write a cwipc to a ply file and read it back?"""
        pc = self._build_pointcloud()
        filename = os.path.join(TEST_OUTPUT_DIR, 'test_cwipc_write.ply')
        cwipc.cwipc_write(filename, pc, cwipc.CWIPC_FLAGS_BINARY)
        pc2 = cwipc.cwipc_read(filename, 0)
        self.assertEqual(list(pc.get_points()), list(pc2.get_points()))
        
    def test_cwipc_write_nonexistent(self):
        """When we write a cwipc from a nonexistent ply file do we get an exception?"""
        pc = self._build_pointcloud()
        with self.assertRaises(cwipc.CwipcError):
            pc = cwipc.cwipc_write(os.path.join(PLY_FILENAME, 'non', 'existent'), pc)

    def test_cwipc_write_debugdump(self):
        """Can we write a cwipc to a debugdump file and read it back?"""
        pc = self._build_pointcloud()
        filename = os.path.join(TEST_OUTPUT_DIR, 'test_cwipc_write_debugdump.cwipcdump')
        cwipc.cwipc_write_debugdump(filename, pc)
        pc2 = cwipc.cwipc_read_debugdump(filename)
        self.assertEqual(list(pc.get_points()), list(pc2.get_points()))
    
    def test_cwipc_write_debugdump_nonexistent(self):
        """When we write a cwipc from a nonexistent ply file do we get an exception?"""
        pc = self._build_pointcloud()
        filename = os.path.join(TEST_OUTPUT_DIR, 'test_cwipc_write_debugdump_nonexistent.cwipcdump', 'non', 'existent')
        with self.assertRaises(cwipc.CwipcError):
            cwipc.cwipc_write_debugdump(filename, pc)

    def test_cwipc_packet(self):
        """Test cwipc_copy_packet and cwipc_from_packet"""
        pc = self._build_pointcloud()
        packet = pc.get_packet()
        pc2 = cwipc.cwipc_from_packet(packet)
        self.assertEqual(pc.timestamp(), pc2.timestamp())
        self.assertEqual(pc.cellsize(), pc2.cellsize())
        points = pc.get_points()
        points2 = pc2.get_points()
        self.assertEqual(len(points), len(points2))
        for i in range(len(points)):
            op = points[i]
            np = points2[i]
            self.assertEqual(op.x, np.x)
            self.assertEqual(op.y, np.y)
            self.assertEqual(op.z, np.z)
            self.assertEqual(op.r, np.r)
            self.assertEqual(op.g, np.g)
            self.assertEqual(op.b, np.b)
            self.assertEqual(op.tile, np.tile)
        packet2 = pc2.get_packet()
        self.assertEqual(packet, packet2)

    def test_cwipc_logger(self):
        """Can we configure the cwipc logger and receive log messages?"""
        self.log_messages = []
        def log_callback(level: int, message: bytes) -> None:
            self.log_messages.append( (level, message.decode('utf8')) )
        cwipc.cwipc_log_configure(cwipc.CWIPC_LOG_LEVEL_DEBUG, log_callback)
        # Emit a test log message
        cwipc._cwipc_log_emit(cwipc.CWIPC_LOG_LEVEL_DEBUG, "test_module", "This is a test log message")
        self.assertGreaterEqual(len(self.log_messages), 1)
        found = False
        for level, message in self.log_messages:
            if  "This is a test log message" in message:
                found = True
                self.assertEqual(level, cwipc.CWIPC_LOG_LEVEL_DEBUG)
        self.assertTrue(found)
        # Reset logging to default
        cwipc.cwipc_log_configure(cwipc.CWIPC_LOG_LEVEL_WARNING, None)

    def test_cwipc_synthetic(self):
        """Can we create a synthetic pointcloud?"""
        pcs = cwipc.cwipc_synthetic()
        didStart = pcs.start()
        self.assertTrue(didStart)
        self.assertTrue(pcs.available(True))
        self.assertFalse(pcs.eof())
        pc = pcs.get()
        self.assertIsNotNone(pc)
        assert pc # Only to keep linters happy
        self._verify_pointcloud(pc)
        pcs.stop()
        
    def test_cwipc_synthetic_available_false(self):
        """Does the synthetic reader implement available(False) correctly?"""
        pcs = cwipc.cwipc_synthetic(5)
        didStart = pcs.start()
        self.assertTrue(didStart)
        self.assertTrue(pcs.available(True))
        pc = pcs.get()
        self.assertFalse(pcs.available(False))
        assert pc # Only to keep linters happy
        pcs.stop()
        
    def test_cwipc_synthetic_nonexistent_metadata(self):
        """If we request nonexistent metadata on a cwipc_source do we get an error?"""
        pcs = cwipc.cwipc_synthetic()
        wantUnknown = pcs.is_metadata_requested("nonexistent-metadata")
        self.assertFalse(wantUnknown)
        pcs.request_metadata("nonexistent-metadata")
        wantUnknown = pcs.is_metadata_requested("nonexistent-metadata")
        self.assertTrue(wantUnknown)
        pcs.stop()
    
    def test_cwipc_synthetic_metadata(self):
        """Can we request metadata on a cwipc_source"""
        pcs = cwipc.cwipc_synthetic()
        pcs.request_metadata("test-angle")
        wantTestAngle = pcs.is_metadata_requested("test-angle")
        self.assertTrue(wantTestAngle)
        didStart = pcs.start()
        self.assertTrue(didStart)
        pc = pcs.get()
        self.assertIsNotNone(pc)
        assert pc # Only to keep linters happy
        ap = pc.access_metadata()
        self.assertIsNotNone(ap)
        assert ap # Only to keep linters happy
        self.assertEqual(ap.count(), 1)
        self.assertEqual(ap.name(0), "test-angle")
        self.assertEqual(ap.description(0), "")
        self.assertEqual(ap.size(0), 4) # sizeof m_angle
        data = ap.data(0)
        self.assertEqual(len(data), 4)
        # We can no longer assume the angle is nonzero (since adding the start() call to cwipc_source).
        # self.assertNotEqual(data, b'\0\0\0\0')
        pcs.stop()

    def test_cwipc_synthetic_nonexistent_auxiliary_operation(self):
        """If we request a nonexistent auxiliary operation on a cwipc_source do we get an error?"""
        pcs = cwipc.cwipc_synthetic()
        didStart = pcs.start()
        self.assertTrue(didStart)
        inbuf = bytes()
        outbuf = bytearray(4)
        wantUnknown = pcs.auxiliary_operation("nonexistent-auxop", inbuf, outbuf)
        self.assertFalse(wantUnknown)
        pcs.stop()
    
    def test_cwipc_synthetic_auxiliary_operation(self):
        """Can we request an auxiliary operation on a cwipc_source"""
        pcs = cwipc.cwipc_synthetic()
        didStart = pcs.start()
        self.assertTrue(didStart)
        angle = 42.0
        inbuf = struct.pack("f", angle)
        outbuf = bytearray(struct.pack("f", 0))
        ok = pcs.auxiliary_operation("test-setangle", inbuf, outbuf)
        self.assertTrue(ok)
        newAngle, = struct.unpack("f", outbuf)
        self.assertEqual(angle, newAngle)
        pcs.stop()

    def test_cwipc_synthetic_args(self):
        """Can we create a synthetic pointcloud with fps and npoints arguments?"""
        pcs = cwipc.cwipc_synthetic(10, 1000)
        didStart = pcs.start()
        self.assertTrue(didStart)
        self.assertTrue(pcs.available(True))
        # not always true: self.assertTrue(pcs.available(False))
        self.assertFalse(pcs.eof())
        pc = pcs.get()
        self.assertIsNotNone(pc)
        assert pc # Only to keep linters happy
        self._verify_pointcloud(pc)
        pcs.stop()
        
    def test_cwipc_synthetic_tiled(self):
        """Is a synthetic pointcloud generator providing the correct tiling interface?"""
        pcs = cwipc.cwipc_synthetic()
        self.assertEqual(pcs.maxtile(), 3)
        self.assertEqual(pcs.get_tileinfo_dict(0), {'normal':{'x':0, 'y':0, 'z':0},'cameraName':b'synthetic', 'ncamera':2, 'cameraMask':0})
        self.assertEqual(pcs.get_tileinfo_dict(1), {'normal':{'x':0, 'y':0, 'z':1},'cameraName':b'synthetic-right', 'ncamera':1, 'cameraMask':1})
        self.assertEqual(pcs.get_tileinfo_dict(2), {'normal':{'x':0, 'y':0, 'z':-1},'cameraName':b'synthetic-left', 'ncamera':1, 'cameraMask':2})
        pcs.stop()

    def test_cwipc_synthetic_config(self):
        """Is a synthetic pointcloud generator providing the correct config interface?"""
        pcs = cwipc.cwipc_synthetic()
        self.assertFalse(pcs.reload_config("auto"))
        self.assertFalse(pcs.reload_config("{\"dummy\":0}"))
        with self.assertRaises(cwipc.CwipcError):
            _ = pcs.get_config()
        pcs.stop()

    def test_cwipc_capturer_nonexistent(self):
        """Check that creating a capturer for a nonexistent camera type fails"""
        with self.assertRaises(cwipc.CwipcError):
            pcs = cwipc.cwipc_capturer('{"type":"nonexistent"}') # type: ignore
        
    def test_tilefilter(self):
        """Check that the tilefilter returns the same number of points if not filtering, and correct number if filtering"""
        gen = cwipc.cwipc_synthetic()
        didStart = gen.start()
        self.assertTrue(didStart)
        pc_orig = gen.get()
        self.assertIsNotNone(pc_orig)
        assert pc_orig # Only to keep linters happy
        pc_filtered = cwipc.cwipc_tilefilter(pc_orig, 0)
        self.assertEqual(len(pc_orig.get_points()), len(pc_filtered.get_points()))
        pc_filtered_1 = cwipc.cwipc_tilefilter(pc_orig, 1)
        pc_filtered_2 = cwipc.cwipc_tilefilter(pc_orig, 2)
        self.assertEqual(len(pc_orig.get_points()), len(pc_filtered_1.get_points()) + len(pc_filtered_2.get_points()))
        self.assertEqual(pc_orig.timestamp(), pc_filtered_1.timestamp())
        self.assertEqual(pc_orig.timestamp(), pc_filtered_2.timestamp())
        gen.stop()
        
    def test_tilefilter_empty(self):
        """Check that the tilefilter returns an empty pointcloud when passed an empty pointcloud"""
        pc_orig = cwipc.cwipc_from_points([], 0)
        pc_filtered = cwipc.cwipc_tilefilter(pc_orig, 0)
        self.assertEqual(len(pc_orig.get_points()), 0)
        self.assertEqual(len(pc_filtered.get_points()), 0)
        
    def test_join(self):
        """Check that joining two pointclouds results in a pointcloud with the correct number of points"""
        gen = cwipc.cwipc_synthetic()
        didStart = gen.start()
        self.assertTrue(didStart)
        pc_1 = gen.get()
        self.assertIsNotNone(pc_1)
        assert pc_1 # Only to keep linters happy
        pc_2 = gen.get()
        self.assertIsNotNone(pc_2)
        assert pc_2 # Only to keep linters happy
        pc_out = cwipc.cwipc_join(pc_1, pc_2)
        self.assertEqual(len(pc_out.get_points()), len(pc_1.get_points()) + len(pc_2.get_points()))
        
    def test_tilemap(self):
        """Check that tilemap keeps the correct numer of points in the mapped tiles"""
        gen = cwipc.cwipc_synthetic()
        didStart = gen.start()
        self.assertTrue(didStart)
        pc_orig = gen.get()
        self.assertIsNotNone(pc_orig)
        assert pc_orig # Only to keep linters happy
        pc_filtered_1 = cwipc.cwipc_tilefilter(pc_orig, 1)
        pc_filtered_2 = cwipc.cwipc_tilefilter(pc_orig, 2)
        pc_filtered_5 = cwipc.cwipc_tilefilter(pc_orig, 5)
        pc_filtered_6 = cwipc.cwipc_tilefilter(pc_orig, 6)
        pc_mapped = cwipc.cwipc_tilemap(pc_orig, {1:5, 2:6})
        pc_mapped_1 = cwipc.cwipc_tilefilter(pc_mapped, 1)
        pc_mapped_2 = cwipc.cwipc_tilefilter(pc_mapped, 2)
        pc_mapped_5 = cwipc.cwipc_tilefilter(pc_mapped, 5)
        pc_mapped_6 = cwipc.cwipc_tilefilter(pc_mapped, 6)
        self.assertEqual(len(pc_filtered_1.get_points()), len(pc_mapped_5.get_points()))
        self.assertEqual(len(pc_filtered_2.get_points()), len(pc_mapped_6.get_points()))
        self.assertEqual(len(pc_filtered_5.get_points()), len(pc_mapped_1.get_points()))
        self.assertEqual(len(pc_filtered_6.get_points()), len(pc_mapped_2.get_points()))
        gen.stop()
        
    def test_colormap(self):
        """Check that colormap keeps all points but gives them the new color"""
        gen = cwipc.cwipc_synthetic()
        didStart = gen.start()
        self.assertTrue(didStart)
        pc = gen.get()
        self.assertIsNotNone(pc)
        assert pc # Only to keep linters happy
        pc2 = cwipc.cwipc_colormap(pc, 0xffffffff, 0x010203)
        points = pc.get_points()
        points2 = pc2.get_points()
        self.assertEqual(len(points), len(points2))
        for i in range(len(points)):
            op = points[i]
            np = points2[i]
            self.assertEqual((op.x, op.y, op.z), (np.x, np.y, np.z))
            self.assertEqual((np.r, np.g, np.b, np.tile), (0x01, 0x02, 0x03, 0x00))
        gen.stop()
        
    def test_crop(self):
        """Check that splitting a pointcloud into two using cropping gives the right number of points"""
        gen = cwipc.cwipc_synthetic()
        didStart = gen.start()
        self.assertTrue(didStart)
        pc = gen.get()
        self.assertIsNotNone(pc)
        assert pc # Only to keep linters happy
        left_pc = cwipc.cwipc_crop(pc, [-999, 0, -999, 999, -999, 999])
        right_pc = cwipc.cwipc_crop(pc, [0, 999, -999, 999, -999, 999])
        points = pc.get_points()
        left_points = left_pc.get_points()
        right_points = right_pc.get_points()
        self.assertEqual(len(points), len(left_points)+len(right_points))
        for pt in left_points:
            self.assertLess(pt.x, 0)
        for pt in right_points:
            self.assertGreaterEqual(pt.x, 0)
        gen.stop()
                
    def test_remove_outliers(self):
        """Chech that remove_outliers returns less points than the original pc, but still > 0 points."""
        gen = cwipc.cwipc_synthetic()
        didStart = gen.start()
        self.assertTrue(didStart)
        pc_orig = gen.get()
        self.assertIsNotNone(pc_orig)
        assert pc_orig # Only to keep linters happy
        count_orig = len(pc_orig.get_points())
        pc_filtered = cwipc.cwipc_remove_outliers(pc_orig, 30, 1.0, True)
        count_filtered = len(pc_filtered.get_points())
        self.assertLess(count_filtered, count_orig)
        self.assertGreater(count_filtered, 0)
        gen.stop()
        
    def test_downsample(self):
        """Check that the downsampler returns at most the same number of points and eventually returns less than 8"""
        gen = cwipc.cwipc_synthetic()
        didStart = gen.start()
        self.assertTrue(didStart)
        pc_orig = gen.get()
        self.assertIsNotNone(pc_orig)
        assert pc_orig # Only to keep linters happy
        count_orig = len(pc_orig.get_points())
        count_filtered = count_orig
        cellsize = pc_orig.cellsize() / 2
        while cellsize < 16:
            pc_filtered = cwipc.cwipc_downsample(pc_orig, cellsize)
            count_filtered = len(pc_filtered.get_points())
            self.assertGreaterEqual(count_filtered, 1)
            self.assertLessEqual(count_filtered, count_orig)
            self.assertEqual(pc_orig.timestamp(), pc_filtered.timestamp())
            if count_filtered < 2:
                break
            cellsize = cellsize * 2
        self.assertLessEqual(count_filtered, 8)
        gen.stop()
        
    def test_downsample_voxelgrid(self):
        """Check that the voxelgrid downsampler returns at most the same number of points and eventually returns less than 8"""
        gen = cwipc.cwipc_synthetic()
        didStart = gen.start()
        self.assertTrue(didStart)
        pc_orig = gen.get()
        self.assertIsNotNone(pc_orig)
        assert pc_orig # Only to keep linters happy
        count_orig = len(pc_orig.get_points())
        count_filtered = count_orig
        cellsize = pc_orig.cellsize() / 2
        while cellsize < 16:
            pc_filtered = cwipc.cwipc_downsample(pc_orig, -cellsize)
            count_filtered = len(pc_filtered.get_points())
            self.assertGreaterEqual(count_filtered, 1)
            self.assertLessEqual(count_filtered, count_orig)
            self.assertEqual(pc_orig.timestamp(), pc_filtered.timestamp())
            if count_filtered < 2:
                break
            cellsize = cellsize * 2
        self.assertLessEqual(count_filtered, 8)
        gen.stop()
                
    def test_downsample_empty(self):
        """Check that the downsample returns an empty pointcloud when passed an empty pointcloud"""
        pc_orig = cwipc.cwipc_from_points([], 0)
        pc_filtered = cwipc.cwipc_downsample(pc_orig, 1)
        self.assertEqual(len(pc_orig.get_points()), 0)
        self.assertEqual(len(pc_filtered.get_points()), 0)
        
    def test_playback_file(self):
        src = cwipc.playback.cwipc_playback([PLY_FILENAME], loop=False)
        didStart = src.start()
        self.assertTrue(didStart)
        self.assertFalse(src.eof())
        pc = src.get()
        self.assertIsNotNone(pc)
        assert pc # Only to keep linters happy
        self._verify_pointcloud(pc)
        self.assertTrue(src.eof())
        src.stop()

    def test_playback_dir(self):
        src = cwipc.playback.cwipc_playback(PLY_DIRNAME, loop=False)
        self.assertFalse(src.eof())
        pc = src.get()
        self.assertIsNotNone(pc)
        assert pc # Only to keep linters happy
        self._verify_pointcloud(pc)
        src.stop()

    @unittest.skip("Fails for reasons unknown")  
    def test_proxy(self):
        src = cwipc.cwipc_proxy('', 8887)
        self.assertFalse(src.available(False))
        src.stop()
        
    @unittest.skip("Fails for reasons unknown")  
    def test_proxy_badhost(self):
        with self.assertRaises(cwipc.CwipcError):
            src = cwipc.cwipc_proxy('8.8.8.8', 8887)
            src.stop()
    
    @unittest.skip("Fails for reasons unknown")  
    def test_proxy_unknownhost(self):
        with self.assertRaises(cwipc.CwipcError):
            src = cwipc.cwipc_proxy('unknown.host.name', 8887)
            src.stop()
        
    def test_metadata_empty(self):
        pc = self._build_pointcloud()
        metadata = pc.access_metadata()
        self.assertNotEqual(metadata, None)
        assert metadata # Keep linters happy
        nItems = metadata.count()
        self.assertEqual(nItems, 0)
        
    def _verify_pointcloud(self, pc : cwipc.cwipc_pointcloud_wrapper, tiled : bool=False):
        points = pc.get_points()
        self.assertGreater(len(points), 1)
        p0 = points[0].x, points[0].y, points[0].z
        p1 = points[len(points)-1].x, points[len(points)-1].y, points[len(points)-1].z
        self.assertNotEqual(p0, p1)
        if tiled:
            t0 = points[0].tile
            t1 = points[len(points)-1].tile
            self.assertNotEqual(t0, t1)
    
    def _build_pointcloud(self):
         points = cwipc.cwipc_point_array(values=[(1, 2, 3, 0x10, 0x20, 0x30, 1), (4, 5, 6, 0x40, 0x50, 0x60, 2)])
         return cwipc.cwipc_from_points(points, 0)
   
if __name__ == '__main__':
    unittest.main()
