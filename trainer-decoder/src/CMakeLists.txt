# REMEMBER TO COMPILE WITH
# -DCMAKE_CXX_COMPILER_ARG1=-std=c++11
cmake_minimum_required(VERSION 2.8)
project(packet_decoder)

# WIN32 SPECIFICS
if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# SUB PROJECTS
set(LIBGURRA_BUILD_FFMPEG TRUE)
set(LIBGURRA_BUILD_SHM TRUE)
add_subdirectory(libgurra)

# SOURCES
set(    SOURCES
        DecodeServer
        packet_decoder
        ShmManager
)

# INCLUDE AND LINK DIRECTORIES
include_directories(    ${CMAKE_CURRENT_BINARY_DIR}
                        ${CMAKE_SOURCE_DIR}/libgurra/	)
link_directories(       ${libgurra_LINK_DIRS})

# SET UP SOURCES TO BUILD
add_executable(${PROJECT_NAME} ${SOURCES})
	
# LINK LIBRARIES
target_link_libraries(  ${PROJECT_NAME} 
                        libgurra	)


