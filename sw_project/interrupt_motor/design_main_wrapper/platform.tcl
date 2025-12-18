# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct C:\Users\vanes\christmas\sw_project\interrupt_motor\design_main_wrapper\platform.tcl
# 
# OR launch xsct and run below command.
# source C:\Users\vanes\christmas\sw_project\interrupt_motor\design_main_wrapper\platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {design_main_wrapper}\
-hw {C:\Users\vanes\christmas\fpga_project\motor_interrupted\design_main_wrapper.xsa}\
-out {C:/Users/vanes/christmas/sw_project/interrupt_motor}

platform write
domain create -name {standalone_ps7_cortexa9_0} -display-name {standalone_ps7_cortexa9_0} -os {standalone} -proc {ps7_cortexa9_0} -runtime {cpp} -arch {32-bit} -support-app {hello_world}
platform generate -domains 
platform active {design_main_wrapper}
domain active {zynq_fsbl}
domain active {standalone_ps7_cortexa9_0}
platform generate -quick
platform generate
