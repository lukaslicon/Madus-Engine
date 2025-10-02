add_executable(MadusSandbox src/main.cpp)
target_link_libraries(MadusSandbox PRIVATE Madus)

set_target_properties(MadusSandbox PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Sandbox")

add_custom_target(run
  COMMAND $<TARGET_FILE:MadusSandbox>
  DEPENDS MadusSandbox
  WORKING_DIRECTORY $<TARGET_FILE_DIR:MadusSandbox>)

add_custom_command(TARGET MadusSandbox POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/Sandbox/assets
          $<TARGET_FILE_DIR:MadusSandbox>/assets)
