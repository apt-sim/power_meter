This library provides a software power meter for Intel and AMD CPUs, and Nvidia GPUs. On CPU it reads from Intel and AMD's RAPL interfaces, while on GPU it uses Nvidia's NVML interface.

The simplest way to obtain measurements is by using the built-in monitoring loop. It runs on a separate thread taking measurements at the specified sampling interval, the measurements are stored at "power_meter_out/[cpu/gpu]" or optionally on user-specified files.

It's important to note that the measured application needs to run with root privileges in order to be able to read the CPU's MSR registers.

In order to start the monitoring, run:

```
power_meter::launch_monitoring_loop([Sampling interval in ms]);
```

To stop it:

```
power_meter::stop_monitoring_loop();
```

# Build

The library has no dependencies on other packages, to configure, run:

```
cmake -S. -B./build -DCMAKE_INSTALL_PREFIX=[install_directory]
```

To build and install:

```
cd build/ && make install
```

# Integration in other projects

In order to include the library on other Cmake projects, add the following line to `CMakeLists.txt`:

```
find_package(Power_meter REQUIRED)
```

This will make the library `Power_meter::Power_meter` available
