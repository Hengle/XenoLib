project(XenobladeToolset)
set_target_properties(spike_cli PROPERTIES OUTPUT_NAME xenoblade_toolset)

include(version)
toolset_version()

add_spike_subdir(bdat)
add_spike_subdir(mdo)
add_spike_subdir(smt)
add_spike_subdir(sar)
add_spike_subdir(tex)
add_spike_subdir(shader)
add_spike_subdir(arh)
add_spike_subdir(asm)
add_spike_subdir(map)

if(NOT RELEASEVER)
    add_spike_subdir(dev)
endif()
