.macro push %r
	subi $sp, $sp, 4
	sw %r, ($sp)
.end_macro

.macro pop %r
	lw %r, ($sp)
	addi $sp, $sp, 4
.end_macro
	
.data
prompt: .asciiz "Enter the number of rows: "
space: .asciiz " "
star: .asciiz "*"
newline: .asciiz "\n"

.text
	# Print the prompt
	li $v0, 4
	la $a0, prompt
	syscall
	
	# Get an integer input from the user
	li $v0, 5
	syscall
	
	# Set up the argument for the procedure call
	#  (v0 contains the result from the previous syscall)
	move $a0, $v0
	
	# Call the procedure
	jal PRINT_TRI_PROC
	
	# Exit cleanly
	li $v0, 10
	syscall



## PRINT_TRI_PROC
# Prints a triangle of  stars (*)
# Arguments:
#  - a0: Number of rows in the triangle
# Returns:
#  - Nothing
##
PRINT_TRI_PROC:

	# Prologue
	push $ra
	push $s0
	push $s1
	push $s2
	push $s3
	
	move $s0, $a0         # s0 contains the number of rows
	move $s1, $zero       # s1 contains the current iteration
	li $s2, 1             # s2 contains the current number of stars to print for the current iteration
	                      #   which starts at 1
	subi $s3, $s0, 1      # s3 contains the current number of spaces to print for the current iteration
	                      #   which starts at the number of rows - 1
	
print_tri_proc_loop:
	beq $s0, $zero, print_tri_proc_loop_finished #when row number remaining = 0
	subi $s0, $s0, 1 #row number -1
	
	# Set up the arguments for the procedure call
	move $a0, $s3
	move $a1, $s2
	
	# Call the procedure
	jal PRINT_TRI_ROW_PROC
	
	# Add 2 stars for the next row
	addi $s2, $s2, 2
	# Subtract 1 space for the next row
	subi $s3, $s3, 1
	
	j print_tri_proc_loop
	
	
print_tri_proc_loop_finished:
	# Epilogue
	pop $s3
	pop $s2
	pop $s1
	pop $s0
	pop $ra
	
	jr $ra



## PRINT_TRI_ROW_PROC
# Prints a single row of stars
# that are centered
# Arguments:
#  - a0: number of spaces
#  - a1: number of stars
# Returns:
#  - Nothing
##
PRINT_TRI_ROW_PROC:

	push $ra
	
	#Moving args to temp_vars
	move $t0, $a0 
	move $t1, $a1

	
	#Printing spaces
	#t0 is number of spaces
	PRINT_SPACES:
		beq $t0, $zero, print_spaces_finished
		subi $t0, $t0, 1
	
	
		li $v0, 4
		la $a0, space
		syscall


		j PRINT_SPACES
	
	print_spaces_finished:

	#printing stars
	#t1 is number of stars
	PRINT_STARS:
		beq $t1, $zero, print_stars_finished
		subi $t1, $t1, 1
		
		li $v0, 4
		la $a0, star
		syscall
		
		j PRINT_STARS
	
	print_stars_finished:
	
	#print a newline char
	li $v0, 4
	la $a0, newline
	syscall
	
	
	pop $ra
	jr $ra	#end of the PRINT_TRI_ROW_PROC
	
	

