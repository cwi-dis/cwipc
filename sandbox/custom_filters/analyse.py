import numpy

class CustomFilter:
    """cwipc custom_filter: Analyse pointclouds for bounding box and centroid.
    
    At end of run, prints minimum and maximum x, y and z, and average centroid."""
    def __init__(self):
        self.count = 0
        self.min_x = self.min_y = self.min_z = 999999
        self.max_x = self.max_y = self.max_z = -999999
        self.sum_avg_x = self.sum_avg_y = self.sum_avg_z = 0
        
    def filter(self, pc):
        self.count += 1
        points = pc.get_points()
        count = len(points)
        pointarray = numpy.zeros((count,3))
        for i in range(count):
            p = numpy.array([points[i].x, points[i].y, points[i].z])
            pointarray[i] = p
        min_x = numpy.min(pointarray[:,0])
        max_x = numpy.max(pointarray[:,0])
        min_y = numpy.min(pointarray[:,1])
        max_y = numpy.max(pointarray[:,1])
        min_z = numpy.min(pointarray[:,2])
        max_z = numpy.max(pointarray[:,2])

        self.sum_avg_x += (min_x + max_x) / 2 
        self.sum_avg_y += (min_y + max_y) / 2 
        self.sum_avg_z += (min_z + max_z) / 2 
        return pc
        
    def statistics(self):
        print(f"analyse: count={self.count}")
        print(f"analyse: x: min={self.min_x}, max={self.max_x}, average centroid={self.sum_avg_x/self.count}")
        print(f"analyse: y: min={self.min_y}, max={self.max_y}, average centroid={self.sum_avg_y/self.count}")
        print(f"analyse: z: min={self.min_z}, max={self.max_z}, average centroid={self.sum_avg_z/self.count}")
