/*
 * Entry point for -bios none -kernel boot on the virt machine: QEMU jumps
 * here directly in M-mode with nothing set up, so we just need a stack
 * before calling into C.
 */
.section .text.start
.global _start
_start:
    la sp, _stack_top
    call main
1:
    j 1b
