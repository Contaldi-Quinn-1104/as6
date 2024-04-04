cmake_minimum_required(VERSION 3.20)
project(DuckLib LANGUAGES CXX)

include(includeable.cmake)
add_subdirectory(raylib)

file(GLOB sources "src/*.cpp" "src/*.c")
add_executable(as6 ${sources})
target_link_libraries(as6 PUBLIC raylib raylib_cpp raylib::buffered)
set_property(TARGET as6 PROPERTY CXX_STANDARD 20)

configure_file(textures/skybox.png textures/skybox.png COPYONLY)
file(MAKE_DIRECTORY textures)
configure_file(textures/water.jpg textures/water.jpg COPYONLY)
configure_file(meshes/PolyPlane.glb meshes/PolyPlane.glb COPYONLY)
#5 ships
configure_file(meshes/OilTanker.glb meshes/OilTanker.glb COPYONLY)
configure_file(meshes/ddg51.glb meshes/ddg51.glb COPYONLY)
configure_file(meshes/CargoG_HOSBrigadoon.glb meshes/CargoG_HOSBrigadoon.glb COPYONLY)
configure_file(meshes/Container_ShipLarge.glb meshes/Container_ShipLarge.glb COPYONLY)

make_includeable(raylib/raylib/examples/models/resources/shaders/glsl330/skybox.vs generated/skybox.330.vs)
make_includeable(raylib/raylib/examples/models/resources/shaders/glsl330/skybox.fs generated/skybox.330.fs)
make_includeable(raylib/raylib/examples/models/resources/shaders/glsl330/cubemap.vs generated/cubemap.330.vs)
make_includeable(raylib/raylib/examples/models/resources/shaders/glsl330/cubemap.fs generated/cubemap.330.fs)