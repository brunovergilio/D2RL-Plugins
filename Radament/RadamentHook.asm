.code

EXTERN g_pSkillPoints : QWORD ; pointer to an array of skill point values
EXTERN g_STATLIST_AddUnitStatAddress : QWORD ; pointer to a global function address
EXTERN g_JumpBackAddress : QWORD ; pointer to a global address to go back to after this

RadamentASMHook PROC
	; 1. Preserve all volatile registers to protect the game state

	; TODO: In the future, trim down which registers needs to be saved / restored

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

	; 2. Rework the code
	movzx esi, byte ptr [rbx + 0104h] ; difficulty
	mov r10, [g_pSkillPoints]
	mov r11d, dword ptr [r10 + rsi * 4] ; skills array
	xor r9d, r9d
	mov rcx, rdi
	mov edx, 5
	mov r8, r11
	call qword ptr [g_STATLIST_AddUnitStatAddress]

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

	; 3. JUMP BACK TO THE EXE
	jmp qword ptr [g_JumpBackAddress]
RadamentASMHook ENDP

END