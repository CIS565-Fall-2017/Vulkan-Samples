function(buildSample SAMPLE_NAME)
  file(GLOB SAMPLE_SOURCES ${SAMPLE_NAME}/*.cpp ${SAMPLE_NAME}/*.h)
  file(GLOB_RECURSE SHADER_SOURCES
    ${SAMPLE_NAME}/*.vert
    ${SAMPLE_NAME}/*.frag
    ${SAMPLE_NAME}/*.geom
    ${SAMPLE_NAME}/*.comp
    ${SAMPLE_NAME}/*.tese
    ${SAMPLE_NAME}/*.tesc
  )
  foreach(SHADER_SOURCE ${SHADER_SOURCES})
    if(WIN32)
      execute_process(COMMAND $ENV{VK_SDK_PATH}/Bin/glslangValidator.exe -V ${SHADER_SOURCE} -o ${SHADER_SOURCE}.spv)
    endif(WIN32)
  endforeach()
  source_group("Shaders" FILES ${SHADER_SOURCES})

  if(WIN32)
    add_executable(${SAMPLE_NAME} WIN32 ${SAMPLE_SOURCES} ${SHADER_SOURCES})
    target_link_libraries(${SAMPLE_NAME} ${WINLIBS})
  else(WIN32)
    add_executable(${SAMPLE_NAME} ${SAMPLE_SOURCES})
    target_link_libraries(${SAMPLE_NAME} ${CMAKE_THREAD_LIBS_INIT})
  endif(WIN32)

  target_link_libraries(${SAMPLE_NAME} ${ASSIMP_LIBRARIES} Vulkan::Vulkan glfw)
  target_include_directories(${SAMPLE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/${SAMPLE_NAME}
  )
  
  ExternalTarget("samples" ${SAMPLE_NAME})
endfunction(buildSample)

buildSample(0_helloWindow)
buildSample(1_helloTriangle)
buildSample(2_helloVertexBuffers)
