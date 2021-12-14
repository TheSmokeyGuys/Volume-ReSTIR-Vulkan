# Volume-ReSTIR
Vulkan implementation of Fast Volume Rendering with Spatiotemporal Reservoir Resampling.

* Zhihao Ruan (ruanzh@seas.upenn.edu), Shubham Sharma (sshubh@seas.upenn.edu), Raymond Yang (rayyang@seas.upenn.edu)
* Tested on: Windows 10 Home 21H1 Build 19043.1288, Ryzen 7 3700X @ 3.59GHz 48GB, RTX 2060 Super 8GB

**This project requires an RTX-compatible (Vulkan Ray Tracing KHR-compatible) graphics card to run.**

![](img/explosion2.gif)

![](img/demo1.png)

## Demos
|   Volume-ReSTIR    |   Normal ReSTIR    |
| :----------------: | :----------------: |
| ![](img/demo1.png) | ![](img/demo3.png) |

| Ray Traced Volume Rendering (with OBJ Models) | Ray Traced  Volume Rendering |
| :-------------------------------------------: | :--------------------------: |
|              ![](img/demo2.png)               |      ![](img/demo4.png)      |

### Rasterization vs. Path Tracing
|      Rasterization      |       Path Tracing       |
| :---------------------: | :----------------------: |
| ![](img/rasterizer.png) | ![](img/path_tracer.gif) |


### ReSTIR
![](img/fire.gif)

## Usage
### Software Dependencies
This project depends on the following softwares/drivers:
* C++ 17
* Vulkan SDK 1.2.189.2
* OpenVDB 8.0+
* CUDA 11.3+

### Building on Windows
This project relies on [Vcpkg](https://github.com/microsoft/vcpkg) to provide Windows support. Before building the code, one should make sure a working Vcpkg is installed in the system.

1. Install [OpenVDB](https://www.openvdb.org/) using Vcpkg:

```bash
vcpkg install openvdb --triplet=x64-windows
vcpkg integrate install
```

2. Create a build folder in the project directory.

```bash
mkdir build; cd build
```

3. Specify the root directory of Vcpkg in CMake configurations (default to be `C:\vcpkg`):

```bash
cmake .. -DVcpkg_ROOT=<PATH_TO_VCPKG>
```
If using CMake GUI, add a cache entry "Vcpkg_ROOT" of type `STRING` and type in the path to Vcpkg root folder.

4. On CMake GUI, Configure and generate the project.
5. Open Visual Studio 2019, select `volume_restir` project as startup project, and run the project.

### (Beta) Building on Linux (Ubuntu)
Before building, make sure that a working version of OpenVDB has been installed in the system.

```bash
mkdir build; cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
./bin/volume_restir
```

## Highlights 
This project achieves the following:
* Vulkan ray tracing pipeline with [nvpro](https://github.com/nvpro-samples/nvpro_core) and Vulkan Ray Tracing KHR extension.
* Volume assets loading and rendering through [OpenVDB](https://www.openvdb.org/).
* ReSTIR algorithm rendering on GLTF scene and volume assets.

## Introduction
ReSTIR has been a very successful fast path tracing-based rendering algorithm in recent computer graphics. However, the current state-of-the-art ReSTIR algorithm only works with meshes. Moreover, there exists some common objects in the scene that are not suitable for mesh creation. VDB asset is a special kind of asset that compensates the drawback of meshes and provides a much more accurate description for volume-based objects, such as smokes, clouds, fire flames, etc. Therefore, it is of great importance to migrate current ReSTIR procedures on volume assets so that we could also render the smokes and clouds photo-realistically and efficiently. 

### VDB Data Structures
VDB is a special type of data structure for smokes, clouds, fire flames, etc. that is based on hierarchical voxel grids. It essentially holds a set of particles. It also uses a similar tree-like data structure as scene graphs for fast traversal and access that stores all transformations at intermediate nodes, and only the particle positions at leaf nodes.

![](img/VDB.png)
![](/img/VDB-diagram.jpeg)

### ReSTIR Algorithm 
*Source:* 
- [*Spatiotemporal reservoir resampling for real-time ray tracing with dynamic direct lighting*](https://research.nvidia.com/sites/default/files/pubs/2020-07_Spatiotemporal-reservoir-resampling/ReSTIR.pdf)
- [*Fast Volume Rendering with Spatiotemporal Reservoir Resampling*](https://research.nvidia.com/publication/2021-11_Fast-Volume-Rendering)

ReSTIR algorithm is a special ray tracing-based rendering algorithm that deals with large number of light source efficiently. It takes advantage of alias tables for Resampled Importance Sampling (RIS) and flexible reservoir data structures. RIS effectively culls images of low weight lights and constructs a PDF of lights for the scene. The reservoirs, allocated one for each pixel, map geometry collisions to light sources in the scene. The reservoirs are easily updated for every bounce, each time considering a candidate from a subset of all lights. If the candidate is chosen, the reservoir will map the geometry to the new light. As more samples are considered, it becomes less likely for any candidate to be placed into the reservoir. These reservoirs use a combination of probability and giving up precision to generate result pixel color such that the image converges in near real time.

![](img/ReSTIR.png)

### Vulkan Ray Tracing
Vulkan is considered as the next-generation API for uniform graphics drivers. It is fast, efficient, light weight, but yet verbose. Vulkan users needs to explicitly set up every little details of the entire rendering pipeline, which is often quite problematic. In this project we provide an explicit example of setting up a working Vulkan rendering pipeline, and it generally involves the following procedures:
1. Set up [glfw](https://github.com/glfw/glfw) to work with the latest version of Vulkan.
2. Initialize Vulkan instance, Vulkan physical device, Vulkan logical device, Vulkan swapchain (for passing to frame buffer display), Vulkan graphics command queue.
3. Create all buffers to be passed to device memory. This includes: frame buffer, rendering pipeline description buffer, data buffer (primitive vertices, indices, normals, materials, textures, etc.).
4. Create descriptor set for all device buffers. Vulkan descriptor set defines the *stage* that GPU reads the data (either in vertex shader, fragment shader, or any stage in the ray tracing pipeline) and the *way* that GPU reads the data (either as uniform samplers, uniform images, storage buffers, uniform buffers, etc.).
5. Bind the data buffer with the corresponding descriptor set.
6. Create the graphics pipeline. Graphics pipelines are defined in `VkPipeline`, which specifies the actual procedures of an entire render pass. It can either be defined as a rasterization pipeline (vertex shader --> primitive assembly --> rasterization --> fragment shader), or a ray tracing pipeline (ray generation shader --> ray intersection shader --> ray closest hit shader/ray miss shader --> post processing fragment shader).
7. Get the current command buffer, prepare the frame, and run the pre-defined pipeline with all device buffers passing in using descriptor sets. 

Vulkan does not render objects directly. Instead, it uses multiple command queues to queue all commands to be passed onto device. This is actually very similar to CUDA as CUDA also uses multiple streams to send commands to the GPU. That's why the graphics command queue comes in.

The ray tracing pipeline in Vulkan requires the usage of Vulkan Acceleration structures. The acceleration structure are divided into two levels to allow shared usage of geometry resources. For example, if you have a high poly model of sphere mesh formed with 30000 triangles, a great optimisation on the memory side would be to create the acceleration structure only once and transform it two detect intersection of rays with it. 

The structure is stored in form of AABBs and can be controlled by
1. Bottom Level Acceleration Structure (BLAS) - Used for creating the the Axis Aligned bounding box for a geometry. Vulkan Provides built in feature to build the AABBs for the triangle primitives but we have to specify the AABBs ourselves for non primitive geometry such as Sphere in our case. 
2. Top Level Acceleration structure - Points to an instance of BLAS, this allows easy transformations of an exisiting instance of BLAS across the scene.
Note - BLAS requires the AABBs to be passed as 4 bit floating point. So if you see any un intended behaviour do look at memory padding in C++.

It is generally very hard to set up a complete Vulkan rendering pipeline from the lowest-level Vulkan libraries. Nowadays, there have been many different styles of Vulkan libraries wrappers for cleaner code production. In this project we took the advantage of [nvpro](https://github.com/nvpro-samples/nvpro_core) to set up a clean Vulkan pipeline for us.

## Pitch
- [Project Pitch](https://github.com/TheSmokeyGuys/Volume-ReSTIR-Vulkan/blob/task/updateReadme/docs/finalProjectPitch.pdf)

## Milestone 1 (11/17/2021)
- [Milestone 1 Presentation](https://github.com/TheSmokeyGuys/Volume-ReSTIR-Vulkan/blob/task/updateReadme/docs/milestone1.pdf)
By milestone 1, we have set up a basic Vulkan rendering pipeline using [vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap).

## Milestone 2 (11/29/2021)
- [Milestone 2 Presentation](https://github.com/TheSmokeyGuys/Volume-ReSTIR-Vulkan/blob/task/updateReadme/docs/milestone2.pdf)
By milestone 2, we have set up general 3D rendering in Vulkan and volume rasterization with [OpenVDB](https://www.openvdb.org/) 

## Milestone 3 (12/06/2021)
- [Milestone 3 Presentation](https://github.com/TheSmokeyGuys/Volume-ReSTIR-Vulkan/blob/task/updateReadme/docs/milestone3.pdf)
By milestone 3, we have set up a pipeline for path tracing volumetric data and a separate pipeline for ReSTIR on triangle mesh 3D data. 

<p align="center">
<img src="img/explosionPathTrace.gif" alt="Explosion VDB" width="800" />
</p>

<p align="center">
<img src="img/firePathTrace.gif" alt="Fire VDB" width="800" />
</p>

<p align="center">
<img src="img/rabbitPathTrace.gif" alt="Rabbit VDB" width="800" />
</p>

<p align="center">
<img src="img/sceneRestir.gif" alt="Scene ReSTIR" width="800" />
</p>



## References
1. Daqi Lin, Chris Wyman, Cem Yuksel. [Fast Volume Rendering with Spatiotemporal Reservoir Resampling](https://research.nvidia.com/publication/2021-11_Fast-Volume-Rendering). ACM Transactions on Graphics (Proceedings of SIGGRAPH Asia 2021), 40, 6, 2021.
2. [OpenVDB](https://www.openvdb.org/) for VDB data loading.
3. [nvpro](https://github.com/nvpro-samples/nvpro_core) for Vulkan Ray Tracing KHR setup.
4. [spdlog](https://github.com/gabime/spdlog) for fast C++ logging.
5. Common graphics utilities [glfw](https://github.com/glfw/glfw), [glm](https://github.com/g-truc/glm), [stb](https://github.com/nothings/stb). These are all included in the [nvpro](https://github.com/nvpro-samples/nvpro_core).
