.data
    dna_seq_1: .asciiz "ACCTGGCTTCAACGACCCGTACACGTGGCACTTCAGGAGGCGCCCGCAGGGGGGAA"
    dna_seq_2: .asciiz "CTTTCACTTGAACGTCCCGTACACGTGGCACTTCAGGAGGCGCCAGTAGGGGAGAA"
    result_1: .asciiz "The shortest Hamming distance is "
    result_2: .asciiz " at a window offset of "
    newline: .asciiz "\n"
    
.text
    
    # address for sequence 1 as first argument
    la $a0, dna_seq_1
    # address for sequence 2 as second argument
    la $a1, dna_seq_2
    # target window length
    li $a2, 10
    
    # Call hamming proc
    jal CALC_HAMMING_WINDOW
    
    # Store result
    move $t0, $v0
    move $t1, $v1
    
    # Print result
    li $v0, 4
    la $a0, result_1
    syscall
    
    li $v0, 1
    move $a0, $t0
    syscall

    li $v0, 4
    la $a0, result_2
    syscall
    
    li $v0, 1
    move $a0, $t1
    syscall
    
    li $v0, 4
    la $a0, newline
    syscall
    
    # Cleanly exit
    li $v0, 10
    syscall
    
    
## Shortest Hamming  Window Procedure
# Args:
#    - a0: address pointing to first sequence
#    - a1: address pointint to second sequence
#    - a2: window length
# 
# Returns:
#    - v0: the Hamming distance of the window with the
#           shortest Hamming distance
#    - v1: the offset of the window with the shortest 
#          Hamming distance
##
CALC_HAMMING_WINDOW:

## YOUR CODE GOES HERE
## Do not edit anything above this comment
	subi $sp, $sp, 16
	sw $ra,  0($sp)
	sw $s0,  4($sp)
	sw $s1,  8($sp)
	sw $s2, 12($sp)
	   
	la $s0, -1 #short hamming offset
	la $s1, 99999  # current shortest hamming value from any window
	la $s2, -1 #hamming offset (index)
	      
    HAMMING_FIND_LOOP:	   
    	addi $s2, $s2, 1
    	move $a3, $s2 # window offset
    	
    	jal CALC_HAMMING #v0 -1 if reached end
    	
	blt $v0, $s1 SHORTEST_HAMMING_FOUND
	j HAMMING_FIND_LOOP
	
	
    SHORTEST_HAMMING_FOUND:
    	beq $v0, -1, find_hamming_loop_finished
	move $s1, $v0     #shortest hamming
	move $s0, $s2
	beq $s1, 0 find_hamming_loop_finished
	j HAMMING_FIND_LOOP
    
    find_hamming_loop_finished:
   #    - v0: the Hamming distance of the window with the
   #           shortest Hamming distance
   #    - v1: the offset of the window with the shortest 
   #          Hamming distance
	move $v0, $s1
    	move $v1, $s2
    
    
    my_return_operations:
		lw $s2, 12($sp)
		lw $s1,  8($sp)
		lw $s0,  4($sp)
		lw $ra,  0($sp)
		addi $sp, $sp, 16
		
		jr $ra
    
## Hamming distance procedure
# Args:
#    - a0: address pointing to first sequence
#    - a1: address pointint to second sequence
#    - a2: window length
#    - a3: window offset
#           (window offset + window length will always 
#            be less than the sequence length)
# Returns:
#    - v0: hamming distance of first and second sequence
#          for the provided window
##
CALC_HAMMING:

## YOUR CODE GOES HERE
## Do not edit anything above this comment
	subi $sp, $sp, 16
	sw $ra,  0($sp)
	sw $s0,  4($sp)
	sw $s1,  8($sp)
	sw $s2, 12($sp)

	move $s0, $a0
	move $s1, $a1
	move $s2, $zero
	
	add $s0, $s0, $a3
	add $s1, $s1, $a3
	
	la $t6, 0
	
	calc_hamming_loop:
		lb $t0, 0($s0)
		lb $t1, 0($s1)
			
		
		addi $t6, $t6, 1
		
		beq $t0, $zero, calc_end_before_window
		beq $t1, $zero, calc_end_before_window
		beq $t0, $t1, calc_hamming_equal
		addi $s2, $s2, 1
		
	calc_hamming_equal:
		addi $s0, $s0, 1
		addi $s1, $s1, 1
		
		beq $a2, $t6, calc_hamming_loop_finished #if we reached the end of the window
		j calc_hamming_loop
	
	
	calc_end_before_window:
		la $v0, -1
		j return_operations
		
	calc_hamming_loop_finished:
		move $v0, $s2
		
			
	return_operations:
		lw $s2, 12($sp)
		lw $s1,  8($sp)
		lw $s0,  4($sp)
		lw $ra,  0($sp)
		addi $sp, $sp, 16
		
		jr $ra

	
