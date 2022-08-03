file(GLOB SOURCE_FILES_ "*.cpp")

file(GLOB ZLIB_SOURCE_FILES "${TPD_PATH}/zlib/*.c")

set(CORE_SOURCE_FILES
  ${ZLIB_SOURCE_FILES}
  ${SOURCE_FILES_}
)

if((NOT NO_OBJECTS) OR XN_STATIC_LIB)
  build_target(
    NAME
    xeno-objects
    TYPE
    OBJECT
    SOURCES
    ${CORE_SOURCE_FILES}
    LINKS
    precore-objects
    NO_VERINFO
    NO_PROJECT_H)

  set_target_properties(xeno-objects PROPERTIES POSITION_INDEPENDENT_CODE
                                                ${OBJECTS_PID})
  target_compile_options(xeno-objects PRIVATE -fvisibility=hidden)

  target_include_directories(
    xeno-objects
    PUBLIC ../include
    PRIVATE . ${TPD_PATH}/zlib)
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
    LINKS
    precore
    START_YEAR
    2017
    AUTHOR
    "Lukas Cone"
    NO_PROJECT_H)

  target_include_directories(
    xeno
    PUBLIC ../include
    PRIVATE . ${TPD_PATH}/zlib)

  target_compile_definitions(
    xeno
    PRIVATE XN_EXPORT REF_EXPORT
    INTERFACE XN_IMPORT)

  install(
    TARGETS xeno
    LIBRARY NAMELINK_SKIP DESTINATION $<IF:$<BOOL:${MINGW}>,bin,lib>
    RUNTIME DESTINATION bin)
endif()
