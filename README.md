# VulkanStuff

Instructions for compiling:

Make sure cmake is installed.

Open a cmd window.

Navigate to the glslang folder in the vulkan sdk install directory, for me this was:

cd c:\VulkanSDK\1.1.97.0\glslang

Enter these commands to build the visual studio glslang projects:

mkdir installx64
cd installx64
mkdir install
cmake .. -G "Visual Studio 15 Win64" -DCMAKE_INSTALL_PREFIX="install"

This will build the solution and place it in the installx64 directory.

Open the glslang.sln and build the ALL_BUILD project.  When it finishes, build the INSTALL project.
This will create and populate the folders bin, include, and lib in the install directory we made.
Copy these directories into your vulkan project and link against the compiled libs.

Copy the newly created directories (bin, include, lib folders) into the common/libs/glslang directory.

This is necessary because some of the libs are over the 100mb github file limit.