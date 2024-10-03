#include "rv32i.h"

void add_log_value(uint32_t prev_value, uint32_t current_value, uint8_t addr, uint8_t is_mem)
{
    struct log_value *cur_log = &__log_value_list[__written_count];
    uint8_t i = 0;
    for(i = 0; i < __written_count; i++){
        if(__log_value_list[i].addr == addr && __log_value_list[i].is_mem == is_mem){
            break;
        }
    }

    if(i == __written_count){
        cur_log->prev_value = prev_value;
        cur_log = &__log_value_list[__written_count++];
    } else{
        cur_log->prev_value = __log_value_list[i].prev_value;
        cur_log = &__log_value_list[i];
    }

    cur_log->current_value = current_value;
    cur_log->addr = addr;
    cur_log->is_mem = is_mem;

}

void print_written(uint8_t mem_write, uint8_t reg_write)
{
    struct log_value* cur_log;
    for(uint8_t i = 0; i < __written_count; i++){
        cur_log = &__log_value_list[i];
        if(cur_log->is_mem && (mem_write > 0)){
            printf("\033[0;31mWritten memory - %08X => %08X, addr : %d\033[0m\n", cur_log->prev_value, cur_log->current_value, cur_log->addr);
        }
        if((!cur_log->is_mem) && (reg_write > 0)){
            printf("\033[0;31mWritten register - %08X => %08X, addr : %d\033[0m\n", cur_log->prev_value, cur_log->current_value, cur_log->addr);
        }
    }
    __written_count = 0;
}

struct imem_output_t imem(struct imem_input_t imem_in, uint32_t *imem_data)
{
    struct imem_output_t output;
    output.opcode = (imem_data[imem_in.addr] & ((1UL << 7) - 1));
    output.inst = (imem_data[imem_in.addr] >> 7);
    printf("Fetched instruction: %08X\n", *(uint32_t*)(&output));
    return output;
}

struct ctl_output_t ctl(struct ctl_input_t ctl_input)
{
    struct ctl_output_t output;

    switch(ctl_input.opcode){
        case (CTL_I_FORMAT_LOAD): {
            output.branch = 0b0;
            output.mem_read = 0b1;
            output.mem_to_reg = 0b1;
            output.alu_op = 0b00;
            output.mem_write = 0b0;
            output.alu_src = 0b1;
            output.reg_write = 0b1;
            output.imm = IMM_FORMAT_I;
            output.jump = 0b0;
            output.reg_write_src = 0b00;
            printf("Control unit detected Load instruction\n");
        } break;
        case (CTL_S_FORMAT): {
            output.branch = 0b0;
            output.mem_read = 0b0;
            output.mem_to_reg = 0b0;
            output.alu_op = 0b00;
            output.mem_write = 0b1;
            output.alu_src = 0b1;
            output.reg_write = 0b0;
            output.imm = IMM_FORMAT_S;
            output.jump = 0b0;
            output.reg_write_src = 0b00;
             printf("Control unit detected Store instruction\n");
       } break;
        case (CTL_R_FORMAT): {
            output.branch = 0b0;
            output.mem_read = 0b0;
            output.mem_to_reg = 0b0;
            output.alu_op = 0b10;
            output.mem_write = 0b0;
            output.alu_src = 0b0;
            output.reg_write = 0b1;
            output.imm = 0;
            output.jump = 0b0;
            output.reg_write_src = 0b00;
            printf("Control unit detected R-Format instruction\n");
        } break;
        case (CTL_I_FORMAT): {
            output.branch = 0b0;
            output.mem_read = 0b0;
            output.mem_to_reg = 0b0;
            output.alu_op = 0b10;
            output.mem_write = 0b0;
            output.alu_src = 0b1;
            output.reg_write = 0b1;
            output.imm = IMM_FORMAT_I;
            output.jump = 0b0;
            output.reg_write_src = 0b00;
            printf("Control unit detected I-Format instruction\n");
        } break;
        case (CTL_LUI): {
            output.branch = 0b0;
            output.mem_read = 0b0;
            output.mem_to_reg = 0b0;
            output.alu_op = 0b10;
            output.mem_write = 0b0;
            output.alu_src = 0b1;
            output.reg_write = 0b1;
            output.imm = IMM_FORMAT_U;
            output.jump = 0b0;
            output.reg_write_src = 0b01;
            printf("Control unit detected LUI instruction\n");
        } break;
        case (CTL_AUIPC): {
            output.branch = 0b0;
            output.mem_read = 0b0;
            output.mem_to_reg = 0b0;
            output.alu_op = 0b10;
            output.mem_write = 0b0;
            output.alu_src = 0b1;
            output.reg_write = 0b1;
            output.imm = IMM_FORMAT_U;
            output.jump = 0b0;
            output.reg_write_src = 0b10;
            printf("Control unit detected AUIPC instruction\n");
       } break;
        case (CTL_B_FORMAT): {
            output.branch = 0b1;
            output.mem_read = 0b0;
            output.mem_to_reg = 0b0;
            output.alu_op = 0b01;
            output.mem_write = 0b0;
            output.alu_src = 0b0;
            output.reg_write = 0b0;
            output.imm = IMM_FORMAT_S;
            output.jump = 0b0;
            output.reg_write_src = 0b00;
            printf("Control unit detected B-Format instruction\n");
       } break;
        case (CTL_JAL): {
            output.branch = 0b1;
            output.mem_read = 0b0;
            output.mem_to_reg = 0b0;
            output.alu_op = 0b00;
            output.mem_write = 0b0;
            output.alu_src = 0b1;
            output.reg_write = 0b1;
            output.imm = IMM_FORMAT_U;
            output.jump = 0b1;
            output.reg_write_src = 0b11;
            printf("Control unit detected JAL instruction\n");
        } break;
        case (CTL_JALR): {
            output.branch = 0b0;
            output.mem_read = 0b0;
            output.mem_to_reg = 0b0;
            output.alu_op = 0b10;
            output.mem_write = 0b0;
            output.alu_src = 0b1;
            output.reg_write = 0b1;
            output.imm = IMM_FORMAT_I;
            output.jump = 0b1;
            output.reg_write_src = 0b11;
            printf("Control unit detected JALR instruction\n");
        } break;
        default: {
            printf("Error in ctl : ctl received %d as an opcode", ctl_input.opcode);
            exit(1);
        } break;
    }

    return output;
}

struct regfile_output_t regfile(struct regfile_input_t regfile_in, uint32_t *reg_data)
{
    struct regfile_output_t regfile_out;

    regfile_out.rs1_dout = (regfile_in.rs1 == 0) ? 0 : reg_data[regfile_in.rs1];
    regfile_out.rs2_dout = (regfile_in.rs2 == 0) ? 0 : reg_data[regfile_in.rs2];

    printf("Register(read) - %08X as rs1, addr: %d\n", regfile_out.rs1_dout, regfile_in.rs1);
    printf("Register(read) - %08X as rs2, addr: %d\n", regfile_out.rs2_dout, regfile_in.rs2);

    if(regfile_in.reg_write){
        uint32_t temp = reg_data[regfile_in.rd];        //temp variable for logging.

        printf("Register(write) - from %08X ", reg_data[regfile_in.rd]);
        reg_data[regfile_in.rd] = regfile_in.rd_din;
        printf("to %08X, addr: %d\n", reg_data[regfile_in.rd], regfile_in.rd);
    
        add_log_value(temp, reg_data[regfile_in.rd], regfile_in.rd, 0);
    }

    return regfile_out;
}

struct alu_ctl_output_t alu_ctl(struct alu_ctl_input_t alu_ctl_in)
{
    struct alu_ctl_output_t alu_ctl_out;
    alu_ctl_out.bout = 0;

    if(alu_ctl_in.alu_op == 0b00){
        alu_ctl_out.bout = ALU_CTL_ADD;
    } else if(extract_bits(alu_ctl_in.alu_op, 0, 1) == 1){
        alu_ctl_out.bout = ALU_CTL_SUB;
    } else if(extract_bits(alu_ctl_in.alu_op, 1, 1) == 1){
        if(alu_ctl_in.func3 == 0b000){
            if(alu_ctl_in.func7 == 0b0100000){
                alu_ctl_out.bout = ALU_CTL_SUB;
            } else{
                alu_ctl_out.bout = ALU_CTL_ADD;
            }
        } else if(alu_ctl_in.func3 == 0b001){
            alu_ctl_out.bout = ALU_CTL_SLL;
        } else if(alu_ctl_in.func3 == 0b010){
            alu_ctl_out.bout = ALU_CTL_SLT;
        } else if(alu_ctl_in.func3 == 0b011){
            alu_ctl_out.bout = ALU_CTL_SLTU;
        } else if(alu_ctl_in.func3 == 0b100){
            alu_ctl_out.bout = ALU_CTL_XOR;
        } else if(alu_ctl_in.func3 == 0b101){
            if(alu_ctl_in.func7 == 0b0000000){
                alu_ctl_out.bout = ALU_CTL_SRL;
            } else if(alu_ctl_in.func7 == 0b0100000){
                alu_ctl_out.bout = ALU_CTL_SRLA;
            }
        } else if(alu_ctl_in.func3 == 0b110){
            alu_ctl_out.bout = ALU_CTL_OR;
        } else if(alu_ctl_in.func3 == 0b111){
            alu_ctl_out.bout = ALU_CTL_AND;
        }
    }

    return alu_ctl_out;
}

struct imm_gen_output_t imm_gen(struct imm_gen_input_t gen_in)
{
    struct imm_gen_output_t gen_out;
    uint32_t offset = 0;
    uint8_t signBit = extract_bits(*((uint32_t*)&gen_in.imem_out), 31, 1);
    
    // 코드를 보기 좋게 만들기 위한 임시적인 변수들입니다.
    // temp. B와 J format의 추가적인 연산을 위한 변수입니다.
    uint32_t I_format_imm = 0;
    uint32_t S_format_imm = 0;
    uint32_t U_format_imm = 0;
    uint32_t temp = 0;              

    I_format_imm = gen_in.imem_out.i.imm1;

    S_format_imm = gen_in.imem_out.s.imm1;
    S_format_imm |= left_shift_gate(gen_in.imem_out.s.imm2, 5);


    gen_out.dout = two_to_one_mux32(I_format_imm, S_format_imm, extract_bits(gen_in.imm, 0, 1));
    gen_out.dout = sign_extender(gen_out.dout, 12, signBit);

    // B format의 추가적인 연산입니다.
    temp = gen_out.dout;
    temp = temp & ~(1UL << 11);
    temp |= left_shift_gate(extract_bits(temp, 0, 1), 11);
    temp = temp & ~(1);
    gen_out.dout = two_to_one_mux32(gen_out.dout, temp, gen_in.branch);

    U_format_imm = (extract_bits(*(uint32_t*)&gen_in.imem_out, 12, 20) << 12);

    // J format의 추가적인 연산입니다.
    // 실제로는 right shift가 사용될 것이라고 생각합니다.
    // 현재는 extract_bits 함수가 정확한 bit 주소를 반환하지 않기 때문에 left shift를 사용했습니다.
    temp = U_format_imm;
    temp |= left_shift_gate(extract_bits(temp, 20, 1), 11);
    temp |= left_shift_gate(extract_bits(temp, 21, 10), 1);
    temp |= left_shift_gate(extract_bits(temp, 31, 1), 20);
    temp = sign_extender(temp, 20, signBit);

    U_format_imm = two_to_one_mux32(U_format_imm, temp, gen_in.jump);

    gen_out.dout = two_to_one_mux32(gen_out.dout, U_format_imm, extract_bits(gen_in.imm, 1, 1));

    printf("Immediate generator generate %08X\n", gen_out.dout);

    
    return gen_out;
}

struct alu_output_t alu(struct alu_input_t alu_in)
{
    struct alu_output_t alu_out;

    switch(alu_in.alu_ctl){
        case (ALU_CTL_AND): {
            alu_out.result = (alu_in.in1 & alu_in.in2);
            printf("ALU(AND) - %08X & %08X => %08X\n", alu_in.in1, alu_in.in2, alu_out.result);
        } break;
        case (ALU_CTL_OR): {
            alu_out.result = (alu_in.in1 | alu_in.in2);
            printf("ALU(OR) - %08X | %08X => %08X\n", alu_in.in1, alu_in.in2, alu_out.result);
        } break;
        case (ALU_CTL_ADD): {
            alu_out.result = (alu_in.in1 + alu_in.in2);
            printf("ALU(ADD) - %08X + %08X => %08X\n", alu_in.in1, alu_in.in2, alu_out.result);
        } break;
        case (ALU_CTL_XOR): {
            alu_out.result = (alu_in.in1 ^ alu_in.in2);
            printf("ALU(XOR) - %08X ^ %08X => %08X\n", alu_in.in1, alu_in.in2, alu_out.result);
        } break;
        case (ALU_CTL_SLL): {
            alu_out.result = (alu_in.in1 << alu_in.in2);
            printf("ALU(SLL) - %08X << %08X => %08X\n", alu_in.in1, alu_in.in2, alu_out.result);
        } break;
        case (ALU_CTL_SLT): {
            alu_out.result = ((int32_t)alu_in.in1 < (int32_t)alu_in.in2) ? 1 : 0;
            printf("ALU(SLT) - ?(%08X < %08X  => %08X\n", alu_in.in1, alu_in.in2, alu_out.result);
        } break;
        case (ALU_CTL_SUB): {
            alu_out.result = (alu_in.in1 - alu_in.in2);
            printf("ALU(SUB) - %08X - %08X => %08X\n", alu_in.in1, alu_in.in2, alu_out.result);
        } break;
        case (ALU_CTL_SLTU): {
            alu_out.result = (alu_in.in1 < alu_in.in2) ? 1 : 0;
            printf("ALU(SLTU) - ?(%08X < %08X  => %08X\n", alu_in.in1, alu_in.in2, alu_out.result);
        } break;
        case (ALU_CTL_SRL): {
            alu_out.result = (alu_in.in1 >> alu_in.in2);
            printf("ALU(SRL) - (%08X > %08X  => %08X\n", alu_in.in1, alu_in.in2, alu_out.result);
        } break;
        case (ALU_CTL_SRLA): {
            alu_out.result = ((int32_t)alu_in.in1 >> alu_in.in2);
            printf("ALU(SRLA) - (%08X > %08X  => %08X\n", alu_in.in1, alu_in.in2, alu_out.result);
        } break;
        default: {
            printf("Error in alu : alu received %d as an opcode", alu_in.alu_ctl);
            exit(1);
        } break;
    }

    alu_out.br_eq_sig = (alu_out.result == 0) ? 1 : 0;
    alu_out.br_lt_sig = ((alu_out.result & (1UL << 31)) > 0 ? 1 : 0);

    alu_out.br_ltu_sig = 0;
    for(int mask = 31; mask > -1; mask--){
        uint8_t bit_in1 = extract_bits(alu_in.in1, mask, 1);
        uint8_t bit_in2 = extract_bits(alu_in.in2, mask, 1);
        if((bit_in1 ^ bit_in2) != 0){
            alu_out.br_ltu_sig = bit_in2;
            break;
        }
    }
    printf("ALU(branch signals)\n eq: %d, lt: %d, ltu: %d\n", alu_out.br_eq_sig, alu_out.br_lt_sig, alu_out.br_ltu_sig);

    return alu_out;
}


struct branch_ctl_output_t branch_ctl(struct branch_ctl_input_t br_in)
{
    struct branch_ctl_output_t br_out;
    br_out.bout = four_to_one_mux32(br_in.br_eq_sig, 0, br_in.br_lt_sig, br_in.br_ltu_sig, br_in.cond_branch);
    br_out.bout = xor_gate(br_out.bout, br_in.reverse);
    br_out.bout = and_gate(br_in.branch, or_gate(br_out.bout, br_in.jump));

    printf("Branch control signal - %d\n", br_out.bout);
    return br_out;
}


struct dmem_output_t dmem(struct dmem_input_t dmem_in, uint32_t *dmem_data)
{
    struct dmem_output_t dmem_out;

    uint32_t bitmask = ((1UL << (0b1000 << dmem_in.acc_unit)) - 1);
    
    if(dmem_in.mem_write){
        uint32_t temp = dmem_data[dmem_in.addr];        //temp variable for logging.
        
        printf("Memory(write) - from %08X ", dmem_data[dmem_in.addr]);
        dmem_data[dmem_in.addr] &= ~(dmem_data[dmem_in.addr] & bitmask);
        dmem_data[dmem_in.addr] |= (dmem_in.din & bitmask);
        printf("to %08x, addr: %d\n", dmem_data[dmem_in.addr], dmem_in.addr);
    
        add_log_value(temp, dmem_data[dmem_in.addr], dmem_in.addr, 1);
    }

    if(dmem_in.mem_read){
        dmem_out.dout = (dmem_data[dmem_in.addr] & bitmask);
        printf("Memory(read) - from %08X, addr: %d\n", dmem_out.dout, dmem_in.addr);
    }

    return dmem_out;
}


uint32_t extract_bits(uint32_t target, uint8_t start, uint8_t length)
{
    uint32_t mask = (1UL << length) - 1;
    return (target >> start) & mask;
}

uint32_t two_to_one_mux32(uint32_t input_0, uint32_t input_1, uint8_t signal)
{
    return (signal == 0) ? input_0 : input_1;
}

uint32_t four_to_one_mux32(uint32_t input_0, uint32_t input_1, uint32_t input_2, uint32_t input_3, uint8_t signal)
{
    return two_to_one_mux32(two_to_one_mux32(input_0, input_1, extract_bits(signal, 0, 1)), two_to_one_mux32(input_2, input_3, extract_bits(signal, 0, 1)), extract_bits(signal, 1, 1));;
}

uint32_t add_gate32(uint32_t input_0, uint32_t input_1)
{
    return (input_0 + input_1);
}

uint8_t and_gate(uint8_t input_0, uint8_t input_1)
{
    return (input_0 & input_1);
}

uint8_t or_gate(uint8_t input_0, uint8_t input_1)
{
    return (input_0 | input_1);
}

uint8_t xor_gate(uint8_t input_0, uint8_t input_1)
{
    return (input_0 ^ input_1);
}

uint8_t not_gate(uint8_t input_0)
{
    return (input_0 == 0 ? 1 : 0);
}

uint32_t left_shift_gate(uint32_t input_0, uint32_t amount)
{
    return (input_0 << amount);
}

uint32_t sign_extender(uint32_t target, uint8_t size, uint8_t sign)
{
    uint32_t out = target & ((1UL << size) - 1);
    if (sign) {
        out |= ~((1UL << size) - 1);
    }

    return out;
}
