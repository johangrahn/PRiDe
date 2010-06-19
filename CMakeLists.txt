cmake_minimum_required(VERSION 2.6)

project( pride )

# Sets where the compiled executable should be placed 
set( EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR} )

# Sets stricter compile flags for better development 
set( CMAKE_C_FLAGS "-g -Wall" )
set( CMAKE_C_FLAGS_DEBUG "-g" )

find_package(PkgConfig)
pkg_check_modules(GLIB glib-2.0)

# Find Berkeley DB library 
find_path( BDB_INC_DIR db.h PATHS
	/opt/local/include/db47
	/usr/local/BerkeleyDB.4.8/include 
	/usr/include
	NO_DEFAULT_PATH
)

if( NOT BDB_INC_DIR )
	message( FATAL_ERROR "Failed to find BDB library" )
else()
	message( "FOUND BDB library at ${BDB_INC_DIR}" )
endif()

find_library( BDB_LIB_DIR NAMES db PATHS /usr/local/BerkeleyDB.4.8/lib /opt/local/lib/db47  )
if( NOT BDB_LIB_DIR )
	message( FATAL_ERROR "Failed to find BDB lib dir")
endif()

include_directories( ${BDB_INC_DIR} ${GLIB_INCLUDE_DIRS} )
add_subdirectory( src )
