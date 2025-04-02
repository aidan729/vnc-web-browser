# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\QtBrowser_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\QtBrowser_autogen.dir\\ParseCache.txt"
  "QtBrowser_autogen"
  )
endif()
