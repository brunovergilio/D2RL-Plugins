.code

EXTERN HookHandler : PROC
EXTERN g_JumpBackAddress : QWORD
EXTERN g_TargetCallAddress : QWORD

hkTreasureHook PROC
    ; 1. Preserve all volatile registers to protect the game state

	; volatile
	push rax
	push rcx
	push rdx
	push r8
	push r9
	push r10
	push r11

	; non-volatile - must save
	push rbx
	push rbp
	push rdi
	push rsi
	push r12
	push r13
	push r14
	push r15
    
    ; 15 registers pushed = 120 bytes (misaligned by 8 bytes - add 8 to rsp)
    ; Allocate 32 + misaligned bytes of shadow space for the C++ function call
    sub rsp, 40

    ; 2. MAP THE RECOVERED REGISTER POINTERS TO THE C++ ARGUMENTS
    ; RCX = Parameter 1 (pGame)   -> Found in RSI
    ; RDX = Parameter 2 (pMonster)-> Found in RBX
    ; R8  = Parameter 3 (pPlayer) -> Found in R14
    mov rcx, rsi
    mov rdx, rbx
    mov r8, r14

    call HookHandler     ; Jump to your streamlined C++ code
    
    ; 3. Clean up shadow space and restore original engine registers
	add rsp, 40
	pop r15
	pop r14
	pop r13
	pop r12
	pop rsi
	pop rdi
	pop rbp
	pop rbx
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdx
	pop rcx
	pop rax

    ; 4. EXECUTE THE ORIGINAL OBLITERATED INSTRUCTIONS
    movzx ecx, byte ptr [rsi + 0106h]
    mov r8d, edi
    movzx edx, bp
    
    ; 5. MANUALLY EXECUTE THE ORIGINAL CALL
    call qword ptr [g_TargetCallAddress]

    ; 6. JUMP BACK TO THE EXE
    jmp qword ptr [g_JumpBackAddress]
hkTreasureHook ENDP

END
