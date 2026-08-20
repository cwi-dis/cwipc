# Aruco Markers

These markers are intended to be printed on A4 paper. If you print them at another size please ensure that they are still rectangular. If you print them at a very different size (more than, say, 20% scaling) you may need to modify `cwipc_register`.

## Usage

If all cameras can see a common point on the floor: call this _Origin_, also known as `(0, 0, 0)`, print `target-a4-aruco-0.pdf` and place that marker there. The arrow shows the "natural forward looking direction" of the captured subject.

Now `cwipc_register` should be able to find this marker and do coarse alignment of the camera positions.

If automatic coarse alignment does not work you may be able to do manual coarse alignment: call `cwipc_register --no_aruco` and manually select the four points on the floor _just outside_ the A4-sized marker print. You must ensure you select the floor points in the correct order.

Laminating your printed markers may be a good idea (because they will fold less easily) and may be a bad idea (because the reflections may hinder recognition).

### Advanced usage

If not all cameras can see the origin you also print some of the auxiliary markers. Place the origin marker where it can be seen by most cameras (as in the step above). Now place one or more auxiliary markers where they can be seen by some of the cameras that can also see the origin marker, but also by some cameras that cannot see the origin marker. Continue with this until all cameras can see at least one marker, and every marker is seen by at least two cameras.

If all goes well `cwipc_register` should now first register all the cameras that can see the origin marker. These cameras are now aligned, and moreover the `(x, y, z)` position of at least one auxiliary marker should be known. So now all cameras that can see that specific auxiliary marker (but not the origin marker) can also be aligned. etc.