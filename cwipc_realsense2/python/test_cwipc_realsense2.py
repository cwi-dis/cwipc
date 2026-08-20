import unittest
import cwipc
import _cwipc_realsense2
import os
import sys
import tempfile

#
# Find directories for test inputs and outputs
#
_thisdir=os.path.dirname(os.path.join(os.getcwd(), __file__))
_topdir=os.path.dirname(_thisdir)
TEST_FIXTURES_DIR=os.path.join(_topdir, "tests", "fixtures")
TEST_FIXTURES_PLAYBACK_CONFIG=os.path.join(TEST_FIXTURES_DIR, "input", "realsense2_recording", "cameraconfig.json")

class TestApi(unittest.TestCase):
        
    def test_0_check_module_realsense2(self):
        ok = cwipc.cwipc_check_module('realsense2')
        self.assertTrue(ok)
    
    def _open_grabber(self):
        if not 'CWIPC_TEST_HAVE_REALSENSE2_HARDWARE' in os.environ:
            self.skipTest('CWIPC_TEST_HAVE_REALSENSE2_HARDWARE not set in environment')
        grabber = _cwipc_realsense2.cwipc_realsense2("auto")
        didStart = grabber.start()
        self.assertTrue(didStart)
        return grabber
        
    def test_cwipc_realsense2(self):
        """Test that we can grab a realsense2 image"""

        grabber = self._open_grabber()
        self.assertFalse(grabber.eof())
        self.assertTrue(grabber.available(True))
        pc = grabber.get()
        self.assertIsNotNone(pc)
        assert pc # Only to keep linters happy
        self._verify_pointcloud(pc)
        grabber.stop()

    def test_cwipc_realsense2_tileinfo(self):
        """Test that we can get tileinfo from a realsense2 grabber"""
        grabber = self._open_grabber()
        nTile = grabber.maxtile()
        self.assertGreaterEqual(nTile, 1)
        # Assure the non-tiled-tile exists and points nowhere.
        tileInfo = grabber.get_tileinfo_dict(0)
        self.assertIsNotNone(tileInfo)
        assert tileInfo # Only to keep linters happy
        self.assertIn('normal', tileInfo)
        self.assertIn('cameraName', tileInfo)
        self.assertIn('cameraMask', tileInfo)
        self.assertIn('ncamera', tileInfo)
        # Untrue if multiple realsenses connected: self.assertLessEqual(tileInfo['ncamera'], 1)
        # Test some minimal conditions for other tiles
        for i in range(1, nTile):
            tileInfo = grabber.get_tileinfo_dict(i)
            self.assertIsNotNone(tileInfo)
            assert tileInfo # Only to keep linters happy
            if i in (1, 2, 4, 8, 16, 32, 64, 128):
                # These tiles should exist and have a normal and camera ID (which may be None)
                self.assertIn('normal', tileInfo)
                self.assertIn('camera', tileInfo)
        grabber.stop()

    @unittest.skipIf('CI' in os.environ, "Skipping playback test on CI server")
    def test_cwipc_realsense2_playback(self):
        """Test that we can grab a realsense2 image from the playback grabber"""
        if not os.path.exists(TEST_FIXTURES_PLAYBACK_CONFIG):
            self.skipTest(f'Playback config file {TEST_FIXTURES_PLAYBACK_CONFIG} not found')
        grabber = _cwipc_realsense2.cwipc_realsense2_playback(TEST_FIXTURES_PLAYBACK_CONFIG)
        didStart = grabber.start()
        self.assertTrue(didStart)
        self.assertFalse(grabber.eof())
        self.assertTrue(grabber.available(True))
        pc = grabber.get()
        # Hack around problem: sometimes we get NULL or empty clouds at the beginnging of the run.
        if pc is None or pc.count() == 0:
            pc = None
            self.assertTrue(grabber.available(True))
            pc = grabber.get()
        if pc is None or pc.count() == 0:
            pc = None
            self.assertTrue(grabber.available(True))
            pc = grabber.get()
        self.assertIsNotNone(pc)
        assert pc # Only to keep linters happy
        self._verify_pointcloud(pc)
        grabber.stop()

    def _verify_pointcloud(self, pc : cwipc.cwipc_pointcloud_wrapper):
        points = pc.get_points()
        self.assertGreater(len(points), 1)
        halfway = int((len(points)+1)/2)
        p0 = points[0].x, points[0].y, points[0].z, points[0].r, points[0].g, points[0].b
        p1 = points[halfway].x, points[halfway].y, points[halfway].z, points[halfway].r, points[halfway].g, points[halfway].b
        self.assertNotEqual(p0, p1)
        
if __name__ == '__main__':
    unittest.main()
