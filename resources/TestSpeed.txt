Use the core timer to double check your CPU clock settings

while (1) {
    _CP0_SET_COUNT(0); // set core timer to 0, remember it counts at half the CPU clock
    LATBINV = some_pin; // invert a pin

    // wait for half a second, setting LED brightness to pot angle while waiting
    while (_CP0_GET_COUNT() < 10000000) {
        val = readADC();
        OC1RS = val * normalize_to_pr;

        if (push_button_pin == 1) {
            // nothing
        } else {
            LATBINV = some_pin;
        }
    }
}
