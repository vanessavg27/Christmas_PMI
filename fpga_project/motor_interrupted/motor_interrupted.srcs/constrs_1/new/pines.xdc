# Pines EMIO para control - Tri-state
# CK_IO0 - enable
set_property -dict { PACKAGE_PIN U14 IOSTANDARD LVCMOS33 } [get_ports { GPIO_m_tri_io[0] }]
# CK_IO1 - step  
set_property -dict { PACKAGE_PIN V13 IOSTANDARD LVCMOS33 } [get_ports { GPIO_m_tri_io[1] }]
# CK_IO2 - DIR
set_property -dict { PACKAGE_PIN T14 IOSTANDARD LVCMOS33 } [get_ports { GPIO_m_tri_io[2] }]
# CK_IO3 - (no usado, pero necesario definir)
set_property -dict { PACKAGE_PIN T15 IOSTANDARD LVCMOS33 } [get_ports { GPIO_m_tri_io[3] }]
# CK_IO4 - (no usado, pero necesario definir)
set_property -dict { PACKAGE_PIN V17 IOSTANDARD LVCMOS33 } [get_ports { GPIO_m_tri_io[4] }]
# CK_IO5 - (no usado, pero necesario definir)
set_property -dict { PACKAGE_PIN V18 IOSTANDARD LVCMOS33 } [get_ports { GPIO_m_tri_io[5] }]

# BOTON 
set_property -dict { PACKAGE_PIN D20 IOSTANDARD LVCMOS33 } [get_ports { BTN_0_tri_i }]