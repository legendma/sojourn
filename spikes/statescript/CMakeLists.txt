cmake_minimum_required( VERSION 3.16 )

project( StateScript LANGUAGES CXX )

set( SOURCE_ROOT_DIR 
     ${CMAKE_SOURCE_DIR}/src
   )

set( SOURCE_FILES
     ${SOURCE_ROOT_DIR}/pch.cpp
     ${SOURCE_ROOT_DIR}/main.cpp
     ${SOURCE_ROOT_DIR}/app.cpp
     ${SOURCE_ROOT_DIR}/directx12.cpp
     ${SOURCE_ROOT_DIR}/msw.cpp
     ${SOURCE_ROOT_DIR}/statescript.cpp
   )

set( INCLUDE_ROOT_DIR
     ${SOURCE_ROOT_DIR}/include
   )

set( LIB_DIR ${CMAKE_SOURCE_DIR}/lib )
set( IMGUI_DIR ${LIB_DIR}/imgui )
set( IMGUI_FILE_DLG_EXT ${LIB_DIR}/imgui_filedialog )

set( IMGUI_INCLUDE ${IMGUI_DIR} ${IMGUI_DIR}/backends ${IMGUI_FILE_DLG_EXT} )

set( IMGUI_LIB_FILES
     ${IMGUI_DIR}/imgui.cpp
     ${IMGUI_DIR}/imgui_demo.cpp
     ${IMGUI_DIR}/imgui_draw.cpp
     ${IMGUI_DIR}/imgui_tables.cpp
     ${IMGUI_DIR}/imgui_widgets.cpp
     ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
     ${IMGUI_DIR}/backends/imgui_impl_dx12.cpp
     ${IMGUI_FILE_DLG_EXT}/ImGuiFileDialog.cpp
   )

add_library( imgui STATIC ${IMGUI_LIB_FILES} )
target_include_directories( imgui PRIVATE ${IMGUI_INCLUDE} )

add_executable( StateScriptEditor ${SOURCE_FILES} )

target_include_directories( StateScriptEditor PRIVATE ${INCLUDE_ROOT_DIR} ${IMGUI_INCLUDE} )
target_link_libraries( StateScriptEditor imgui d3d12.lib dxgi.lib )

target_precompile_headers( StateScriptEditor PRIVATE ${INCLUDE_ROOT_DIR}/pch.hpp )