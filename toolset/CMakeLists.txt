add_subdirectory(${TPD_PATH}/precore/spike ${CMAKE_CURRENT_BINARY_DIR}/spike)
add_subdirectory(${TPD_PATH}/precore/gltf ${CMAKE_CURRENT_BINARY_DIR}/gltf)
set_target_properties(spike PROPERTIES OUTPUT_NAME xenoblade_toolset)

add_spike_subdir(bdat)
add_spike_subdir(mdo)
add_spike_subdir(smt)
add_spike_subdir(sar)
add_spike_subdir(tex)
add_spike_subdir(shader)
add_spike_subdir(arh)
add_spike_subdir(asm)

add_spike_subdir(dev)
