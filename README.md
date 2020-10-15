# KWIKFormat
A plugin to record to and read from files based on the KWIK specification

## Installation
### Installing the OpenEphysHDF5Lib Common Library
This plugin depends on the [OpenEphysHDF5Lib](https://github.com/open-ephys-plugins/OpenEphysHDF5Lib) common library. Make sure you build and install that first before you proceed with building KWIKFormat plugin.

### Building the plugin
Building the plugins requires [CMake](https://cmake.org/). Detailed instructions on how to build open ephys plugins with CMake can be found in [our wiki](https://open-ephys.atlassian.net/wiki/spaces/OEW/pages/1259110401/Plugin+CMake+Builds).

#### [MacOS only] Update rpaths
After builing and installing the plugin with INSTALL scheme, update the rpaths of HDF5 libraries linked to `KWIKFormat.bundle` at `${YOUR_HOME_DIR}/Library/Application Support/open-ephys/plugins` by running the following commands:
```
install_name_tool -change /usr/local/lib/libhdf5.10.dylib @rpath/libhdf5.10.dylib path/to/KWIKFormat.bundle/Contents/MacOS/KWIKFormat
```
```
install_name_tool -change /usr/local/lib/libhdf5_cpp.15.dylib @rpath/libhdf5_cpp.15.dylib path/to/KWIKFormat.bundle/Contents/MacOS/KWIKFormat
```
 