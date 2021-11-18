# Volume-ReSTIR
Vulkan implementation of Fast Volume Rendering with Spatiotemporal Reservoir Resampling.

- [Project Pitch](https://github.com/TheSmokeyGuys/Volume-ReSTIR-Vulkan/blob/main/docs/CIS%20565%20Final%20project%20pitch.pdf)
- [Milestone 1 Presentation](https://github.com/TheSmokeyGuys/Volume-ReSTIR-Vulkan/blob/UserRYang-patch-1/docs/CIS%20565%20Milestone1.pdf)

## Usage
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
