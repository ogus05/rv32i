/* **************************************
 * Module: top design of rv32i single-cycle processor
 *
 * Author: Hyeonseok Oh.
 *
 * **************************************
 */


// struct의 bit단위 variable들의 padding을 없애기 위해 선언했습니다.
#pragma pack(1)

// headers
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// defines
#define REG_WIDTH 32
#define IMEM_DEPTH 1024
#define DMEM_DEPTH 1024

//definitions for ALU.
#define ALU_CTL_AND 	0b0000
#define ALU_CTL_OR 		0b0001
#define ALU_CTL_ADD 	0b0010
#define ALU_CTL_XOR 	0b0011
#define ALU_CTL_SLL 	0b0100
#define ALU_CTL_SLT 	0b0101
#define ALU_CTL_SUB 	0b0110
#define ALU_CTL_SLTU 	0b0111
#define ALU_CTL_SRL 	0b1000
#define ALU_CTL_SRLA 	0b1001

//definitions for control unit.
#define CTL_I_FORMAT_LOAD 		0b0000011
#define CTL_S_FORMAT 			0b0100011
#define CTL_R_FORMAT 			0b0110011
#define CTL_I_FORMAT 			0b0010011
#define CTL_LUI					0b0110111
#define CTL_AUIPC				0b0010111
#define CTL_B_FORMAT			0b1100011
#define CTL_JAL					0b1101111
#define CTL_JALR				0b1100111

//definitions for immediate generator.
#define IMM_FORMAT_I			0b00
#define IMM_FORMAT_S			0b01
#define IMM_FORMAT_U			0b10

// configs
#define CLK_NUM 45

// structures
struct imem_input_t {
	uint32_t addr;
};

typedef struct {
	uint8_t opcode: 7;
	uint8_t rd: 5;
	uint8_t func3: 3;
	uint8_t rs1: 5;
	uint8_t rs2: 5;
	uint8_t func7: 7;
} r_format;

typedef struct {
	uint8_t opcode: 7;
	uint8_t rd: 5;
	uint8_t func3: 3;
	uint8_t rs1: 5;
	uint16_t imm1: 12;
} i_format;

typedef struct {
	uint8_t opcode: 7;
	uint8_t imm1: 5;
	uint8_t func3: 3;
	uint8_t rs1: 5;
	uint8_t rs2: 5;
	uint8_t imm2: 7;
} s_format;

typedef struct {
	uint8_t opcode: 7;
	uint8_t rd: 5;
	uint32_t imm1: 20;
} u_format;

typedef struct {
	uint8_t opcode: 7;
	uint8_t imm1: 1;
	uint8_t imm2: 4;
	uint8_t func3: 3;
	uint8_t rs1: 5;
	uint8_t rs2: 5;
	uint8_t imm3: 6;
	uint8_t imm4: 1;
} b_format;

typedef struct {
	uint8_t opcode: 7;
	uint8_t rd: 5;
	uint8_t imm1: 8;
	uint8_t imm2: 1;
	uint16_t imm3: 10;
	uint8_t imm4: 1;
} j_format;

//각 instruction들의 위치를 쉽게 찾기 위해 union type을 사용했습니다.
struct imem_output_t {
	union{
		struct {
			uint8_t opcode: 7;
			uint32_t inst: 25;
		};
		r_format r;
		i_format i;
		s_format s;
		u_format u;
		b_format b;
		j_format j;
	};
};

struct regfile_input_t {
	uint8_t rs1 : 5;
	uint8_t rs2 : 5;
	uint8_t rd : 5;
	uint32_t rd_din;
	uint8_t reg_write: 1;
};

struct regfile_output_t {
	uint32_t rs1_dout;
	uint32_t rs2_dout;
};

struct ctl_input_t {
	uint8_t opcode: 7;
};

struct ctl_output_t {
	uint8_t branch: 1;
	uint8_t mem_read : 1;
	uint8_t mem_to_reg : 1;
	uint8_t alu_op : 2;
	uint8_t mem_write : 1;
	uint8_t alu_src: 1;
	uint8_t reg_write: 1;
	uint8_t jump: 1;
	
	/*
	Immediate의 type을 결정합니다.
	Immediate generator에서 사용됩니다.
	00. I-type.
	01. S(B)-type.
	10. U(J)-type.
	11. Not used.
	*/
	uint8_t imm: 2;
	
	/*
	Register에 write할 값을 결정합니다.
	WB시 4-to-1 mux에서 사용됩니다.
	00. Result of alu or read memory.
	01. Imm(LUI).
	10. PC + imm(AUIPC)
	11. PC + 4(J)
	*/
	uint8_t reg_write_src : 2;
};

struct imm_gen_input_t {
	struct imem_output_t imem_out;
	uint8_t imm: 2;
	uint8_t branch: 1;
	uint8_t jump: 1;
};

struct imm_gen_output_t {
	uint32_t dout;
};

struct alu_ctl_input_t {
	uint8_t alu_op: 2;
	uint8_t func7: 7;
	uint8_t func3: 3;
};

struct alu_ctl_output_t {
	uint8_t bout: 4;
};

struct alu_input_t {
	uint32_t in1;
	uint32_t in2;
	uint8_t alu_ctl : 4;
};

struct alu_output_t {
	uint32_t result;

	/*
	ALU가 result와
	branch를 위한 signal들을 출력합니다.
	*/
	uint8_t br_eq_sig: 1;
	uint8_t br_lt_sig: 1;
	uint8_t br_ltu_sig: 1;
};

struct branch_ctl_input_t {
	uint8_t br_eq_sig: 1;
	uint8_t br_lt_sig: 1;
	uint8_t br_ltu_sig: 1;

	uint8_t branch: 1;
	uint8_t jump: 1;

	/*
	BNE는 BEQ에,
	BGE는 BLT에,
	BGEU는 BLTU에
	not 연산을 사용하기 위한 signal입니다.
	func3의 [0] bit를 사용합니다.
	*/
	uint8_t reverse: 1;

	/*
	Branch가 사용할 signal을 결정합니다.
	func3의 [2:1] bit를 사용합니다.
	00. br_eq_sig.
	01. Not used.
	10. br_lt_sig.
	11. br_ltu_sig.
	*/
	uint8_t cond_branch: 2;

};

struct branch_ctl_output_t {
	uint8_t bout: 1;
};

struct dmem_input_t {
	uint32_t addr;
	uint32_t din;
	uint8_t mem_read : 1;
	uint8_t mem_write : 1;

	/*
	Byte-addressible memory의 access unit을 결정합니다.
	func3의 [1:0] bit를 사용합니다.
	00. Byte.
	01. Half word.
	10. Word.
	11. Not used.
	*/
	uint8_t acc_unit: 2;
};

struct dmem_output_t {
	uint32_t dout;
};

/*
하나의 clock에서 바뀐 값들의 logging을 위해 사용되는 값들입니다.
바뀌기 전, 바뀐 후, 주소, memory or register 여부가 저장됩니다.
하나의 clock cycle이 끝난 후 출력되며, 초기화됩니다.
*/
struct log_value {
	uint32_t prev_value;
	uint32_t current_value;
	uint8_t addr: 5;
	uint8_t is_mem: 1;
};

static struct log_value __log_value_list[32];
static uint8_t __written_count = 0;
void add_log_value(uint32_t prev_value, uint32_t current_value, uint8_t addr, uint8_t is_mem);
void print_written(uint8_t mem_write, uint8_t reg_write);

/*---------------*/

//functions.
struct imem_output_t imem(struct imem_input_t imem_in, uint32_t* imem_data);

struct ctl_output_t ctl(struct ctl_input_t ctl_input);

struct regfile_output_t regfile(struct regfile_input_t regfile_in, uint32_t* reg_data);

struct alu_ctl_output_t alu_ctl(struct alu_ctl_input_t alu_ctl_in);

// Instruction format에 맞는 immediate를 추출하는 함수입니다.
struct imm_gen_output_t imm_gen(struct imm_gen_input_t gen_in);

struct alu_output_t alu(struct alu_input_t alu_in);

// Branch 여부를 판단하는 함수입니다.
struct branch_ctl_output_t branch_ctl(struct branch_ctl_input_t br_in);

struct dmem_output_t dmem(struct dmem_input_t dmem_in, uint32_t* dmem_data);

// target[(start + length - 1):start] bit를 추출합니다.
uint32_t extract_bits(uint32_t target, uint8_t start, uint8_t length);

uint32_t two_to_one_mux32(uint32_t input_0, uint32_t input_1, uint8_t signal);

uint32_t four_to_one_mux32(uint32_t input_0, uint32_t input_1, uint32_t input_2, uint32_t input_3, uint8_t signal);

uint32_t add_gate32(uint32_t input_0, uint32_t input_1);

uint32_t or_gate32(uint32_t input_0, uint32_t input_1);

uint8_t and_gate(uint8_t input_0, uint8_t input_1);

uint8_t or_gate(uint8_t input_0, uint8_t input_1);

uint8_t xor_gate(uint8_t input_0, uint8_t input_1);

uint8_t not_gate(uint8_t input_0);

uint32_t left_shift_gate(uint32_t input_0, uint32_t amount);

uint32_t sign_extender(uint32_t target, uint8_t size, uint8_t sign);

