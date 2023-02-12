Frustum culling demo.

Frustum culling is an extremely important tool for interactive rendering. It allows the GPU to only process 
primitives that have the chance of being captured by the view camera. To accomplish this we do the following:

![frustum-culling](https://github.com/abkour/moonlight/blob/main/src/demos/01_frustum_culling/frustum_culling.gif)

<h3>Mathematical foundation</h3>

First, we require procedures for testing whether a rendering object is within the viewing frustum or not.
Models typically consist of primitives, usually triangles. Doing this on all triangles within a model
is prohibitive, therefore we encapsulate our model into a bounding volume. For this demo I chose the AABB,
because it is simple, performant and easy to work with.

To encapsulate the model within the AABB, we iterate over all vertices that make up the model. 
On each iteration step, we potentially update two variables (bmin, bmax), when the candidate vertex has 
a component k that is smaller/greater than the vertex component of bmin/bmax.
With the AABB constructed, the next step is to construct the frustum. A frustum can be described as a
set of six planes. Each plane in turn can be described by a normal pointing in the positive halfspace 
of the enclosing space that the plane seperates, and a point on the plane.

The frustum's planes all point inwards by construction, therefore to test that an AABB is inside the frustum,
all points in the AABB have to be within the positive halfspace of all planes. This can be done with the 
seperating-axis test. I won't go into details on this, since it requires too much description, but you can find 
the implemention in the src/math/primitive_tests.hpp file.

<h3>DX12</h3>