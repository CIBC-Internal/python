CMAKE_MINIMUM_REQUIRED(VERSION 2.8 FATAL_ERROR)

PROJECT(Python)

# TODO: add zlib support eventually
#IF(NOT SCI_ZLIB_LIBRARY)
#  MESSAGE(FATAL_ERROR "zlib library is required to compile Python.")
#ENDIF()


##########################################################################
# Version information                                                    #
##########################################################################
FILE(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/Include/patchlevel.h PY_MAJOR_VERSIONSTR
  REGEX "#define[ \t]+PY_MAJOR_VERSION[ \t]+[0-9]+")
STRING(REGEX MATCH "[0-9]+" PY_MAJOR_VERSIONSTR ${PY_MAJOR_VERSIONSTR})
STRING(LENGTH ${PY_MAJOR_VERSIONSTR} PY_MAJOR_VERSIONSTR_LEN)

FILE(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/Include/patchlevel.h PY_MINOR_VERSIONSTR
  REGEX "#define[ \t]+PY_MINOR_VERSION[ \t]+[0-9]+")
STRING(REGEX MATCH "[0-9]+" PY_MINOR_VERSIONSTR ${PY_MINOR_VERSIONSTR})
STRING(LENGTH ${PY_MINOR_VERSIONSTR} PY_MINOR_VERSIONSTR_LEN)

FILE(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/Include/patchlevel.h PY_MICRO_VERSIONSTR
  REGEX "#define[ \t]+PY_MICRO_VERSION[ \t]+[0-9]+")
STRING(REGEX MATCH "[0-9]+" PY_MICRO_VERSIONSTR ${PY_MICRO_VERSIONSTR})
STRING(LENGTH ${PY_MICRO_VERSIONSTR} PY_MICRO_VERSIONSTR_LEN)

IF(PY_MAJOR_VERSIONSTR_LEN AND PY_MINOR_VERSIONSTR_LEN AND PY_MICRO_VERSIONSTR_LEN)
  MATH(EXPR PY_MAJOR "${PY_MAJOR_VERSIONSTR}")
  MATH(EXPR PY_MINOR "${PY_MINOR_VERSIONSTR}")
  MATH(EXPR PY_PATCH "${PY_MICRO_VERSIONSTR}")
  SET(PYTHON_VERSION "${PY_MAJOR}.${PY_MINOR}.${PY_PATCH}")
  MESSAGE(STATUS "Python version ${PYTHON_VERSION}")
ELSE()
  MESSAGE(FATAL_ERROR "Unable to parse Python version from ${CMAKE_CURRENT_SOURCE_DIR}/Include/patchlevel.h")
ENDIF()

# user configurable OPTIONs

OPTION(WITH_TSC "profile with the Pentium timestamp counter" 0)
OPTION(ENABLE_IPV6 "Enable IP v6" 1)
OPTION(WITH_DOC_STRINGS "Enable if you want documentation strings in extension modules" 1)
OPTION(Py_USING_UNICODE "Enable unicode support" 1)
OPTION(Py_DEBUG "Build debugging interpreter" 0)
OPTION(WITH_PYMALLOC "Define if you want to compile in Python-specific mallocs" 1)
OPTION(HAVE_LARGEFILE_SUPPORT "Large file support "1)
OPTION(WITH_THREAD "compile in rudimentary thread support" 1)

MARK_AS_ADVANCED(FORCE WITH_TSC ENABLE_IPV6 WITH_DOC_STRINGS Py_USING_UNICODE 
        Py_DEBUG WITH_PYMALLOC HAVE_LARGEFILE_SUPPORT WITH_THREAD)

IF(NOT Py_DEBUG)
  ADD_DEFINITIONS(-DNDEBUG)
ENDIF()

# do the configure checks

INCLUDE(ConfigureChecks.cmake)

SET(extraLibs "")

SET(THREAD_SRC)
IF(WITH_THREAD)
  IF(HAVE_LIBPTHREAD)
    SET(THREAD_SRC Python/thread.c)
    SET(_REENTRANT 1)
    SET(extraLibs ${extraLibs} ${HAVE_LIBPTHREAD})
  endIF(HAVE_LIBPTHREAD)
  IF(WIN32)
    SET(THREAD_SRC Python/thread.c Python/thread_nt.h)
  ENDIF()
ENDIF()

IF(HAVE_LIBM)
  SET(extraLibs ${extraLibs} ${HAVE_LIBM})
ENDIF()

IF(HAVE_LIBUTIL)
  SET(extraLibs ${extraLibs} ${HAVE_LIBUTIL})
ENDIF()

SET(DYNLOAD_SRC)
IF(HAVE_DYNAMIC_LOADING)
  IF(HAVE_LIBDL)
    SET(extraLibs ${extraLibs} ${HAVE_LIBDL})
    SET(DYNLOAD_SRC ${DYNLOAD_SRC} Python/dynload_shlib.c)
  ENDIF()
  IF(WIN32)
    SET(DYNLOAD_SRC ${DYNLOAD_SRC} Python/dynload_win.c PC/import_nt.c PC/dl_nt.c)
  ENDIF()
#  SET(BUILD_LIBS_SHARED_DEFAULT TRUE)
ELSE()
#  SET(BUILD_LIBS_SHARED_DEFAULT FALSE)
ENDIF()

SET(BUILD_LIBS_SHARED_DEFAULT FALSE)

#OPTION(PYTHON_BUILD_LIB_SHARED "Build libpython shared" ${BUILD_LIBS_SHARED_DEFAULT})
#IF(PYTHON_BUILD_LIB_SHARED)
#  SET(LIB_BUILD_TYPE SHARED)
#ELSE()
  SET(LIB_BUILD_TYPE STATIC)
#ENDIF()

SET(MACHDEP_SRCS )   #TODO: configure

SET(SIGNAL_SRCS )    #TODO: configure

SET(MOD_SRCS )


#####################################
#
# setup modules
#
#####################################

SET(STATIC_MODULES)
SET(DYNAMIC_MODULES)

MACRO(ADD_PYTHON_MODULE _NAME _STATIC_SRCS)
  #OPTION(MODULE_${_NAME}_ENABLE "Add module ${_NAME}" TRUE)
  #MARK_AS_ADVANCED(FORCE MODULE_${_NAME}_ENABLE)
  SET(MODULE_${_NAME}_ENABLE TRUE)

  IF(MODULE_${_NAME}_ENABLE)
    #OPTION(MODULE_${_NAME}_SHARED "Add module ${_NAME} shared" ${BUILD_LIBS_SHARED_DEFAULT})
  SET(MODULE_${_NAME}_SHARED FALSE)
    IF(MODULE_${_NAME}_SHARED)
      MESSAGE(STATUS "Adding ${_NAME} as shared module")
      SET(DYNAMIC_MODULES ${DYNAMIC_MODULES} ${_NAME})
      ADD_LIBRARY(${_NAME} MODULE ${ARGN})
      set_target_properties(${_NAME} PROPERTIES PREFIX "")
    ELSE()
      MESSAGE(STATUS "Adding ${_NAME} as static module")
      SET(STATIC_MODULES ${STATIC_MODULES} ${_NAME})
      SET(${_STATIC_SRCS} ${${_STATIC_SRCS}} ${ARGN})
      SET(PY_STATIC_MODULES_INIT_FUNCTIONS "${PY_STATIC_MODULES_INIT_FUNCTIONS}extern PyObject* PyInit_${_NAME}(void);\n")
      SET(PY_STATIC_MODULES_INITTAB        "${PY_STATIC_MODULES_INITTAB}  {\"${_NAME}\", PyInit_${_NAME}},\n")
    ENDIF()
  ENDIF()
ENDMACRO()

IF(WITH_THREAD)
  ADD_PYTHON_MODULE(_thread  MOD_SRCS Modules/_threadmodule.c)
ENDIF()

IF(WIN32)
  SET(PY_POSIX_NAME nt)
ELSE()
  SET(PY_POSIX_NAME posix)
ENDIF()

# _io module
ADD_PYTHON_MODULE(_io  MOD_SRCS Modules/_io/_iomodule.c)
SET(MOD_SRCS ${MOD_SRCS}
  Modules/_io/bufferedio.c
  Modules/_io/bytesio.c
  Modules/_io/fileio.c
  Modules/_io/iobase.c
  Modules/_io/stringio.c
  Modules/_io/textio.c
)

# from setup.py
ADD_PYTHON_MODULE(cmath MOD_SRCS Modules/cmathmodule.c)
SET(MOD_SRCS ${MOD_SRCS}
  Modules/_math.c
)
# from setup.py
# requires Modules/_math.c, which was added when configuring cmathmodule
ADD_PYTHON_MODULE(math MOD_SRCS Modules/mathmodule.c)

#_multiprocessing module
ADD_PYTHON_MODULE(_multiprocessing  MOD_SRCS Modules/_multiprocessing/multiprocessing.c)

IF(WIN32)
  SET(MOD_SRCS ${MOD_SRCS} Modules/_multiprocessing/semaphore.c)
  SET(extraLibs ${extraLibs} Ws2_32.lib)
ELSEIF(HAVE_SEM_OPEN AND NOT POSIX_SEMAPHORES_NOT_ENABLED)
  # Probably only a problem on some FreeBSD and Solaris distributions.
  # If needed, there is a test for POSIX_SEMAPHORES_NOT_ENABLED in configure.ac.
  SET(MOD_SRCS ${MOD_SRCS} Modules/_multiprocessing/semaphore.c)
ENDIF()
      
# TODO: enable this once zlib support is available in SCIRun prototype
# zlib module
IF(SCI_ZLIB_LIBRARY)
  ADD_PYTHON_MODULE(zlib MOD_SRCS Modules/zlibmodule.c)
  INCLUDE_DIRECTORIES(${SCI_ZLIB_INCLUDE})
ENDIF()
      
#ADD_PYTHON_MODULE(audioop       MOD_SRCS Modules/audioop.c)
      
ADD_PYTHON_MODULE(atexit        MOD_SRCS Modules/atexitmodule.c)
ADD_PYTHON_MODULE(_pickle       MOD_SRCS Modules/_pickle.c)
ADD_PYTHON_MODULE(array         MOD_SRCS Modules/arraymodule.c)
ADD_PYTHON_MODULE(binascii      MOD_SRCS Modules/binascii.c)
ADD_PYTHON_MODULE(errno       MOD_SRCS Modules/errnomodule.c)
ADD_PYTHON_MODULE(${PY_POSIX_NAME}  MOD_SRCS Modules/posixmodule.c)
ADD_PYTHON_MODULE(operator      MOD_SRCS Modules/operator.c)
ADD_PYTHON_MODULE(signal      MOD_SRCS Modules/signalmodule.c)
ADD_PYTHON_MODULE(_md5      MOD_SRCS Modules/md5module.c)
ADD_PYTHON_MODULE(_sha1      MOD_SRCS Modules/sha1module.c)
ADD_PYTHON_MODULE(_sha256    MOD_SRCS Modules/sha256module.c)
ADD_PYTHON_MODULE(_sha512    MOD_SRCS Modules/sha512module.c)
ADD_PYTHON_MODULE(time      MOD_SRCS Modules/timemodule.c)
ADD_PYTHON_MODULE(_locale    MOD_SRCS Modules/_localemodule.c)
ADD_PYTHON_MODULE(_codecs     MOD_SRCS Modules/_codecsmodule.c)
ADD_PYTHON_MODULE(_weakref    MOD_SRCS Modules/_weakref.c)
ADD_PYTHON_MODULE(_random       MOD_SRCS Modules/_randommodule.c)
ADD_PYTHON_MODULE(_bisect       MOD_SRCS Modules/_bisectmodule.c)
ADD_PYTHON_MODULE(_heapq        MOD_SRCS Modules/_heapqmodule.c)
ADD_PYTHON_MODULE(_lsprof       MOD_SRCS Modules/_lsprof.c)
ADD_PYTHON_MODULE(itertools     MOD_SRCS Modules/itertoolsmodule.c)
ADD_PYTHON_MODULE(_collections  MOD_SRCS Modules/_collectionsmodule.c)
ADD_PYTHON_MODULE(_symtable   MOD_SRCS Modules/symtablemodule.c)
ADD_PYTHON_MODULE(mmap       MOD_SRCS Modules/mmapmodule.c)
ADD_PYTHON_MODULE(_csv          MOD_SRCS Modules/_csv.c)
ADD_PYTHON_MODULE(_sre        MOD_SRCS Modules/_sre.c)
ADD_PYTHON_MODULE(parser        MOD_SRCS Modules/parsermodule.c)
ADD_PYTHON_MODULE(_struct       MOD_SRCS Modules/_struct.c)
ADD_PYTHON_MODULE(_datetime      MOD_SRCS Modules/_datetimemodule.c)
ADD_PYTHON_MODULE(_functools    MOD_SRCS Modules/_functoolsmodule.c)
ADD_PYTHON_MODULE(_json        MOD_SRCS Modules/_json.c )
ADD_PYTHON_MODULE(xxsubtype   MOD_SRCS Modules/xxsubtype.c)
ADD_PYTHON_MODULE(zipimport   MOD_SRCS Modules/zipimport.c)
ADD_PYTHON_MODULE(select    MOD_SRCS Modules/selectmodule.c)
ADD_PYTHON_MODULE(faulthandler MOD_SRCS Modules/faulthandler.c)

# TODO: look at Modules/_freeze_importlib - not built by default on OS X

IF(WIN32)
  ADD_PYTHON_MODULE(msvcrt    MOD_SRCS PC/msvcrtmodule.c)
  ADD_PYTHON_MODULE(winreg    MOD_SRCS PC/winreg.c)
ELSE()
  ADD_PYTHON_MODULE(pwd    MOD_SRCS Modules/pwdmodule.c)
  ADD_PYTHON_MODULE(_posixsubprocess MOD_SRCS Modules/_posixsubprocess.c)

#OPTION(BUILD_UNTESTED_MODULES "Build extra modules" ON)
#MARK_AS_ADVANCED(FORCE BUILD_UNTESTED_MODULES)
#IF(BUILD_UNTESTED_MODULES)
  #ADD_PYTHON_MODULE(_elementtree  MOD_SRCS Modules/_elementtree.c )

  ADD_PYTHON_MODULE(fcntl         MOD_SRCS Modules/fcntlmodule.c )
  ADD_PYTHON_MODULE(grp           MOD_SRCS Modules/grpmodule.c )

  # extension modules
  # Stefan Krah's _decimal module
  ADD_PYTHON_MODULE(_decimal DECIMAL_MOD_SRCS Modules/_decimal/_decimal.c)
  SET(DECIMAL_MOD_SRCS ${DECIMAL_MOD_SRCS}
    Modules/_decimal/libmpdec/basearith.c
    Modules/_decimal/libmpdec/constants.c
    Modules/_decimal/libmpdec/context.c
    Modules/_decimal/libmpdec/convolute.c
    Modules/_decimal/libmpdec/crt.c
    Modules/_decimal/libmpdec/difradix2.c
    Modules/_decimal/libmpdec/fnt.c
    Modules/_decimal/libmpdec/fourstep.c
    Modules/_decimal/libmpdec/io.c
    Modules/_decimal/libmpdec/memory.c
    Modules/_decimal/libmpdec/mpdecimal.c
    Modules/_decimal/libmpdec/numbertheory.c
    Modules/_decimal/libmpdec/sixstep.c
    Modules/_decimal/libmpdec/transpose.c
  )

  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/Modules/_decimal/libmpdec)

  # See setup.py for other configuration options
  # Windows flags from PCbuild/pcbuild.sln
  IF(CMAKE_SIZEOF_VOID_P MATCHES  8)
    IF(WIN32)
      SET_PROPERTY(SOURCE ${DECIMAL_MOD_SRCS}
        PROPERTY
          COMPILE_DEFINITIONS CONFIG_64=1 MASM=1)
    ELSE()
      SET_PROPERTY(SOURCE ${DECIMAL_MOD_SRCS}
        PROPERTY
          COMPILE_DEFINITIONS CONFIG_64=1 ASM=1)
    ENDIF()
  ELSE()
    IF(WIN32)
     # /DPPRO?
      SET_PROPERTY(SOURCE ${DECIMAL_MOD_SRCS}
        PROPERTY
          COMPILE_DEFINITIONS CONFIG_32=1 PPRO=1 MASM=1)
    ELSE()
      SET_PROPERTY(SOURCE ${DECIMAL_MOD_SRCS}
        PROPERTY
          COMPILE_DEFINITIONS CONFIG_32=1 ASM=1)
    ENDIF()
  ENDIF()

  SET(MOD_SRCS ${MOD_SRCS} ${DECIMAL_MOD_SRCS})

#ENDIF()
ENDIF()

#CJK codecs
ADD_PYTHON_MODULE(_multibytecodec   MOD_SRCS Modules/cjkcodecs/multibytecodec.c)
ADD_PYTHON_MODULE(_codecs_cn     MOD_SRCS Modules/cjkcodecs/_codecs_cn.c)
ADD_PYTHON_MODULE(_codecs_hk     MOD_SRCS Modules/cjkcodecs/_codecs_hk.c)
ADD_PYTHON_MODULE(_codecs_iso2022   MOD_SRCS Modules/cjkcodecs/_codecs_iso2022.c)
ADD_PYTHON_MODULE(_codecs_jp     MOD_SRCS Modules/cjkcodecs/_codecs_jp.c)
ADD_PYTHON_MODULE(_codecs_kr     MOD_SRCS Modules/cjkcodecs/_codecs_kr.c)
ADD_PYTHON_MODULE(_codecs_tw     MOD_SRCS Modules/cjkcodecs/_codecs_tw.c)

IF(Py_USING_UNICODE)
  ADD_PYTHON_MODULE(_string MOD_SRCS Objects/unicodeobject.c)
  SET(MOD_SRCS ${MOD_SRCS} Objects/unicodectype.c)
ENDIF()

# TODO: module _ctypes

FILE(MAKE_DIRECTORY ${Python_BINARY_DIR}/Modules)
# PC/pyconfig.h used for Windows builds, contains macro definitions for types missing on Windows
IF(WIN32)
  CONFIGURE_FILE(PC/pyconfig.h ${Python_BINARY_DIR}/pyconfig.h COPYONLY)
ELSE()
  CONFIGURE_FILE(pyconfig.h.cmake ${Python_BINARY_DIR}/pyconfig.h @ONLY)
ENDIF()

CONFIGURE_FILE(Modules/config.c.cmake ${CMAKE_CURRENT_BINARY_DIR}/Modules/modules-config.c @ONLY)
FILE(WRITE ${Python_BINARY_DIR}/Modules/Setup "#created by cmake, do not edit\n")


#####################################
#
# build python and libpython
# 
#####################################

SET(P_SRCS Parser/acceler.c
  Parser/grammar1.c
  Parser/listnode.c
  Parser/node.c
  Parser/parser.c
  Parser/bitset.c
  Parser/metagrammar.c
  Parser/firstsets.c
  Parser/grammar.c
  Parser/pgen.c
)

SET(PARSER_SRCS ${P_SRCS} Parser/myreadline.c Parser/parsetok.c Parser/tokenizer.c)

SET(OBJECT_SRCS Objects/abstract.c
  Objects/accu.c
  Objects/boolobject.c
  Objects/bytearrayobject.c
  Objects/bytes_methods.c
  Objects/bytesobject.c
  Objects/capsule.c
  Objects/cellobject.c
  Objects/classobject.c
  Objects/codeobject.c
  Objects/complexobject.c
  Objects/descrobject.c
  Objects/dictobject.c
  Objects/enumobject.c
  Objects/exceptions.c
  Objects/fileobject.c
  Objects/floatobject.c
  Objects/frameobject.c
  Objects/funcobject.c
  Objects/genobject.c
  Objects/iterobject.c
  Objects/listobject.c
  Objects/longobject.c
  Objects/memoryobject.c
  Objects/methodobject.c
  Objects/moduleobject.c
  Objects/namespaceobject.c
  Objects/object.c
  Objects/obmalloc.c
  Objects/rangeobject.c
  Objects/setobject.c
  Objects/sliceobject.c
  Objects/structseq.c
  Objects/tupleobject.c
  Objects/typeobject.c
  Objects/weakrefobject.c
)

SET(PYTHON_SRCS Python/Python-ast.c 
  Python/_warnings.c
  Python/asdl.c 
  Python/ast.c 
  Python/bltinmodule.c 
  Python/ceval.c 
  Python/compile.c 
  Python/codecs.c 
  Python/dynamic_annotations.c
  Python/dtoa.c
  Python/errors.c 
  Python/fileutils.c
  Python/formatter_unicode.c
  Python/frozen.c 
  Python/frozenmain.c 
  Python/future.c 
  Python/getargs.c 
  Python/getcompiler.c 
  Python/getcopyright.c 
  Python/getopt.c 
  Python/getplatform.c 
  Python/getversion.c 
  Python/graminit.c 
  Python/import.c 
  Python/importdl.c 
  Python/importdl.h
  Python/marshal.c 
  Python/modsupport.c 
  Python/mystrtoul.c 
  Python/mysnprintf.c 
  Python/peephole.c
  Python/pyarena.c 
  Python/pyctype.c
  Python/pyfpe.c 
  Python/pymath.c
  Python/pystate.c 
  Python/pystrcmp.c
  Python/pystrtod.c 
  Python/pythonrun.c
  Python/pytime.c
  Python/random.c
  Python/structmember.c 
  Python/symtable.c 
  Python/sysmodule.c 
  Python/traceback.c 
  ${MACHDEP_SRCS}
  ${THREAD_SRC}
  ${DYNLOAD_SRC}
)

SET(MODULE_SRCS ${CMAKE_CURRENT_BINARY_DIR}/Modules/modules-config.c
  Modules/getbuildinfo.c 
  Modules/main.c 
  Modules/gcmodule.c
  Modules/rotatingtree.c
  ${MOD_SRCS}
)

IF(WIN32)
  SET(MODULE_SRCS ${MODULE_SRCS}
          #PC/config.c
          PC/getpathp.c)
#  SET_SOURCE_FILES_PROPERTIES(PC/getpathp.c PROPERTIES 
#          COMPILE_FLAGS  "-DLANDMARK=L\\\"python\\\\lib\\\\os.py\\\" -DPYTHONPATH=L\\\".\\\\python\\\\lib\\\" ")
ELSE()
  SET(MODULE_SRCS ${MODULE_SRCS}
          Modules/getpath.c)
#  SET_SOURCE_FILES_PROPERTIES(Modules/getpath.c PROPERTIES COMPILE_FLAGS  -DVERSION=\\\"${PY_MAJOR}.${PY_MINOR}\\\")
    IF(APPLE AND ${CMAKE_GENERATOR} MATCHES "Xcode")
        SET_SOURCE_FILES_PROPERTIES(Python/getplatform.c PROPERTIES COMPILE_FLAGS -DPLATFORM="\\\\\"${PY_PLATFORM}\\\\\"")
    ELSE()
        SET_SOURCE_FILES_PROPERTIES(Python/getplatform.c PROPERTIES COMPILE_FLAGS -DPLATFORM="\\\"${PY_PLATFORM}\\\"")
    ENDIF()
ENDIF()        

SET(SCI_PYTHON_LIBRARY python CACHE INTERNAL "Python library." FORCE)
SET(SCI_PYTHON_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/Include ${Python_BINARY_DIR} CACHE INTERNAL "Python include directory." FORCE)

INCLUDE_DIRECTORIES(Include ${Python_BINARY_DIR})
IF(WIN32)
  INCLUDE_DIRECTORIES(PC Python)
  ADD_DEFINITIONS(-DWIN32 -D_WINDOWS)
ENDIF()

ADD_DEFINITIONS(-DPy_BUILD_CORE)
ADD_DEFINITIONS(-DPy_NO_ENABLE_SHARED)

IF(SCI_ZLIB_MANGLE)
  ADD_DEFINITIONS(-DZ_PREFIX)
ENDIF()

#IF(NOT HAVE_HYPOT)
#  ADD_DEFINITIONS(-DWITHOUT_COMPLEX)
#ENDIF()

ADD_LIBRARY(${SCI_PYTHON_LIBRARY}  STATIC
  ${PARSER_SRCS}
  ${OBJECT_SRCS}
  ${PYTHON_SRCS}
  ${MODULE_SRCS}
  ${SIGNAL_SRCS}
)

TARGET_LINK_LIBRARIES(${SCI_PYTHON_LIBRARY} ${extraLibs} )
IF(SCI_ZLIB_LIBRARY)
  TARGET_LINK_LIBRARIES(${SCI_PYTHON_LIBRARY} ${SCI_ZLIB_LIBRARY})
ENDIF()

#set_target_properties(pythonLib PROPERTIES OUTPUT_NAME python${PY_MAJOR}.${PY_MINOR})

# overridden by top-level build when included as an external project
SET(PYTHONLIB_DST_PATH "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/lib/" CACHE PATH "python module destination directory.")

IF(WIN32)
  SET_TARGET_PROPERTIES(${SCI_PYTHON_LIBRARY}
            PROPERTIES
            COMPILE_FLAGS "-W3 -wd4267 -wd4244")

  FILE(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/pythonlib.zip" PYTHONLIB_SOURCE_PATH)
  FILE(TO_NATIVE_PATH "${PYTHONLIB_DST_PATH}" PYTHONLIB_DST_NATIVE_PATH)

  ADD_CUSTOM_COMMAND(TARGET ${SCI_PYTHON_LIBRARY}
       POST_BUILD
       COMMAND if not exist "${PYTHONLIB_DST_NATIVE_PATH}" md "${PYTHONLIB_DST_NATIVE_PATH}"
       COMMAND copy /Y "${PYTHONLIB_SOURCE_PATH}" "${PYTHONLIB_DST_NATIVE_PATH}")
ELSE()
  MESSAGE(STATUS "Copying ${CMAKE_CURRENT_SOURCE_DIR}/pythonlib.zip to ${PYTHONLIB_DST_PATH}")
  ADD_CUSTOM_COMMAND(TARGET ${SCI_PYTHON_LIBRARY}
       POST_BUILD
       COMMAND [ -d ${PYTHONLIB_DST_PATH} ] || mkdir -p "${PYTHONLIB_DST_PATH}" 
       #COMMAND cp -rf "${CMAKE_CURRENT_SOURCE_DIR}/pythonlib.zip" "${PYTHONLIB_DST_PATH}")       
       COMMAND cp -rf "${CMAKE_CURRENT_SOURCE_DIR}/Lib/*" "${PYTHONLIB_DST_PATH}")       
ENDIF()

SET(PYTHON_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "" FORCE)
SET(PYTHON_MODULE_SEARCH_PATH "./lib/pythonlib.zip" CACHE INTERNAL "Relative path to pythonlib.zip file." FORCE)

OPTION(BUILD_PYTHON_INTERPRETER "Build the python interpreter executable." OFF)
IF(BUILD_PYTHON_INTERPRETER)
  ADD_DEFINITIONS(-DPYTHONPATH=L"${PYTHON_MODULE_SEARCH_PATH}")
  ADD_EXECUTABLE(python3 pythonmain.cc)
  TARGET_LINK_LIBRARIES(python3 ${SCI_PYTHON_LIBRARY})
ENDIF()
