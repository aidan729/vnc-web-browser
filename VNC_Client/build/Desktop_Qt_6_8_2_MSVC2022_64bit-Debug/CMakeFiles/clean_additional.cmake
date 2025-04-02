# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\VNCClient_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\VNCClient_autogen.dir\\ParseCache.txt"
  "VNCClient_autogen"
  )
endif()
