/* **************************************
 * Module: top design of rv32i single-cycle processor
 *
 * Author: Hyeonseok Oh.
 *
 * **************************************
 */

#include "rv32i.h"

int main (int argc, char *argv[]) {

	// get input arguments
	FILE *f_imem, *f_dmem;
	if (argc < 3) {
		printf("usage: %s imem_data_file dmem_data_file\n", argv[0]);
		exit(1);
	}

	if ( (f_imem = fopen(argv[1], "r")) == NULL ) {
		printf("Cannot find %s\n", argv[1]);
		exit(1);
	}
	if ( (f_dmem = fopen(argv[2], "r")) == NULL ) {
		printf("Cannot find %s\n", argv[2]);
		exit(1);
	}

	// memory data (global)
	uint32_t *reg_data;
	uint32_t *imem_data;
	uint32_t instruction_count = 0;
	uint32_t *dmem_data;

	reg_data = (uint32_t*)malloc(32*sizeof(uint32_t));
	imem_data = (uint32_t*)malloc(IMEM_DEPTH*sizeof(uint32_t));
	dmem_data = (uint32_t*)malloc(DMEM_DEPTH*sizeof(uint32_t));

	// initialize memory data
	int i, j, k;
	for (i = 0; i < 32; i++) reg_data[i] = 0;
	for (i = 0; i < IMEM_DEPTH; i++) imem_data[i] = 0;
	for (i = 0; i < DMEM_DEPTH; i++) dmem_data[i] = 0;

	uint32_t d, buf;
	i = 0;
	printf("\n*** Reading %s ***\n", argv[1]);
	while (fscanf(f_imem, "%1d", &buf) != EOF) {
		d = buf << 31;
		for (k = 30; k >= 0; k--) {
			if (fscanf(f_imem, "%1d", &buf) != EOF) {
				if(buf == ' '){
					k++; continue;
				}
				d |= buf << k;
			} else {
				printf("Incorrect format!!\n");
				exit(1);
			}
		}
		imem_data[i] = d;
		printf("imem[%03d]: %08X\n", i, imem_data[i]);
		i++;
	}
	instruction_count = i;

	i = 0;
	printf("\n*** Reading %s ***\n", argv[2]);
	while (fscanf(f_dmem, "%8x", &buf) != EOF) {
		dmem_data[i] = buf;
		printf("dmem[%03d]: %08X\n", i, dmem_data[i]);
		i++;
	}

	fclose(f_imem);
	fclose(f_dmem);

	// processor model
	uint32_t pc_curr = 0, pc_next = 0;	// program counter
	struct imem_input_t imem_in;
	struct imem_output_t imem_out;

	struct imm_gen_input_t imm_gen_in;
	struct imm_gen_output_t imm_gen_out;

	struct ctl_input_t ctl_in;
	struct ctl_output_t ctl_out;

	struct regfile_input_t regfile_in;
	struct regfile_output_t regfile_out;

	struct branch_ctl_input_t br_in;
	struct branch_ctl_output_t br_out;
	
	struct alu_ctl_input_t alu_ctl_in;
	struct alu_ctl_output_t alu_ctl_out;

	struct alu_input_t alu_in;
	struct alu_output_t alu_out;

	struct dmem_input_t dmem_in;
	struct dmem_output_t dmem_out;


	uint32_t cc = 2;	// clock count
			
	while (cc < CLK_NUM && (instruction_count-- > 0)) {
		// instruction fetch
		pc_curr = pc_next;
		imem_in.addr = pc_curr;
		imem_out = imem(imem_in, imem_data);
		pc_next++;

		// instruction decode
		ctl_in.opcode = imem_out.opcode;
		ctl_out = ctl(ctl_in);
		
		regfile_in.rs1 = imem_out.r.rs1;
		regfile_in.rs2 = imem_out.r.rs2;
		regfile_in.rd =  imem_out.r.rd;
		regfile_in.reg_write = ctl_out.reg_write;
		regfile_out = regfile(regfile_in, reg_data);

		imm_gen_in.imem_out = imem_out;
		imm_gen_in.imm = ctl_out.imm;
		imm_gen_in.jump = ctl_out.jump;
		imm_gen_in.branch = ctl_out.branch;
		imm_gen_out = imm_gen(imm_gen_in);
		
		// execution
		alu_ctl_in.alu_op = ctl_out.alu_op;
		alu_ctl_in.func3 = imem_out.r.func3;
		alu_ctl_in.func7 = imem_out.r.func7;
		alu_ctl_out = alu_ctl(alu_ctl_in);
		
		alu_in.in1 = regfile_out.rs1_dout;
		alu_in.in2 = two_to_one_mux32(regfile_out.rs2_dout, imm_gen_out.dout, ctl_out.alu_src);
		alu_in.alu_ctl = alu_ctl_out.bout;
		alu_out = alu(alu_in);

		br_in.br_eq_sig = alu_out.br_eq_sig;
		br_in.br_lt_sig = alu_out.br_lt_sig;
		br_in.br_ltu_sig = alu_out.br_ltu_sig;
		br_in.cond_branch = extract_bits(imem_out.s.func3, 1, 2);
		br_in.reverse = extract_bits(imem_out.s.func3, 0, 1);
		br_in.branch = ctl_out.branch;
		br_in.jump = ctl_out.jump;
		br_out = branch_ctl(br_in);

		// memory
		dmem_in.addr = alu_out.result;
		dmem_in.din = regfile_out.rs2_dout;
		dmem_in.acc_unit = extract_bits(imem_out.i.func3, 0, 2);
		dmem_in.mem_read = ctl_out.mem_read;
		dmem_in.mem_write = ctl_out.mem_write;
		dmem_out = dmem(dmem_in, dmem_data);
		
		// write-back
		regfile_in.rd_din = two_to_one_mux32(
			alu_out.result,
				sign_extender(dmem_out.dout, 
				left_shift_gate(0b1000, (extract_bits(imem_out.i.func3, 0, 2))),
				not_gate(extract_bits(imem_out.i.func3, 2, 1))),
			ctl_out.mem_to_reg
		);

		// PC 변수들은 1이 증가할 때마다 4가 증가하는 것으로 판단되기 때문에,
		// PC 변수들을 2배 곱한 값을 저장합니다.
		regfile_in.rd_din = four_to_one_mux32(
			regfile_in.rd_din,
			imm_gen_out.dout,
			add_gate32((pc_curr << 2), imm_gen_out.dout),
			(pc_next << 2),
			ctl_out.reg_write_src
		);

		regfile(regfile_in, reg_data);

		// PC 변수들은 1이 증가할 때마다 4가 증가하는 것으로 판단되기 때문에, 
		// alu_out.result 및 imm_gen_out.dout을 4배 줄인 값을 대입합니다.
		printf("Prev program counter : %08X\n", pc_curr);
		pc_next = two_to_one_mux32(pc_next, ((int32_t)alu_out.result >> 2), ctl_out.jump);
		pc_next = two_to_one_mux32(pc_next, add_gate32(pc_curr, ((int32_t)imm_gen_out.dout >> 2)), br_out.bout);
		printf("Next program counter : %08X\n", pc_next);

		// ----
		print_written(ctl_out.mem_write, ctl_out.reg_write);
		printf("cc %d has been processed.\n\n", cc);
		cc++;

	}

	free(reg_data);
	free(imem_data);
	free(dmem_data);

	return 1;
}
