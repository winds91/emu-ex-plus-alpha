if(NOT CMAKE_AR)
	set(CMAKE_AR llvm-ar)
endif()

# needed for DelegateFuncSet.hh and external libc++
string(APPEND CXXFLAGS " -Wno-vla-cxx-extension -Wno-reserved-module-identifier")

include("${CMAKE_CURRENT_LIST_DIR}/compiler-common.cmake")
