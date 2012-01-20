void miiAVBListenerServer(clock clk_smi, out port ?p_mii_resetn, smi_interface_t &smi,
                            mii_interface_t &m, chanend appIn[3], chanend appOut[3], chanend ?server);