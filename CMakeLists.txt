CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT( pride )

# Sets where the compiled executable should be placed 
SET( EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR} )

# Sets stricter compile flags for better development 
SET( CMAKE_C_FLAGS "-g -Wall" )
SET( CMAKE_C_FLAGS_DEBUG "-g" )

IF(UNIX)
	MESSAGE( "Linux System")
	SET( BUILD_VARS " -D__UNIX" )
ENDIF()

IF(APPLE)
	MESSAGE( "OSX system" )
	SET( BUILD_VARS " -D__OSX" )
ENDIF()

# Handle the number of replicas 
SET( NUM_REPLICAS "2" CACHE TYPE STRING )
IF( NUM_REPLICAS ) 
	SET (NUM_REPLICAS "-DPRIDE_NUM_REPICAS=${NUM_REPLICAS}" ) 
ELSE( NUM_REPLICAS )
	SET (NUM_REPLICAS "-DPRIDE_NUM_REPICAS=2" )
ENDIF( NUM_REPLICAS )

FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(GLIB glib-2.0)

# Find Berkeley DB library 
FIND_PATH( BDB_INC_DIR db.h PATHS
	/opt/local/include/db47
	/usr/local/BerkeleyDB.4.8/include 
	/usr/include
	NO_DEFAULT_PATH
)

IF( NOT BDB_INC_DIR )
	MESSAGE( FATAL_ERROR "Failed to find BDB library" )
ELSE()
	MESSAGE( "FOUND BDB includes at ${BDB_INC_DIR}" )
ENDIF()

FIND_LIBRARY( BDB_LIB_DIR NAMES db PATHS /usr/local/BerkeleyDB.4.8/lib /opt/local/lib/db47  )
IF( NOT BDB_LIB_DIR )
	MESSAGE( FATAL_ERROR "Failed to find BDB lib dir")
ELSE()
	MESSAGE( "Found BDB libs in ${BDB_LIB_DIR}")
ENDIF()

ADD_DEFINITIONS(${NUM_REPLICAS} ${BUILD_VARS})
INCLUDE_DIRECTORIES( ${BDB_INC_DIR} ${GLIB_INCLUDE_DIRS} $ENV{HOME}/include) 
ADD_SUBDIRECTORY( src )
