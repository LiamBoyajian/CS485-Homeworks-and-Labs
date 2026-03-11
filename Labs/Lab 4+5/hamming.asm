.data
    dna_seq_1: .asciiz "CTTCAACGACCCGTACACGTGGCACTTCAGGAGGCGCCCGCAGGGGGGAA\n"
    dna_seq_2: .asciiz "ATTGAACGTCCCGTACACGTCCCACTTCAGGAGGCGCCAGTAGGGGAGAA\n"
    result: .asciiz "The sequences have a Hamming distance of "
    newline: .asciiz "\n"
    
.text
    
    # address for sequence 1 as first argument
    la $a0, dna_seq_1
    # address for sequence 2 as second argument
    la $a1, dna_seq_2
    
    # Call hamming proc
    jal CALC_HAMMING
    
    # Store result
    move $t0, $v0
    
    # Print result
    li $v0, 4
    la $a0, result
    syscall
    
    li $v0, 1
    move $a0, $t0
    syscall
    
    li $v0, 4
    la $a0, newline
    syscall
    
    # Cleanly exit
    li $v0, 10
    syscall
    
    ## Result should look like:
    ## The sequences have a Hamming distance of 7
    
    
## Hamming distance procedure
# Args:
#    - a0: address pointing to first sequence
#    - a1: address pointint to second sequence
# Returns:
#    - v0: hamming distance of first and second sequence
##
CALC_HAMMING:

## YOUR CODE GOES HERE
## Do not edit anything above this comment
	subi $sp, $sp, 4
	sw $ra, ($sp)
	
	move $t0, $a0
	move $t1, $a1
	la $t2, newline
	li $t3, 0
	#Loop through given strings
	HAM_LOOP:

	
	lb $t4, 0($t0)
	lb $t5, 0($t1)

		
	beq $t4, 10, HAM_END 
	beq $t5, 10, HAM_END
	#Will get here if niether are equal to '/n'
		
	beq $t4, $t5, SKIP_OVER_INCREMENT
	#increment counter only if the values are unequal
	addi $t3, $t3, 1
	
	SKIP_OVER_INCREMENT:	
	
	#Increment both
	addi $t0, $t0, 1
	addi $t1, $t1, 1
	j HAM_LOOP
	
	
	HAM_END:
	lw $ra, ($sp)
	addi $sp, $sp, 4
	
	move $v0, $t3
	jr $ra #end of CALC_HAMMING


