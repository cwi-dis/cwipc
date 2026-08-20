import unittest
import cwipc
import _cwipc_orbbec
import os
import sys


#
# Find directories for test inputs and outputs
#
_thisdir=os.path.dirname(os.path.join(os.getcwd(), __file__))
_topdir=os.path.dirname(_thisdir)
TEST_FIXTURES_DIR=os.path.join(_topdir, "tests", "fixtures")
TEST_FIXTURES_PLAYBACK_CONFIG=os.path.join(TEST_FIXTURES_DIR, "input", "orbbec_recording", "cameraconfig.json")

class TestApi(unittest.TestCase):
        
    def test_0_check_module_orbbec(self):
        ok = cwipc.cwipc_check_module('orbbec')
        self.assertTrue(ok)
    
    def _open_grabber(self):
        if not 'CWIPC_TEST_HAVE_ORBBEC_HARDWARE' in os.environ:
            self.skipTest('CWIPC_TEST_HAVE_ORBBEC_HARDWARE not set in environment')
        grabber = _cwipc_orbbec.cwipc_orbbec("auto")
        return grabber
        
    def test_cwipc_orbbec(self):
        """Test that we can grab a orbbec image"""
        grabber = self._open_grabber()
        self.assertFalse(grabber.eof())
        self.assertTrue(grabber.available(True))
        pc = grabber.get()
        self.assertIsNotNone(pc)
        assert pc # Only to keep linters happy
        # It seems the first pointcloud could be empty. Unsure why...
        if pc.count() == 0:
            pc = grabber.get()
            self.assertIsNotNone(pc)
            assert pc # Only to keep linters happy
        self._verify_pointcloud(pc)
        grabber.stop()

    def test_cwipc_orbbec_tileinfo(self):
        """Test that we can get tileinfo from a orbbec grabber"""
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
                self.assertIn('cameraName', tileInfo)
        grabber.stop()

    @unittest.skipIf('CI' in os.environ, "Skipping playback test on CI server")
    def test_cwipc_orbbec_playback(self):
        """Test that we can grab a orbbec image from the playback grabber"""
        if not os.path.exists(TEST_FIXTURES_PLAYBACK_CONFIG):
            self.skipTest(f'Playback config file {TEST_FIXTURES_PLAYBACK_CONFIG} not found')
        grabber = _cwipc_orbbec.cwipc_orbbec_playback(TEST_FIXTURES_PLAYBACK_CONFIG)
        didStart = grabber.start()
        self.assertTrue(didStart)
        self.assertFalse(grabber.eof())
        self.assertTrue(grabber.available(True))
        pc = grabber.get()
        self.assertIsNotNone(pc)
        assert pc # Only to keep linters happy
        self._verify_pointcloud(pc)
        grabber.stop()

    @unittest.skip("not implemented yet")
    def test_cwipc_orbbec_playback_seek(self):
        """Test that we can grab a orbbec image from the playback grabber"""
        if not os.path.exists(TEST_FIXTURES_PLAYBACK_CONFIG):
            self.skipTest(f'Playback config file {TEST_FIXTURES_PLAYBACK_CONFIG} not found')
        grabber = _cwipc_orbbec.cwipc_orbbec_playback(TEST_FIXTURES_PLAYBACK_CONFIG)
        self.assertFalse(grabber.eof())
        self.assertTrue(grabber.available(True))
        result = grabber.seek(1600233)
        self.assertTrue(result)
        result = grabber.seek(5000000)
        self.assertFalse(result)
        pc = grabber.get()
        self.assertIsNotNone(pc)
        assert pc # Only to keep linters happy
        self._verify_pointcloud(pc)
        grabber.stop()
        
    def _verify_pointcloud(self, pc : cwipc.cwipc_pointcloud_wrapper) -> None:
        points = pc.get_points()
        self.assertGreater(len(points), 1)
        halfway = int((len(points)+1)/2)
        p0 = points[0].x, points[0].y, points[0].z, points[0].r, points[0].g, points[0].b
        p1 = points[halfway].x, points[halfway].y, points[halfway].z, points[halfway].r, points[halfway].g, points[halfway].b
        self.assertNotEqual(p0, p1)
   
if __name__ == '__main__':
    unittest.main()
