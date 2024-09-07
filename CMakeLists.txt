
cmake_minimum_required(VERSION 3.5.0)
project(CGraph-lite)

set(CMAKE_CXX_STANDARD 14)

set(CGRAPH_LITE_TUTORIAL_LIST
    T01-Simple
    T02-Param
)

foreach(tut ${CGRAPH_LITE_TUTORIAL_LIST})
    add_executable(${tut}
        ${tut}.cpp
    )
endforeach()
