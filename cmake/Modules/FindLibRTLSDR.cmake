<<<<<<< HEAD
INCLUDE(FindPkgConfig)
if(NOT LIBRTLSDR_FOUND)
  pkg_check_modules (LIBRTLSDR_PKG librtlsdr)
  find_path(LIBRTLSDR_INCLUDE_DIRS NAMES rtl-sdr.h
    PATHS
    ${LIBRTLSDR_PKG_INCLUDE_DIRS}
    /usr/include
    /usr/local/include
  )

  find_library(LIBRTLSDR_LIBRARIES NAMES rtlsdr
    PATHS
    ${LIBRTLSDR_PKG_LIBRARY_DIRS}
    /usr/lib
    /usr/local/lib
  )

if(LIBRTLSDR_INCLUDE_DIRS AND LIBRTLSDR_LIBRARIES)
  set(LIBRTLSDR_FOUND TRUE CACHE INTERNAL "librtlsdr found")
  message(STATUS "Found librtlsdr: ${LIBRTLSDR_INCLUDE_DIRS}, ${LIBRTLSDR_LIBRARIES}")
else(LIBRTLSDR_INCLUDE_DIRS AND LIBRTLSDR_LIBRARIES)
  set(LIBRTLSDR_FOUND FALSE CACHE INTERNAL "librtlsdr found")
  message(STATUS "librtlsdr not found.")
endif(LIBRTLSDR_INCLUDE_DIRS AND LIBRTLSDR_LIBRARIES)

mark_as_advanced(LIBRTLSDR_LIBRARIES LIBRTLSDR_INCLUDE_DIRS)

endif(NOT LIBRTLSDR_FOUND)
=======
# - Try to find LibRTLSDR
# Once done this will define
#  LIBRTLSDR_FOUND - System has LibRTLSDR
#  LIBRTLSDR_INCLUDE_DIRS - The LibRTLSDR include directories
#  LIBRTLSDR_LIBRARIES - The libraries needed to use LibRTLSDR
#  LIBRTLSDR_DEFINITIONS - Compiler switches required for using LibRTLSDR

find_package(PkgConfig)
pkg_check_modules(LIBRTLSDR_PKG QUIET librtlsdr)
set(LIBRTLSDR_DEFINITIONS ${LIBRTLSDR_PKG_CFLAGS_OTHER})

find_path(LIBRTLSDR_INCLUDE_DIR NAMES rtl-sdr.h
          HINTS ${LIBRTLSDR_PKG_INCLUDE_DIRS}
          PATHS
          /usr/include
          /usr/local/include )

find_library(LIBRTLSDR_LIBRARY NAMES rtlsdr
             HINTS ${LIBRTLSDR_PKG_LIBRARY_DIRS}
             PATHS
             /usr/lib
             /usr/local/lib )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBRTLSDR_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LibRTLSDR  DEFAULT_MSG
                                  LIBRTLSDR_LIBRARY LIBRTLSDR_INCLUDE_DIR)

mark_as_advanced(LIBRTLSDR_LIBRARY LIBRTLSDR_INCLUDE_DIR)

set(LIBRTLSDR_LIBRARIES ${LIBRTLSDR_LIBRARY} )
set(LIBRTLSDR_INCLUDE_DIRS ${LIBRTLSDR_INCLUDE_DIR} )
>>>>>>> e5a6083a0a1677f4b0a435602fc623fbbb54ecc3
