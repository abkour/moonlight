*MOONLIGHT

Moonlight is a from DX12 framework that was originally intended for implementing and testing
computer graphics solutions.

With moonlight I was able to built the following applications:

    1. Backwards Path Tracer
    2. PBR texture viewer
    3. Frustum culling demo
    4. Tetris

Each of the demos was built entirely from scratch. No external libraries were used, except for ImGUI 
to provide a GUI to the user.

PROJECTS

1. Backwards Path Tracer
A simple backwards path tracer. Features include:
    - Importance sampling
    - BVHs
    - Lambertian surfaces
    - Area light sources
    - Ambient occlusion
    - Customizable model loader

Features I want to implement:
    - Bidirectionality
    - Arbitrary light shapes in combination with Importance Sampling
    - Physically based shading model
    - IBL

Optimization
The renderer originally supported single/multithreaded CPU Raytracing and Compute Shader Raytracing.
However, during feature development, the Compute Shader implementation is multiple versions behind.
My plan is to work on the CS implementation once I have implemented the wanted features listed above.
The reason for this is that debugging on the CPU is much easier than on the GPU.


2. PBR texture viewer
This is a simple application that allows the user to view a PBR texture under the influence of different
light conditions.
It uses the lambertian BRDF for diffuse surfaces. For specular surfaces I used the Cook-Torrance BRDF.


3. Frustum culling demo
This demo features CPU frustum culling to the amount of geometry pushed to the GPU and therefore it reduces
the amount of vertices the Vertex Shader has to process. The demo uses simple cubes in combination with 
instanced rendering. For that reason, the effects of frustum culling are not that visible. 
In the feature, I will swap out the cubes for more complex models, attach AABBs to them.


4. Tetris
This is a tetris clone. It works like the original game. I was inspired to work on this because it doesn't 
require assets and I since I never developed a proper game I was interested to try it out.


FUTURE PROJECTS

The project I want to ultimately work on the most is a 3D asteroids clone using mesh shaders, LODs and 
instanced rendering. It is currently out of reach for me, since I don't know how to do mesh simplification
to create the LODs for the asteroids. I also have not tried out mesh shaders. 

Another project I'm interested in working on is a water simulation shader. Some fluid simulators produce 
amazing results that I want to be able to recreate.

I need to enhance the DX12 Framework for multithreaded command queues. For as long as this is not possible,
the power of DX12 is not realized.

A frustum based, point light culling algorithm could come in handy in the future.