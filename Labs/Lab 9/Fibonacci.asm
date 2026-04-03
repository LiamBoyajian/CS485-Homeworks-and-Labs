.macro push %r
	subi $sp, $sp, 4
	sw %r, ($sp)
.end_macro

.macro pop %r
	lw %r, ($sp)
	addi $sp, $sp, 4
.end_macro

.data

prompt: .asciiz "Enter the number of sequence numbers to print: "
spacer: .asciiz ", "

.text
	# Print prompt
	li $v0, 4
	la $a0, prompt
	syscall
	
	# Get response
	li $v0, 5
	syscall
	
	# Move response out of v0 and into a0 as the procedure argument
	move $a0, $v0
	
		
	#int proper fib values
	la $a1 0
	la $a2 1
	
	jal FIB_PROC	
	
	# Exit cleanly
	li $v0, 10
	syscall
	

##
# Fibonacci procedure
# Arguments:
#  - a0: work towards
#  - a1: fib 1 
#  - a2: fib 2  
FIB_PROC:
	push $ra
	move $t0, $a0
	push $t0
	
	beq  $a0, $zero, END_FIB
	
	jal PRINT_PARAM #print the largest value
	
	#Calc here
	add $t0, $a1, $a2 #calc next largest val
	move $a1, $a2 #shift former largest to smallest
	move $a2, $t0 #replace former largest
	
	subi $a0, $a0, 1
	
	jal FIB_PROC

END_FIB:
	pop $t0
	add $v0, $v0, $t0
	
	pop $ra
	jr $ra
	
PRINT_PARAM:
	push $ra
	push $a0
	# Print the result string
	li $v0, 1
	move $a0, $a2
	syscall
	
	li $v0, 4
	la $a0 , spacer
	syscall
	
	pop $a0
	pop $ra
	
	jr $ra