cmake_minimum_required(VERSION 3.1.0)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/External)

set(CMAKE_CXX_STANDARD 17)

set(GENERIC_SRC_StormRefl
            ./StormReflOutput.cpp
            ./StaticLibs.cpp
            ./main.cpp
            #CPP_PLACEHOLDER
            )
set(GENERIC_HEADER_StormRefl
            ./StormReflMetaCall.h
            ./StormReflMetaCallJsonHelpers.h
            ./StormReflMetaCallJson.h
            ./StormReflMetaCallHelpers.h
            ./StormReflOutput.h
            ./StormRefl.h
            ./StormReflMetaFuncs.h
            ./StormReflMetaEnum.h
            ./StormReflMetaEnumHelpers.h
            ./StormReflJson.h
            ./StormReflMetaHelpers.h
            ./StormReflMetaInfoBase.h
            ./StormReflJsonStd.h
            #HEADER_PLACEHOLDER
            )


add_library(StormRefl ${GENERIC_SRC_StormRefl} ${GENERIC_HEADER_StormRefl})
