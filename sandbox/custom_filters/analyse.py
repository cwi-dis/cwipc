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
        min_x = min_y = min_z = 999999
        max_x = max_y = max_z = -999999
        sum_x = sum_y = sum_z = 0
        for p in points:
            if p.x < min_x: min_x = p.x
            if p.x > max_x: max_x = p.x
            sum_x += p.x
            if p.y < min_y: min_y = p.y
            if p.y > max_y: max_y = p.y
            sum_y += p.y
            if p.z < min_z: min_z = p.z
            if p.z > max_z: max_z = p.z
            sum_z += p.z
        if min_x < self.min_x: self.min_x = min_x
        if max_x > self.max_x: self.max_x = max_x
        if min_y < self.min_y: self.min_y = min_y
        if max_y > self.max_y: self.max_y = max_y
        if min_z < self.min_z: self.min_z = min_z
        if max_z > self.max_z: self.max_z = max_z
        self.sum_avg_x += (min_x + max_x) / 2 
        self.sum_avg_y += (min_y + max_y) / 2 
        self.sum_avg_z += (min_z + max_z) / 2 
        return pc
        
    def statistics(self):
        print(f"analyse: count={self.count}")
        print(f"analyse: x: min={self.min_x}, max={self.max_x}, average centroid={self.sum_avg_x/self.count}")
        print(f"analyse: y: min={self.min_y}, max={self.max_y}, average centroid={self.sum_avg_y/self.count}")
        print(f"analyse: z: min={self.min_z}, max={self.max_z}, average centroid={self.sum_avg_z/self.count}")
