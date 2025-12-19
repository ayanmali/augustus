# Augustus - KVM VM Manager

A C++ library and examples for managing lightweight VMs using KVM (Kernel-based Virtual Machine).

## Overview

This project provides a C++ interface for creating and managing lightweight virtual machines using the KVM API. KVM is a Linux kernel module that provides hardware-assisted virtualization.

## Features

- **VM Lifecycle Management**: Create and destroy VMs
- **Memory Management**: Allocate and configure guest memory regions
- **VCPU Management**: Create and control virtual CPUs
- **VM Execution**: Run VMs and handle exit events
- **Error Handling**: Comprehensive error reporting

## Requirements

- Linux kernel with KVM support
- `/dev/kvm` device file (requires root or kvm group membership)
- GCC with C++20 support
- CMake 3.10 or higher
- Linux kernel headers (for KVM ioctl definitions)

## Building

**Important**: CMake has two steps:
1. **Configure** (`cmake -B build`) - Sets up the build system
2. **Build** (`cmake --build build`) - Actually compiles the code

### Quick Build

```bash
# Configure and build in one go
cmake -B build && cmake --build build
```

### Step-by-step Build

```bash
# Step 1: Configure (creates build files)
cmake -B build

# Step 2: Build (compiles the executables)
cmake --build build

# Or build specific target
cmake --build build --target augustus
cmake --build build --target kvm_demo  # Linux only
```

### Traditional CMake Workflow

```bash
mkdir build && cd build
cmake ..
make
```

**Note**: The `kvm_demo` executable will only be built on Linux systems. On macOS or other platforms, only `augustus` will be built.

The executables will be located in `build/bin/`.

## Usage

### Basic Example

```cpp
#include "kvm/kvm.h"

KVM kvm;
if (!kvm.initialize()) {
    // Handle error
}

int vm_fd = kvm.create_vm();
// ... configure VM ...
```

### Running the Demo

```bash
sudo ./build/bin/kvm_demo
```

Or if you built in-place:

```bash
sudo ./kvm_demo
```

**Note**: Running KVM requires root privileges or membership in the `kvm` group.

## Architecture

- `src/kvm/kvm.h` - KVM class interface
- `src/kvm/kvm.cpp` - KVM implementation
- `src/kvm.cpp` - Example usage demonstrating VM creation and execution

## Key Concepts

### 1. KVM Initialization
- Open `/dev/kvm` device
- Verify API version compatibility
- Check required capabilities

### 2. VM Creation
- Create a VM using `KVM_CREATE_VM` ioctl
- Configure memory regions
- Set up VCPUs

### 3. Memory Management
- Allocate memory for guest using `mmap`
- Register memory regions with KVM
- Guest memory is mapped into the host process

### 4. VCPU Management
- Create VCPUs for the VM
- Configure CPU registers
- Map VCPU run structure for efficient execution

### 5. VM Execution
- Use `KVM_RUN` ioctl to execute guest code
- Handle exit reasons (HLT, I/O, etc.)
- Process VM events as needed

## Exit Reasons

Common VM exit reasons:
- `KVM_EXIT_HLT`: Guest executed HLT instruction
- `KVM_EXIT_IO`: Guest performed I/O operation
- `KVM_EXIT_SHUTDOWN`: VM shutdown requested
- `KVM_EXIT_INTERNAL_ERROR`: Internal KVM error

## Security Note

KVM provides direct hardware access. Always:
- Validate all inputs
- Run with minimal privileges when possible
- Isolate VMs appropriately
- Handle errors gracefully

## Further Reading

- [KVM API Documentation](https://www.kernel.org/doc/html/latest/virt/kvm/api.html)
- [KVM Source Code](https://git.kernel.org/pub/scm/virt/kvm/kvm.git)

## License

See LICENSE file for details.
