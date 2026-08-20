"""
Create a point cloud for testing analysis and registration.
"""
import sys
import os.path
import json
from typing import Optional, Dict, Any
import argparse
import traceback
import random
import numpy as np
from scipy.spatial.transform import Rotation, RigidTransform
import cwipc
from cwipc.registration.fine import RegistrationComputer_ICP_Point2Point
from cwipc.registration.analyze import RegistrationAnalyzer
from cwipc.registration.util import transformation_topython, transformation_identity, cwipc_transform
from cwipc.registration.plot import Plotter
from cwipc.filters.simulatecams import SimulatecamsFilter
from cwipc.filters.noise import NoiseFilter

class AnalysisTestCreator:
    def __init__(self, args : argparse.Namespace):
        self.args = args
        self.verbose = args.verbose
        self.noise = args.noise
        self.ncamera = args.ncamera
        self.per_camera_movement = args.move
        self.per_camera_rotate = args.rotate
        self.per_camera_tilt = args.tilt
        self.input_pc : Optional[cwipc.cwipc_pointcloud_wrapper] = None
        self.output_pc : Optional[cwipc.cwipc_pointcloud_wrapper] = None
        self.description : Optional[Dict[str, Any]] = None
        if self.args.descr:
            self.create_default_description()
            

    def load_input(self, source: str):
        pc = cwipc.cwipc_read(source, 0)
        self.input_pc = pc

    def save_output(self, target: str):
        assert self.output_pc
        cwipc.cwipc_write(target, self.output_pc)
        if self.description:
            target_base, _ = os.path.splitext(target)
            target_json = target_base + ".json"
            json.dump(self.description, open(target_json, "w"), indent=2)
    
    def create_default_description(self) -> None:
        self.description = dict(
            noise=0.0,
            tiles = [
                dict(
                    corr=0,
                    move=dict(
                        x=0,
                        y=0,
                        z=0
                    ),
                    rotate=dict(
                        x=0,
                        y=0,
                        z=0
                    )
                )
                for tilenum in range(self.ncamera)
            ]
        )

    def run(self):
        assert self.input_pc
        sim_filter = SimulatecamsFilter(self.args.ncamera, hard=False, skew=self.args.skew)
        tiled_pc = sim_filter.filter(self.input_pc)
        if self.verbose:
            print(f"Input point cloud tiled into {self.args.ncamera} cameras, {tiled_pc.count()} points in total.")
        per_tile_pcs = []
        for camnum in range(self.args.ncamera):
            tilemask = 1 << camnum
            per_tile_pc = cwipc.cwipc_tilefilter(tiled_pc, tilemask)
            if self.verbose:
                print(f"Tile {camnum} has {per_tile_pc.count()} points")
            transform = transformation_identity()
            transform_changed = False
            # Rotate a camera around the Y axis.
            if self.per_camera_rotate and camnum < len(self.per_camera_rotate) and self.per_camera_rotate[camnum] != 0:
                rotation = self.per_camera_rotate[camnum]
                new_rotation = Rotation.from_euler('y', rotation)
                new_transform = RigidTransform.from_rotation(new_rotation)
                transform = new_transform.as_matrix() @ transform
                transform_changed = True
                # Make a wild guess at the movement, assuming points are about 20 cm from the Y axis
                movement = abs(0.2 * rotation)
                if self.description:
                    self.description["tiles"][camnum]["rotate"]["y"] += rotation
                    self.description["tiles"][camnum]["corr"] += movement
            # Tilt a camera around the X or Z axis
            if self.per_camera_tilt and camnum < len(self.per_camera_tilt) and self.per_camera_tilt[camnum] != 0:
                rotation = self.per_camera_tilt[camnum]
                rotation_axis = random.choice(('x','z'))
                new_rotation = Rotation.from_euler(rotation_axis, rotation)
                new_transform = RigidTransform.from_rotation(new_rotation)
                transform = new_transform.as_matrix() @ transform
                transform_changed = True
                # Make a wild guess at the movement, assuming a human is about 1.8m tall
                movement = abs(1.8 * rotation)
                if self.description:
                    self.description["tiles"][camnum]["rotate"][rotation_axis] += rotation
                    self.description["tiles"][camnum]["corr"] += movement

            # Move camera in a random direction (in the Z=0 plane)
            if self.per_camera_movement and camnum < len(self.per_camera_movement) and self.per_camera_movement[camnum] > 0:
                movement = self.per_camera_movement[camnum]
                random_angle = np.random.uniform(0, 2 * np.pi)
                delta_x = movement * np.cos(random_angle)
                delta_z = movement * np.sin(random_angle)
                transform[0, 3] += delta_x
                transform[2, 3] += delta_z
                if self.description:
                    self.description["tiles"][camnum]["move"]["x"] += delta_x
                    self.description["tiles"][camnum]["move"]["z"] += delta_z
                    self.description["tiles"][camnum]["corr"] += movement
                transform_changed = True
            if transform_changed:
                if self.verbose:
                    print(f"Moving tile {camnum} by {transform}")
                per_tile_pc = cwipc_transform(per_tile_pc, transform)
            per_tile_pcs.append(per_tile_pc)
        cwipc_joined_pc = cwipc.cwipc_join_multi(per_tile_pcs)
        if self.noise > 0:
            if self.verbose:
                print(f"Adding noise of {self.noise} meters to the point cloud")
            noise_filter = NoiseFilter(self.noise)
            if self.description != None:
                self.description["noise"] = self.noise
            cwipc_joined_pc = noise_filter.filter(cwipc_joined_pc)
        self.output_pc = cwipc_joined_pc
       
def main():
    assert __doc__ is not None
    parser = argparse.ArgumentParser(description=__doc__.strip(), formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("input", help="Input point cloud .ply file")
    parser.add_argument("output", help="Output point cloud .ply file")
    parser.add_argument("--ncamera", type=int, metavar="NUM", default=1, help="Number of cameras to simulate")
    parser.add_argument("--skew", type=float, metavar="FACTOR", default=1, help="Skew point camera distribution towards the closest one by this factor")
    parser.add_argument("--move", type=float, action="append", metavar="D", help="Distance to move a tile (in meters) in the Y=0 plane, random XZ angle. Repeat for each tile.")
    parser.add_argument("--rotate", type=float, action="append", metavar="RAD", help="Angle to rotate a tile (in radians) around the Y axis. Repeat for each tile.")
    parser.add_argument("--tilt", type=float, action="append", metavar="RAD", help="Angle to rotate a tile (in radians) around the X or Z axis. Repeat for each tile.")
    parser.add_argument("--noise", type=float, metavar="DIST", default=0.0, help="Add noise to each point (in meters)")
    parser.add_argument("--descr", action="store_true", help="Also store description of modifications as a JSON file")
    parser.add_argument("--verbose", action="store_true", help="Verbose output")
    parser.add_argument("--debugpy", action="store_true", help="Wait for debugpy client to attach")
    args = parser.parse_args()
    if args.debugpy:
        import debugpy
        debugpy.listen(5678)
        print(f"{sys.argv[0]}: waiting for debugpy attach on 5678", flush=True)
        debugpy.wait_for_client()
        print(f"{sys.argv[0]}: debugger attached")
    creator = AnalysisTestCreator(args)
    creator.load_input(args.input)
    creator.run()
    creator.save_output(args.output)
    
if __name__ == '__main__':
    main()
    
    
    
