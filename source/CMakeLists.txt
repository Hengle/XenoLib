file(GLOB SOURCE_FILES_ "*.cpp")

file(GLOB ZLIB_SOURCE_FILES "${TPD_PATH}/zlib/*.c")
file(GLOB_RECURSE ZSTD_SOURCE_FILES "${TPD_PATH}/zstd/lib/*.c")

enable_language(ASM)

set(CORE_SOURCE_FILES
  ${ZLIB_SOURCE_FILES}
  ${SOURCE_FILES_}
  ${ZSTD_SOURCE_FILES}
  ${TPD_PATH}/zstd/lib/decompress/huf_decompress_amd64.S
)

add_library(xeno-interface INTERFACE)
target_include_directories(xeno-interface INTERFACE ../include)
target_link_libraries(xeno-interface INTERFACE spike-interface)

include(targetex)

if((NOT NO_OBJECTS) OR XN_STATIC_LIB)
  build_target(
    NAME
    xeno-objects
    TYPE
    OBJECT
    SOURCES
    ${CORE_SOURCE_FILES}
    INCLUDES
    .
    ${TPD_PATH}/zlib
    ${TPD_PATH}/zstd/lib/common
    ${TPD_PATH}/zstd/lib
    LINKS
    spike-interface
    xeno-interface
    NO_VERINFO
    NO_PROJECT_H)

  set_target_properties(xeno-objects PROPERTIES POSITION_INDEPENDENT_CODE
                                                ${OBJECTS_PID})
  target_compile_options(xeno-objects PRIVATE -fvisibility=hidden)
  target_compile_definitions(
    xeno-objects PRIVATE
    ZSTDLIB_VISIBLE=__attribute__\(\(visibility\(\"hidden\"\)\)\)
    ZSTDLIB_HIDDEN=__attribute__\(\(visibility\(\"hidden\"\)\)\)
    ZSTDERRORLIB_VISIBILITY=__attribute__\(\(visibility\(\"hidden\"\)\)\)
  )

  target_expose_defs(xeno XN_EXPORT XN_IMPORT)
endif()

if(XN_STATIC_LIB)
  build_target(
    NAME
    xeno-static
    PROPERTIES
    OUTPUT_NAME
    xeno
    TYPE
    STATIC
    LINKS
    xeno-objects
    NO_VERINFO
    NO_PROJECT_H)
endif()

if(BUILD_SHARED_LIBS)
  build_target(
    NAME
    xeno
    TYPE
    SHARED
    SOURCES
    ${CORE_SOURCE_FILES}
    INCLUDES
    .
    ${TPD_PATH}/zlib
    ${TPD_PATH}/zstd/lib/common
    ${TPD_PATH}/zstd/lib
    LINKS
    spike
    xeno-interface
    START_YEAR
    2017
    AUTHOR
    "Lukas Cone"
    NO_PROJECT_H)

  target_compile_definitions(
    xeno
    PRIVATE XN_EXPORT
    INTERFACE XN_IMPORT)

  if(WIN32 OR MINGW)
    set_target_properties(xeno PROPERTIES SUFFIX ${PROJECT_VERSION_MAJOR}.dll
                                          PREFIX "")
  else()
    set_target_properties(xeno PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
  endif()

  install(
    TARGETS xeno
    LIBRARY NAMELINK_SKIP DESTINATION $<IF:$<BOOL:${MINGW}>,bin,lib>
    RUNTIME DESTINATION bin)
endif()
