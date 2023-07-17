# CMakeList.txt : CMake project for glPBRSampleViewer, include source and define
# project specific logic here.
#
find_package(GLEW REQUIRED)

include_directories(${SDL2_INCLUDE_DIR} ${GLM_INCLUDE_DIR} ${SPDLOG_INCLUDE_DIR} ${ASSIMP_INCLUDE_DIR} ${STB_IMAGE_H} ${JSON_H})

# Add source to this project's executable.
add_executable (glPBRSampleViewer WIN32 "main.cpp" "Renderer.h" "Renderer.cpp"   "Shader.h" "Shader.cpp" "GeometryLoader.h" "GeometryLoader.cpp" "Camera.h" "Camera.cpp"      "imgui/imconfig.h" "imgui/imgui.cpp" "imgui/imgui.h" "imgui/imgui_demo.cpp" "imgui/imgui_draw.cpp" "imgui/imgui_internal.h" "imgui/imgui_tables.cpp" "imgui/imgui_widgets.cpp" "imgui/imstb_rectpack.h" "imgui/imstb_textedit.h" "imgui/imstb_truetype.h" "imgui/imgui_impl_opengl3.cpp" "imgui/imgui_impl_opengl3.h" "imgui/imgui_impl_opengl3_loader.h" "imgui/imgui_impl_sdl2.cpp" "imgui/imgui_impl_sdl2.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET glPBRSampleViewer PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(glPBRSampleViewer ${SDL2_LIBRARY} ${ASSIMP_LIBRARY} GLEW::GLEW opengl32.lib)

# TODO: Add tests and install targets if needed.
