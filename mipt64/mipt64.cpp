/******************************************************************************
     * File: mipt64.cpp
     * Description: emulator of mipt64 machine
     * Created: 12.05.2022
     * Author: astesh
     * Email: alexstesh11@gmail.com

******************************************************************************/

/**
 * MIPT64 - Neumann's architecture machine with address space of 2^21 64-bits words.
 * This code emulates executing MIPT32 programms. Input file "input.fasm" in ASM mode and "input.bin" in BIN mode. Code example:
 *
 * main:
 *    svc r0, rz, 100
 *    add r0, r0, rz, 0, 1 ; r0++
 *    svc r0, rz, 102
 *    svc r0, rz, 0
 *    end main
 *
 * Code asks a number, increases and outputs it
*/

#include <vector>
#include <cstdio>
#include <map>
#include <string>
#include <cstring>
#include <fstream>

using namespace std;
#define MEMSIZE 2097152
#define ASMINP "input.fasm" /// file to get asm code
typedef unsigned long long int dword;

/**
 * conformity between number of command and its type in {"RI", "RR", "RM", "J"}
 */
map<dword, string> TYPE = {
        {0,  "RR"},
        {1,  "RR"},
        {2,  "RR"},
        {3,  "RR"},
        {4,  "RR"},
        {5,  "RR"},
        {6,  "RR"},
        {7,  "RR"},
        {8,  "RR"},
        {9,  "RR"},
        {10, "RR"},
        {11, "RR"},
        {12, "RR"},
        {13, "RR"},
        {14, "RR"},
        {15, "RR"},
        {16, "RR"},
        {17, "RR"},
        {18, "RR"},
        {19, "B"},
        {20, "RR"},
        {21, "RR"},
        {22, "RR"},
        {23, "RR"},
        {24, "RR"},
        {25, "RR"},
        {26, "RR"},
        {27, "RR"},
        {28, "RM"},
        {29, "RM"}
};

vector<string> input; /// asm input commands placed here
map<string, dword> label; /// map of labels - name of label as first element, number of row label start as second. Only significant rows are taken
char mem[MEMSIZE]; /// addresses space of processor
dword regs[33]; /// 16 register and 1 addictional sign register

/// masks to separate command to its part
dword m0_5 = 0b0000000000000000000000000000000011111100000000000000000000000000;
dword m6_10 = 0b0000000000000000000000000000000000000011111000000000000000000000;
dword m11_15 = 0b0000000000000000000000000000000000000000000111110000000000000000;
dword m16_31 = 0b0000000000000000000000000000000000000000000000001111111111111111;
dword m16_20 = 0b0000000000000000000000000000000000000000000000001111100000000000;
dword m21_23 = 0b0000000000000000000000000000000000000000000000000000011100000000;
dword m16_18 = 0b0000000000000000000000000000000000000000000000001110000000000000;
dword m24_31 = 0b0000000000000000000000000000000000000000000000000000000011111111;
dword m21_31 = 0b0000000000000000000000000000000000000000000000000000011111111111;
dword m11_31 = 0b0000000000000000000000000000000000000000000111111111111111111111;
dword m19_31 = 0b0000000000000000000000000000000000000000000000111111111111111111;

/// get 0-5 bits of command
dword t0_5(dword x) {
    return (x & m0_5) >> 26;
}

/// get 6-10 bits of command
dword t6_10(dword x) {
    return (x & m6_10) >> 21;
}

/// get 11-15 bits of command
dword t11_15(dword x) {
    return (x & m11_15) >> 16;
}

/// get 16-31 bits of command
dword t16_31(dword x) {
    return (x & m16_31);
}

/// get 16-20 bits of command
dword t16_20(dword x) {
    return (x & m16_20) >> 11;
}

/// get 21-23 bits of command
dword t21_23(dword x) {
    return (x & m21_23) >> 8;
}

/// get 24-31 bits of command
dword t24_31(dword x) {
    return (x & m24_31);
}

/// get 21-31 bits of command
dword t21_31(dword x) {
    return (x & m21_31);
}

/// get 11-31 bits of command
dword t11_31(dword x) {
    return (x & m11_31);
}

/// get 19-31 bits of command
dword t19_31(dword x) {
    return (x & m19_31);
}

/// get 16-18 bits of command
dword t16_18(dword x) {
    return (x & m16_18) >> 13;
}

/**
 * get value from memory
 * \param[adr] - adress to get value from it
 */
dword gmem(dword adr) {
    dword res;
    memcpy(&res, mem + adr, 8);
    return res;
}

/**
 * set value to memory
 * \param[adr] - adress to set value to it
 * \param[val] - value to set
 */
void smem(dword adr, dword val) {
    memcpy(mem + adr, &val, 8);
    mem[adr] = val;
}

/**
 * get value to register
 * \param[adr] - register to get value from it
 */
dword greg(dword adr) {
    return regs[adr];
}

/**
 * set value to register
 * \param[adr] - register to set value to it
 * \param[val] - value to set
 */
void sreg(dword adr, dword val) {
    regs[adr] = val;
}

/**
 * push value to stack
 * \param[val] - value to push
 * \param[x] - use it to move pointer on [x] BYTES
 */
 void push_stack(dword val, dword x = 8) {
    sreg(29, greg(29) - x);
    smem(greg(29), val);
}

/**
 * pop value from stack
 * \param[x] - use it to move pointer on [x] BYTES. Only first value will be returned
 */
dword pop_stack(dword x = 8) {
    dword val = gmem(greg(29));
    sreg(29, greg(29) + x);
    return val;
}

/**
 * Get assembler code from asm file and (!) write it to input vector
 */
void file_input() {
    string temp, temp1, temp2;
    ifstream fin(ASMINP);
    while (getline(fin, temp, '\n')) {
        temp = temp.substr(0, temp.find(';'));
        if (temp.find(':') != string::npos) {
            temp1 = temp.substr(0, temp.find(':') + 1);
            temp2 = temp.substr(temp.find(':') + 1, temp.length() - temp.find(':') - 1);
            while (temp2.length() > 0 && (temp2[0] == ' ' || temp2[0] == '\t')) temp2.erase(0, 1);
            if (temp2.length() > 3) temp = temp2;
            else temp = temp1;
        }
        while (temp.length() > 0 && (temp[0] == ' ' || temp[0] == '\t')) temp.erase(0, 1);
        if (temp.length() > 0) input.push_back(temp);
    }
    fin.close();
}

/**
 * Perfroms command clearing and separating to parts by comas ans spaces
 * \param[command] command to split
 */
vector<string> split(string command) {
    vector<string> res;
    while (command.find(' ') != command.find(',')) {
        char coma = false;
        size_t pos;
        if (command.find(' ') > command.find(',') + 1 and command.find(',') != -1) coma = true;
        if (coma) pos = command.find(',');
        else pos = command.find(' ');
        if (command.substr(0, pos).length() > 0) {
            if (coma) res.push_back(command.substr(0, pos + 1));
            else res.push_back(command.substr(0, pos));
        }
        if (coma) command = command.substr(pos + 1, command.length() - pos);
        else command = command.substr(pos + 1, command.length() - pos - 1);
    }
    if (command.length() > 0) {
        res.push_back(command);
    }
    return res;
}

/**
 * parse labels in input vector - find "sometext:" rows and delete it. Number of row send in label map
 */
void parse_labels() {
    for (int i = 0; i < input.size(); i++) {
        string first_part = split(input[i])[0];
        if (first_part[first_part.length() - 1] == ':') {
            label[first_part.substr(0, first_part.length() - 1)] = 8 * (i - 1);
            if (split(input[i]).size() == 1)
                input.erase(input.begin() + i);
            else
                input[i] = input[i].substr(first_part.length(), input[i].length() - first_part.length());
        }
    }
}

/**
 * Gets vector of parts of command and creates bin code of it
 * \param[lexemes] - command splitted by split function
 */
dword make_comm(vector<string> &lexemes, dword pc) {
    map<string, dword> CODE = {
            {"halt", 0},
            {"svc",  1},
            {"add",  2},
            {"sub",  3},
            {"mul",  4},
            {"div",  5},
            {"mod",  6},
            {"and",  7},
            {"or",   8},
            {"xor",  9},
            {"nand", 10},
            {"shl",  11},
            {"shr",  12},
            {"addd", 13},
            {"subd", 14},
            {"muld", 15},
            {"divd", 16},
            {"itod", 17},
            {"dtoi", 18},
            {"bl",   19},
            {"cmp",  20},
            {"cmpd", 21},
            {"cne",  22},
            {"ceq",  23},
            {"cle",  24},
            {"clt",  25},
            {"cge",  26},
            {"cgt",  27},
            {"ld",   28},
            {"st",   29}
    };
    map<string, dword> REGISTER = {
            {"rz", 27},
            {"fp", 28},
            {"sp", 29},
            {"lr", 30},
            {"pc", 31}
    };
    dword coded = CODE[lexemes[0]] << 26;
    if (TYPE[coded >> 26] == "RR") {
        dword rd, rs;
        string SRD = lexemes[1].substr(0, lexemes[1].length() - 1);
        if (REGISTER.find(SRD) != REGISTER.end()) {
            rd = REGISTER[SRD] << 21;
        } else {
            SRD = SRD.substr(1, SRD.length() - 1);
            rd = ((dword) strtol(SRD.c_str(), nullptr, 10)) << 21;
        }
        string SRS = lexemes[2].substr(0, lexemes[2].length());
        if (label.find(SRS) != label.end()) {
            rs = label[SRS];
            dword rr = 27 << 16;
            coded += rd + rr + rs;
            return coded;
        } else {
            SRS = SRS.substr(0, SRS.length() - 1);
            if (REGISTER.find(SRS) != REGISTER.end()) {
                rs = REGISTER[SRS] << 16;
            } else {
                SRS = SRS.substr(1, SRS.length() - 1);
                rs = ((dword) strtol(SRS.c_str(), nullptr, 10)) << 16;
            }
            if (label.find(lexemes[3].c_str()) != label.end()) {
                dword rimm = label[lexemes[3].c_str()];
                coded += rd + rs + rimm;
                return coded;
            }
            if (rs >> 16 == 27) {
                dword imm = ((dword) strtol(lexemes[3].c_str(), nullptr, 10));
                coded += rd + rs + imm;
                return coded;
            } else {
                string SRI = lexemes[3].substr(0, lexemes[3].length() - 1);
                dword ri;
                if (REGISTER.find(SRI) != REGISTER.end()) {
                    ri = REGISTER[SRI] << 11;
                } else {
                    SRI = SRI.substr(1, SRI.length() - 1);
                    ri = ((dword) strtol(SRI.c_str(), nullptr, 10)) << 11;
                }
                dword bits = ((dword) strtol(lexemes[4].c_str(), nullptr, 10)) << 8;
                dword im = ((dword) strtol(lexemes[5].c_str(), nullptr, 10));
                coded += rd + rs + ri + bits + im;
                return coded;
            }
        }
    }
    if (TYPE[coded >> 26] == "RM") {
        dword ra, rd;
        string SRD = lexemes[1].substr(0, lexemes[1].length() - 1);
        if (REGISTER.find(SRD) != REGISTER.end()) {
            rd = REGISTER[SRD] << 21;
        } else {
            SRD = SRD.substr(1, SRD.length() - 1);
            rd = ((dword) strtol(SRD.c_str(), nullptr, 10)) << 21;
        }
        string SRA = lexemes[2].substr(0, lexemes[2].length() - 1);
        if (REGISTER.find(SRA) != REGISTER.end()) {
            ra = REGISTER[SRA] << 16;
        } else {
            SRA = SRA.substr(1, SRA.length() - 1);
            ra = ((dword) strtol(SRA.c_str(), nullptr, 10)) << 16;
        }
        if (ra >> 16 == 31 or ra >> 16 == 27 or ra >> 16 == 29) {
            dword imm = ((dword) strtol(lexemes[3].c_str(), nullptr, 10));
            coded += rd + ra + imm;
            return coded;
        } else {
            string SRI = lexemes[3].substr(0, lexemes[3].length() - 1);
            dword ri;
            if (REGISTER.find(SRI) != REGISTER.end()) ri = REGISTER[SRI] << 11;
            else {
                SRI = SRI.substr(1, SRI.length() - 1);
                ri = ((dword) strtol(SRI.c_str(), nullptr, 10)) << 11;
            }
            dword bits = ((dword) strtol(lexemes[4].c_str(), nullptr, 10)) << 8;
            dword im = ((dword) strtol(lexemes[5].c_str(), nullptr, 10));
            coded += rd + ra + ri + bits + im;
            return coded;
        }
    }
    if (TYPE[coded >> 26] == "B") {
        string fp = lexemes[1];
        if (label.find(fp) != label.end()) {
            long long lim = label[fp] - pc;
            if (lim < 0) {
                lim *= -1;
                coded += 1 << 20;
            }
            coded |= lim;
            return coded;
        }
        fp = fp.substr(0, fp.length() - 1);
        if (REGISTER.find(fp) != REGISTER.end()) {
            string sp = lexemes[2].substr(0, lexemes[2].length());
            if (REGISTER[fp] == 27) {
                dword im = label[sp];
                dword ra = 27 << 21;
                coded += ra + im;
            } else if (REGISTER[fp] == 31) {
                long long lim = label[fp] - pc;
                if (lim < 0) {
                    lim *= -1;
                    coded += 1 << 20;
                }
                coded |= lim;
            }
            return coded;
        }
        fp = fp.substr(1, fp.length() - 1);
        dword reg = 31 << 21;
        dword ra = strtol(fp.c_str(), nullptr, 10);
        dword ri = strtol(lexemes[2].substr(1, lexemes[2].length() - 1).c_str(), nullptr, 10);
        dword bits = strtol(lexemes[4].c_str(), nullptr, 10);
        dword im = strtol(lexemes[4].c_str(), nullptr, 10);
        dword imm = greg(ra) + (greg(ri) << bits) + im;
        coded += reg + imm;
        return coded;
    }
    return 0;
}

/**
 * write prepared command to memory
 */
void assemble() {
    dword pc = 0;
    parse_labels();
    for (int i = 0; i < input.size(); i++) {
        vector<string> splited = split(input[i]);
        if (splited[0] == "end") {
            sreg(31, label[splited[1]] + 8);
            break;
        } else if (splited[0] == "word") {
            smem(pc, (dword) strtol(splited[1].c_str(), nullptr, 10));
            pc += 8;
        } else if (splited[0] == "double") {
            double temp = strtod(splited[1].c_str(), nullptr);
            dword tmp;
            memcpy(&tmp, &temp, 8);
            smem(pc, tmp);
            pc += 8;
        } else if (splited[0] == "bytes") {
            dword size = (dword) strtol(splited[1].c_str(), nullptr, 10);
            for (int j = 0; j < size / 8; j++) {
                smem(pc, 0);
                pc += 8;
            }
            if (size % 8 > 4) {
                smem(pc, 0);
            } else if (size % 8 > 0) {
                dword hos = gmem(pc);
                smem(pc, hos & 0b0000000000000000000000000000000011111111111111111111111111111111);
            }
        } else {
            dword comm = make_comm(splited, pc);
            smem(pc, comm);
            pc += 8;
        }
    }
    sreg(29, MEMSIZE - 8);
    sreg(27, 0);
}

/// every functions here emulate processor command. See processor doc to get information

void halt(dword rd, dword rs, dword imm) {
    exit((int) imm);
}

void svc(dword rd, dword rs, dword imm) {
    switch (imm) {
        case 0:
            exit(0);
        case 100:
            dword scanning_int;
            scanf("%lld", &scanning_int);
            sreg(rd, scanning_int);
            break;
        case 101:
            double ddi;
            dword dwi;
            scanf("%lf", &ddi);
            memcpy(&dwi, &ddi, 8);
            sreg(rd, (dwi << 32) >> 32);
            sreg(rd + 1, dwi >> 32);
            break;
        case 102:
            dword sending_int;
            sending_int = greg(rd);
            if (rd == 31 or rd == 30) printf("%lld", sending_int / 2 + 4);
            else printf("%lld", sending_int);
            break;
        case 103:
            dword dwo;
            dwo = greg(rd);
            double ddo;
            memcpy(&ddo, &dwo, 8);
            printf("%lg", ddo);
            break;
        case 104:
            char scanning_char;
            scanf("%c", &scanning_char);
            sreg(rd, scanning_char);
            break;
        case 105:
            char sending_char;
            sending_char = (char) greg(rd);
            printf("%c", sending_char);
            break;
    }
}

void add(dword rd, dword rs, dword imm) {
    if (rd == 31 and rs == 31) sreg(rd, imm);
    else sreg(rd, greg(rs) + imm);
}

void sub(dword rd, dword rs, dword imm) {
    sreg(rd, greg(rs) - imm);
}

void mul(dword rd, dword rs, dword imm) {
    sreg(rd, greg(rs) * imm);
}

void div(dword rd, dword rs, dword imm) {
    sreg(rd, greg(rs) / imm);
}

void mod(dword rd, dword rs, dword imm) {
    sreg(rd, greg(rs) % imm);
}

void And(dword rd, dword rs, dword imm) {
    sreg(rd, greg(rs) & imm);
}

void Or(dword rd, dword rs, dword imm) {
    sreg(rd, greg(rs) | imm);
}

void Xor(dword rd, dword rs, dword imm) {
    sreg(rd, greg(rs) ^ imm);
}

void nand(dword rd, dword rs, dword imm) {
    sreg(rd, (greg(rs) ^ imm) & greg(rs));
}

void shl(dword rd, dword rs, dword imm) {
    sreg(rd, greg(rs) << (imm & 0b111111));
}

void shr(dword rd, dword rs, dword imm) {
    sreg(rd, greg(rs) >> (imm & 0b111111));
}

void addd(dword rd, dword rs, dword imm) {
    double drs, drd, dimm;
    dword in = greg(rs);
    memcpy(&drs, &in, 8);
    memcpy(&dimm, &imm, 8);
    drd = drs + dimm;
    dword out;
    memcpy(&out, &drd, 8);
    sreg(rd, out);
}

void subd(dword rd, dword rs, dword imm) {
    double drs, drd, dimm;
    dword in = greg(rs);
    memcpy(&drs, &in, 8);
    memcpy(&dimm, &imm, 8);
    drd = drs - dimm;
    dword out;
    memcpy(&out, &drd, 8);
    sreg(rd, out);
}

void muld(dword rd, dword rs, dword imm) {
    double drs, drd, dimm;
    dword in = greg(rs);
    memcpy(&drs, &in, 8);
    memcpy(&dimm, &imm, 8);
    drd = drs * dimm;
    dword out;
    memcpy(&out, &drd, 8);
    sreg(rd, out);
}

void divd(dword rd, dword rs, dword imm) {
    double drs, drd, dimm;
    dword in = greg(rs);
    memcpy(&drs, &in, 8);
    memcpy(&dimm, &imm, 8);
    drd = drs / dimm;
    dword out;
    memcpy(&out, &drd, 8);
    sreg(rd, out);
}

void itod(dword rd, dword rs, dword imm) {
    dword in = greg(rs);
    double dout = (double) in + (double) imm;
    dword out;
    memcpy(&out, &dout, 8);
    sreg(rd, out);
}

void dtoi(dword rd, dword rs, dword imm) {
    dword in = greg(rs);
    double din, dimm;
    memcpy(&din, &in, 8);
    memcpy(&dimm, &imm, 8);
    double dout = din + dimm;
    dword out;
    memcpy(&out, &dout, 8);
    sreg(rd, out);
}

void bl(dword ra, dword imm) {
    sreg(30, greg(31));
    if (ra == 27) {
        sreg(31, imm);
    } else {
        if ((imm >> 20) % 2 == 1) {
            imm <<= 1;
            imm /= 2;
            sreg(31, greg(31) - imm);
        } else {
            sreg(31, greg(31) - imm);
        }
    }
}

void cmp(dword rd, dword rs, dword imm) {
    if (greg(rd) == greg(rs) + imm) sreg(32, 0);
    if (greg(rd) < greg(rs) + imm) sreg(32, 1);
    if (greg(rd) > greg(rs) + imm) sreg(32, 2);
}

void cmpd(dword rd, dword rs, dword imm) {
    double drd, drs, dimm;
    dword wrd = greg(rd);
    dword wrs = greg(rs);
    memcpy(&drd, &wrd, 8);
    memcpy(&drs, &wrs, 8);
    memcpy(&dimm, &imm, 8);
    if (drd == drs + dimm) sreg(32, 0);
    if (drd < drs + dimm) sreg(32, 1);
    if (drd > drs + dimm) sreg(32, 2);
}

void cne(dword rd, dword rs, dword imm) {
    if (greg(32) > 0) add(rd, rs, imm);
}

void ceq(dword rd, dword rs, dword imm) {
    if (greg(32) == 0) add(rd, rs, imm);
}

void cle(dword rd, dword rs, dword imm) {
    if (greg(32) < 2) add(rd, rs, imm);
}

void clt(dword rd, dword rs, dword imm) {
    if (greg(32) == 1) add(rd, rs, imm);
}

void cge(dword rd, dword rs, dword imm) {
    if (greg(32) != 1) add(rd, rs, imm);
}

void cgt(dword rd, dword rs, dword imm) {
    if (greg(32) == 2) add(rd, rs, imm);
}

void ld(dword rd, dword ra, dword imm) {
    if (ra == 29) {
        sreg(rd, pop_stack(imm));
    } else {
        sreg(rd, gmem(greg(ra) + imm));
    }
}

void st(dword rd, dword ra, dword imm) {
    if (ra == 29) {
        push_stack(greg(rd), imm);
    } else {
        smem(greg(ra) + imm, greg(rd));
    }
}

/**
 * Get type and args of command and call function
 * \param [type] - command to execute
 * \param [tail] - commands args
 */
void switch_c(dword row) {
    dword type = t0_5(row);
    dword rd = 0, rs = 0, imm = 0, ra = 0;
    if (TYPE[type] == "RR") {
        rd = t6_10(row);
        rs = t11_15(row);
        if (rs == 27) imm = t16_31(row);
        else if (rs == 31) imm = t16_31(row);
        else {
            if (type == 13 or type == 14 or type == 15 or type == 16) {
                dword fw = greg(t16_20(row));
                dword sw = t21_23(row);
                dword tw = t24_31(row);
                double f, r;
                memcpy(&f, &fw, 8);
                r = f * (1 << sw) + tw;
                memcpy(&imm, &r, 8);
            } else imm = (greg(t16_20(row)) << t21_23(row)) + t24_31(row);
        }
    } else if (TYPE[type] == "RM") {
        rd = t6_10(row);
        ra = t11_15(row);
        if (ra == 27 or ra == 29 or ra == 31) imm = t16_31(row);
        else {
            dword ri = t16_20(row);
            if (ri == 27) imm = t21_31(row);
            else imm = greg(ra) + (greg(t16_20(row)) << t21_23(row)) + t24_31(row);
        }
    } else if (TYPE[type] == "B") {
        ra = t6_10(row);
        if (ra == 27 or ra == 31 or ra == 0) imm = t21_31(row);
        else imm = greg(ra) + (greg(t11_15(row)) << t16_18(row)) + t19_31(row);
    }
    switch (type) {
        case 0:
            halt(rd, rs, imm);
            break;
        case 1:
            svc(rd, rs, imm);
            break;
        case 2:
            add(rd, rs, imm);
            break;
        case 3:
            sub(rd, rs, imm);
            break;
        case 4:
            mul(rd, rs, imm);
            break;
        case 5:
            div(rd, rs, imm);
            break;
        case 6:
            mod(rd, rs, imm);
            break;
        case 7:
            And(rd, rs, imm);
            break;
        case 8:
            Or(rd, rs, imm);
            break;
        case 9:
            Xor(rd, rs, imm);
            break;
        case 10:
            nand(rd, rs, imm);
            break;
        case 11:
            shl(rd, rs, imm);
            break;
        case 12:
            shr(rd, rs, imm);
            break;
        case 13:
            addd(rd, rs, imm);
            break;
        case 14:
            subd(rd, rs, imm);
            break;
        case 15:
            muld(rd, rs, imm);
            break;
        case 16:
            divd(rd, rs, imm);
            break;
        case 17:
            itod(rd, rs, imm);
            break;
        case 18:
            dtoi(rd, rs, imm);
            break;
        case 19:
            bl(ra, imm);
            break;
        case 20:
            cmp(rd, rs, imm);
            break;
        case 21:
            cmpd(rd, rs, imm);
            break;
        case 22:
            cne(rd, rs, imm);
            break;
        case 23:
            ceq(rd, rs, imm);
            break;
        case 24:
            cle(rd, rs, imm);
            break;
        case 25:
            clt(rd, rs, imm);
            break;
        case 26:
            cge(rd, rs, imm);
            break;
        case 27:
            cgt(rd, rs, imm);
            break;
        case 28:
            ld(rd, ra, imm);
            break;
        case 29:
            st(rd, ra, imm);
            break;
    }
}

/**
 * main emulating function
 */
void emulate() {
    while (true) {
        dword row_com = gmem(greg(31));
        switch_c(row_com);
        sreg(31, greg(31) + 8);
    }
}

int main() {
    file_input();
    assemble();
    emulate();
}