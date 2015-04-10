set(MODULE TRUE)

set(LLVM_LINK_COMPONENTS support mc)

add_clang_library(ReportMemoryUsage ReportMemoryUsage.cpp)

target_link_libraries(ReportMemoryUsage
  clangFrontend
  clangAST
  )

set_target_properties(ReportMemoryUsage
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "")
