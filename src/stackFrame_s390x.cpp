/*
 * Copyright The async-profiler authors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __s390x__

#include <errno.h>
#include <string.h>
#include <sys/syscall.h>
#include "stackFrame.h"
#include "vmStructs.h"


// s390x register access in ucontext
// PSW (Program Status Word) contains the instruction address
#define REG_PSW_ADDR  _ucontext->uc_mcontext.psw.addr
#define REG_GPR(n)    _ucontext->uc_mcontext.gregs[n]


uintptr_t& StackFrame::pc() {
    return (uintptr_t&)REG_PSW_ADDR;
}

uintptr_t& StackFrame::sp() {
    // r15 is the stack pointer on s390x
    return (uintptr_t&)REG_GPR(15);
}

uintptr_t& StackFrame::fp() {
    // r11 is typically used as frame pointer on s390x
    return (uintptr_t&)REG_GPR(11);
}

uintptr_t& StackFrame::retval() {
    // r2 is the return value register
    return (uintptr_t&)REG_GPR(2);
}

uintptr_t StackFrame::link() {
    // r14 is the link register (return address)
    return (uintptr_t)REG_GPR(14);
}

uintptr_t StackFrame::arg0() {
    // r2 is the first argument register
    return (uintptr_t)REG_GPR(2);
}

uintptr_t StackFrame::arg1() {
    // r3 is the second argument register
    return (uintptr_t)REG_GPR(3);
}

uintptr_t StackFrame::arg2() {
    // r4 is the third argument register
    return (uintptr_t)REG_GPR(4);
}

uintptr_t StackFrame::arg3() {
    // r5 is the fourth argument register
    return (uintptr_t)REG_GPR(5);
}

uintptr_t StackFrame::jarg0() {
    // For Java methods, typically arg1 (r3) contains 'this' or first arg
    return arg1();
}

uintptr_t StackFrame::method() {
    // r13 is often used for method pointer in JVM
    return (uintptr_t)REG_GPR(13);
}

uintptr_t StackFrame::senderSP() {
    // Sender SP is typically stored in the stack frame
    // On s390x, the backchain pointer is at offset 0 of the stack frame
    return stackAt(0);
}

void StackFrame::ret() {
    // Return by setting PC to link register and adjusting SP
    pc() = link();
}


bool StackFrame::unwindStub(instruction_t* entry, const char* name, uintptr_t& pc, uintptr_t& sp, uintptr_t& fp) {
    // For s390x, use the link register for return address
    pc = link();
    
    // Check if we need to adjust the stack pointer
    // s390x uses a backchain pointer at the top of each stack frame
    if (withinCurrentStack(sp)) {
        uintptr_t backchain = ((uintptr_t*)sp)[0];
        if (backchain != 0 && withinCurrentStack(backchain)) {
            sp = backchain;
            return true;
        }
    }
    
    return true;
}

bool StackFrame::unwindPrologue(NMethod* nm, uintptr_t& pc, uintptr_t& sp, uintptr_t& fp) {
    // s390x function prologue typically:
    // 1. Saves r14 (link register)
    // 2. Saves r11 (frame pointer) if used
    // 3. Adjusts stack pointer
    
    // For now, use simple unwinding via link register
    pc = link();
    
    // Try to follow the backchain
    if (withinCurrentStack(sp)) {
        uintptr_t backchain = ((uintptr_t*)sp)[0];
        if (backchain != 0 && withinCurrentStack(backchain)) {
            sp = backchain;
            return true;
        }
    }
    
    return false;
}

bool StackFrame::unwindEpilogue(NMethod* nm, uintptr_t& pc, uintptr_t& sp, uintptr_t& fp) {
    // In epilogue, the function is about to return
    // Use link register for return address
    pc = link();
    
    // Restore stack pointer from backchain
    if (withinCurrentStack(sp)) {
        uintptr_t backchain = ((uintptr_t*)sp)[0];
        if (backchain != 0 && withinCurrentStack(backchain)) {
            sp = backchain;
            return true;
        }
    }
    
    return false;
}

bool StackFrame::unwindAtomicStub(const void*& pc) {
    // Not needed for s390x
    return false;
}

void StackFrame::adjustSP(const void* entry, const void* pc, uintptr_t& sp) {
    // s390x stack frames have a standard layout with backchain at offset 0
    // No special adjustment needed in most cases
}

bool StackFrame::checkInterruptedSyscall() {
    // Check if the return value indicates an interrupted syscall
    return retval() == (uintptr_t)-EINTR;
}

bool StackFrame::isSyscall(instruction_t* pc) {
    // s390x syscall instruction is SVC (Supervisor Call)
    // SVC instruction format: 0x0A?? where ?? is the immediate value
    // We check for the SVC opcode (0x0A)
    return (*pc >> 8) == 0x0A;
}

#endif // __s390x__

// Made with Bob
