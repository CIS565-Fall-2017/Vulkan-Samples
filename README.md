# CIS 565 Vulkan Samples

This respository contains Vulkan sample material for CIS 565 at the University of Pennsylvania.

The [`master`](https://github.com/CIS565-Fall-2017/Vulkan-Samples) branch contains separate folders which contains simple applications which build on each other:

0. [Hello Window](https://github.com/CIS565-Fall-2017/Vulkan-Samples/tree/master/samples/0_helloWindow)
    - Creates a window and sets up the Vulkan instance, logical device, and swapchain
1. [Hello Triangle](https://github.com/CIS565-Fall-2017/Vulkan-Samples/tree/master/samples/1_helloTriangle)
    - Creates a graphics pipline and draws a triangle with hard-coded vertex attributes in the shader
2. [Hello Vertex Buffers](https://github.com/CIS565-Fall-2017/Vulkan-Samples/tree/master/samples/2_helloVertexBuffers)
    - Creates buffers for vertex attributes, binds them, and draws
3. [Hello Uniform Buffers](https://github.com/CIS565-Fall-2017/Vulkan-Samples/tree/master/samples/3_helloUniformBuffers)
    - Adds descriptor pools, descriptor set layouts, and descriptors
    - Creates descriptors for uniform buffer objects
    - Binds descriptors to the graphics pipeline to transform the triangle based on camera position
4. [Hello Compute](https://github.com/CIS565-Fall-2017/Vulkan-Samples/tree/master/samples/4_helloCompute)
    - Adds a compute shader to rotate initial vertex positions based on time
5. [Hello Tessellation](https://github.com/CIS565-Fall-2017/Vulkan-Samples/tree/master/samples/5_helloTessellation)
    - Tessellates a single vertex into a quad

Each sample is also included on its own branch. Diffs of the branches can be viewed on Github to see the changes necessary to incrementally go from one sample to the next:
- [Hello Window --> Hello Triangle](https://github.com/CIS565-Fall-2017/Vulkan-Samples/pull/1/files)
- [Hello Triangle --> Hello Vertex Buffers](https://github.com/CIS565-Fall-2017/Vulkan-Samples/pull/2/files)
- [Hello Vertex Buffers --> Hello Uniform Buffers](https://github.com/CIS565-Fall-2017/Vulkan-Samples/pull/3/files)
- [Hello Uniform Buffers --> Hello Compute](https://github.com/CIS565-Fall-2017/Vulkan-Samples/pull/4/files)
- [Hello Compute --> Hello Tessellation](https://github.com/CIS565-Fall-2017/Vulkan-Samples/pull/5/files)

## Credits
Content adapted from [Vulkan Tutorial](https://vulkan-tutorial.com/)
