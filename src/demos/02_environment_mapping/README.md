## Environment mapping

This demo uses cube textures to implement environment mapping. This technique is useful for specular surfaces in 
outdoor scenes. It can be used in combination with SSR to achieve realistic looking reflections.

![frustum-culling](https://github.com/abkour/moonlight/blob/main/src/demos/02_environment_mapping/res/envmap.gif)

### Implementation

First, we load the cube into memory. For this I chose to use DirectTX12, because of its ease of use. To create a
descriptor for a cube texture we simply populate the 'TextureCube' field of the 'D3D12_SHADER_RESOURCE_VIEW_DESC'.
In particular, we set the 'TextureCube.MipLevels' field to 6. The rest is standard resource handling.

With the texture in memory we can implement environment mapping.

First, we need to define the cube that is used to render the 'TextureCube' on. The volume of the cube is not important,
past the fact that it might stretch or contract the texture. The important part is that the UV-Coordinates need to 
be linearly mapped from [minx,miny,minz]x[maxx,maxy,maxz] to [0,0]x[1,1].

Given such a cube, we implement the vertex shader. The vertex shader will take a per-vertex 3D position and normal.
We also supply a transformation matrix to allow the user to rotate the matrix. We don't want the user to move 
the camera as that would allow the camera to move within the cube.

The output of the vertex shader is an interpolated position and normal in world space. This is used in the pixel shader
to compute the view direction V by subtracting the camera position from the ws_position. With this V
and the normal direction we can compute the reflected vector R with the built-in reflect function. 
Finally, given R we can sample the TextureCube object.

