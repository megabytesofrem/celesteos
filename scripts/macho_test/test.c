// Bare-metal Mach-O test payload
void _start(void) {
    // Volatile write to dummy address to test execution flow later
    volatile unsigned int* dummy = (unsigned int*)0x1000;
    *dummy = 0x1337C0DE;

    while (1) {
        // Spin loop
    }
}