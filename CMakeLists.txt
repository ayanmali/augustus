cmake_minimum_required(VERSION 3.10)
project(Augustus VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build configuration
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)

# KVM library (Linux only)
if(UNIX AND NOT APPLE)
    # Check for Linux kernel headers
    find_path(LINUX_KVM_HEADER linux/kvm.h
        PATHS
        /usr/include
        /usr/src/linux/include
        REQUIRED
    )
    
    # KVM library source files
    set(KVM_SOURCES
        src/kvm/kvm.cpp
    )
    
    # Create KVM library
    add_library(kvm_lib STATIC ${KVM_SOURCES})
    target_include_directories(kvm_lib PUBLIC
        ${CMAKE_SOURCE_DIR}/src
    )
    target_compile_definitions(kvm_lib PRIVATE
        _GNU_SOURCE
    )
    
    # KVM demo executable (Linux only)
    add_executable(kvm_demo src/kvm.cpp)
    target_link_libraries(kvm_demo PRIVATE kvm_lib)
    target_compile_definitions(kvm_demo PRIVATE
        _GNU_SOURCE
    )
    
    message(STATUS "KVM support enabled (Linux)")
else()
    message(STATUS "KVM support disabled (not Linux)")
endif()

# Libvirt support (for vm.cpp and main.cpp)
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBVIRT libvirt)
endif()

if(LIBVIRT_FOUND)
    # Main executable (requires libvirt since main.cpp includes vm.cpp)
    add_executable(augustus src/main.cpp)
    target_include_directories(augustus PRIVATE ${LIBVIRT_INCLUDE_DIRS})
    target_link_directories(augustus PRIVATE ${LIBVIRT_LIBRARY_DIRS})
    target_link_libraries(augustus PRIVATE ${LIBVIRT_LIBRARIES})
    target_compile_options(augustus PRIVATE ${LIBVIRT_CFLAGS_OTHER})
    message(STATUS "libvirt found")
    message(STATUS "  libvirt include dirs: ${LIBVIRT_INCLUDE_DIRS}")
    message(STATUS "  libvirt libraries: ${LIBVIRT_LIBRARIES}")
    
    # Add libvirt include path globally for IDE support
    include_directories(${LIBVIRT_INCLUDE_DIRS})
else()
    message(WARNING "augustus target skipped - libvirt required")
    message(WARNING "libvirt not found - augustus target will fail")
    message(STATUS "  Install libvirt: brew install libvirt")
endif()

# Optional: gRPC support (if server.cpp is implemented)
# Uncomment the following if you want to build gRPC server
# find_package(Protobuf REQUIRED)
# find_package(gRPC REQUIRED)
# 
# set(PROTO_FILES src/server.proto)
# protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
# 
# add_executable(server src/server.cpp ${PROTO_SRCS} ${PROTO_HDRS})
# target_link_libraries(server PRIVATE
#     gRPC::grpc++
#     protobuf::libprotobuf
# )

# Installation
set(INSTALL_TARGETS augustus)
if(LIBVIRT_FOUND)
    list(APPEND INSTALL_TARGETS augustus)
endif()
install(TARGETS ${INSTALL_TARGETS}
    RUNTIME DESTINATION bin
)

if(UNIX AND NOT APPLE)
    install(TARGETS kvm_demo
        RUNTIME DESTINATION bin
    )
endif()
