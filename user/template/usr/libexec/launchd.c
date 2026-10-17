void _start(void) {
    while (1) {
        __asm__ volatile("pause");
    }
}