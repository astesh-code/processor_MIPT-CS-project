/******************************************************************************
     * File: mipt32.cpp
     * Description: emulator of mipt32 machine
     * Created: 29.04.2022
     * Author: astesh
     * Email: alexstesh11@gmail.com

******************************************************************************/

/**
 * MIPT32 - Neumann's architecture machine with address space of 2^20 32-bits words.
 * This code emulates executing MIPT32 programms. Input file "input.fasm" in ASM mode and "input.bin" in BIN mode. Code example:
 *
 *  syscall r0, 100 ; number input
 *  addi r0, 1 ; number increase
 *  syscall r0, 102 ; number output
 *  lc r1, 10 ; '\n' symbol load
 *  syscall r1, 105 ; '\n' symbol output
 *   syscall r1, 0 ; exit
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
#define MEMSIZE 1048576
#define ASMINP "input.fasm"
#define BININP "input.bin"
typedef unsigned long int word;
typedef unsigned long long int dword;

/**
 * conformity between number of command and its type in {"RI", "RR", "RM", "J"}
 */
map<word, string> TYPE = {
        {0,  "RI"},
        {1,  "RI"},
        {2,  "RR"},
        {3,  "RI"},
        {4,  "RR"},
        {5,  "RI"},
        {6,  "RR"},
        {7,  "RI"},
        {8,  "RR"},
        {9,  "RI"},
        {12, "RI"},
        {13, "RR"},
        {14, "RI"},
        {15, "RR"},
        {16, "RI"},
        {17, "RR"},
        {18, "RI"},
        {19, "RR"},
        {20, "RI"},
        {21, "RR"},
        {22, "RI"},
        {23, "RI"},
        {24, "RR"},
        {32, "RR"},
        {33, "RR"},
        {34, "RR"},
        {35, "RR"},
        {36, "RR"},
        {37, "RR"},
        {38, "RI"},
        {39, "RI"},
        {40, "RR"},
        {41, "J"},
        {42, "RI"},
        {43, "RR"},
        {44, "RI"},
        {45, "RR"},
        {46, "J"},
        {47, "J"},
        {48, "J"},
        {49, "J"},
        {50, "J"},
        {51, "J"},
        {52, "J"},
        {64, "RM"},
        {65, "RM"},
        {66, "RM"},
        {67, "RM"},
        {68, "RR"},
        {69, "RR"},
        {70, "RR"},
        {71, "RR"}
};

vector<string> input; /// asm input commands placed here
map<string, word> label; /// map of labels - name of label as first element, number of row label start as second. Only significant rows are taken
word mem[MEMSIZE]; /// addresses space of processor
word regs[17]; /// 16 register and 1 addictional sign register

/**
 * convert doubleword to double
 */
double dw_t_d(dword x) {
    double res=0;
    memcpy(&res, &x, 8);
    return res;
}
/**
 * convert double to doubleword
 */
dword d_t_dw(double x) {
    dword res=0;
    memcpy(&res, &x, 8);
    return res;
}

/// masks to separate command to its part
const word f8 = 0b11111111000000000000000000000000;
const word s4 = 0b00000000111100000000000000000000;
const word t4 = 0b00000000000011110000000000000000;
const word l20 = 0b00000000000011111111111111111111;
const word l16 = 0b00000000000000001111111111111111;
const word l24 = 0b00000000111111111111111111111111;

/// get first 8 bits of command
word tf8(word x) {
    return (x & f8) >> 24;
}

/// get 8-12 bits of command
word ts4(word x) {
    return (x & s4) >> 20;
}

/// get 12-16 bits of command
word tt4(word x) {
    return (x & t4) >> 16;
}

/// get last 20 bits of command
word tl20(word x) {
    return (x & l20);
}

/// get last 16 bits of command
word tl16(word x) {
    return (x & l16);
}

/// get last 24 bits of command
word tl24(word x) {
    return (x & l24);
}

/**
 * get value from memory
 * \param[adr] - adress to get value from it
 */
word gmem(word adr) {
    return mem[adr];
}

/**
 * set value to memory
 * \param[adr] - adress to set value to it
 * \param[val] - value to set
 */
void smem(word adr, word val) {
    mem[adr] = val;
}

/**
 * get value to register
 * \param[adr] - register to get value from it
 */
word greg(word adr) {
    return regs[adr];
}

/**
 * set value to register
 * \param[adr] - register to set value to it
 * \param[val] - value to set
 */
void sreg(word adr, word val) {
    regs[adr] = val;
}

/**
 * push value to stack
 * \param[val] - value to push
 */
void push_stack(word val) {
    sreg(14, greg(14) - 1);
    smem(greg(14), val);
}

/**
 * pop value from stack
 * \param[x] - use it to move pointer on [x] positions. Only first value will be returned
 */
word pop_stack(word x = 1) {
    word val = gmem(greg(14));
    sreg(14, greg(14) + x);
    return val;
}

/**
 * Get binary code from bin file and write it to memory
 */
void bin_input() {
    FILE *fp = fopen(BININP, "rb");
    word size = 0, size_c = 0, size_d = 0, start = 0;
    fseek(fp, 16, SEEK_SET);
    fread(&size, sizeof(word), 1, fp);
    fseek(fp, 20, SEEK_SET);
    fread(&size_c, sizeof(word), 1, fp);
    fseek(fp, 28, SEEK_SET);
    fread(&start, sizeof(word), 1, fp);
    word comm = 0;
    word pc = 0;
    for (word i = 512; i < (size + size_d + size_c + 128) * 4; i += 4) {
        fseek(fp, i, SEEK_SET);
        fread(&comm, sizeof(word), 1, fp);
        smem(pc, comm);
        pc++;
    }
    fclose(fp);
    sreg(15, start);
    sreg(14, MEMSIZE - 1);
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
            label[first_part.substr(0, first_part.length() - 1)] = i;
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
word make_comm(vector<string> &lexemes) {
    const map<string, word> CODE = {
            {"halt",    0},
            {"syscall", 1},
            {"add",     2},
            {"addi",    3},
            {"sub",     4},
            {"subi",    5},
            {"mul",     6},
            {"muli",    7},
            {"div",     8},
            {"divi",    9},
            {"lc",      12},
            {"shl",     13},
            {"shli",    14},
            {"shr",     15},
            {"shri",    16},
            {"and",     17},
            {"andi",    18},
            {"or",      19},
            {"ori",     20},
            {"xor",     21},
            {"xori",    22},
            {"not",     23},
            {"mov",     24},
            {"addd",    32},
            {"subd",    33},
            {"muld",    34},
            {"divd",    35},
            {"itod",    36},
            {"dtoi",    37},
            {"push",    38},
            {"pop",     39},
            {"call",    40},
            {"calli",   41},
            {"ret",     42},
            {"cmp",     43},
            {"cmpi",    44},
            {"cmpd",    45},
            {"jmp",     46},
            {"jne",     47},
            {"jeq",     48},
            {"jle",     49},
            {"jl",      50},
            {"jge",     51},
            {"jg",      52},
            {"load",    64},
            {"store",   65},
            {"load2",   66},
            {"store2",  67},
            {"loadr",   68},
            {"loadr2",  69},
            {"storer",  70},
            {"storer2", 71}
    };
    word coded = CODE.at(lexemes[0]) << 24;
    if (TYPE.at(coded >> 24) == "RM") {
        string reg_str = lexemes[1].substr(1, lexemes[1].length() - 1);
        string mod_str = lexemes[2];
        word reg = ((word) strtol(reg_str.c_str(), nullptr, 10)) << 20;
        word mod = (word) strtol(mod_str.c_str(), nullptr, 10);
        coded += reg + mod;
    } else if (TYPE.at(coded >> 24) == "RR") {
        string reg1_str = lexemes[1].substr(1, lexemes[1].length() - 1);
        string reg2_str = lexemes[2].substr(1, lexemes[2].length() - 1);
        string mod_str = lexemes[3];
        word reg1 = ((word) strtol(reg1_str.c_str(), nullptr, 10)) << 20;
        word reg2 = ((word) strtol(reg2_str.c_str(), nullptr, 10)) << 16;
        word mod = ((word) strtol(mod_str.c_str(), nullptr, 10));
        coded += reg1 + reg2 + mod;
    } else if (TYPE.at(coded >> 24) == "RI") {
        string reg_str = "0";
        if (coded >> 24 != 42) reg_str = lexemes[1].substr(1, lexemes[1].length() - 1);
        string mod_str = lexemes[1];
        if (coded >> 24 != 42) mod_str = lexemes[2];
        word mod;
        if (label.find(mod_str) != label.end()) mod = label[mod_str];
        else mod = ((word) strtol(mod_str.c_str(), nullptr, 10));
        word reg = ((word) strtol(reg_str.c_str(), nullptr, 10)) << 20;
        coded += reg + mod;
    } else if (TYPE.at(coded >> 24) == "J") {
        word mod;
        if (label.find(lexemes[1]) != label.end()) mod = label[lexemes[1]];
        else mod = ((word) strtol(lexemes[1].c_str(), nullptr, 10));
        coded += mod;
    }
    return coded;
}

/**
 * write prepared command to memory
 */
void assemble() {
    word pc = 0;
    parse_labels();
    for (int i = 0; i < input.size(); i++) {
        vector<string> splited = split(input[i]);
        if (splited[0] == "end") {
            sreg(15, label[splited[1]]);
            break;
        } else if (splited[0] == "word") {
            smem(pc, (word) strtol(splited[1].c_str(), nullptr, 10));
            pc++;
        } else if (splited[0] == "double") {
            double temp = strtod(splited[1].c_str(), nullptr);
            dword tmp;
            memcpy(&tmp, &temp, 8);
            smem(pc, (tmp << 32) >> 32);
            smem(pc + 1, tmp >> 32);
            pc++;
        } else {
            smem(pc, make_comm(splited));
            pc++;
        }
    }
    sreg(14, MEMSIZE-1);
}

/// every functions here emulate processor command. See processor doc to get information

void halt(word mod) {
    exit((int) mod);
}

void syscall(word reg, word arg) {
    switch (arg) {
        case 0:
            exit(0);
        case 100:
            int scanning_int;
            scanf("%d", &scanning_int);
            sreg(reg, scanning_int);
            break;
        case 101:
            double ddi;
            scanf("%lf", &ddi);
            dword dwi;
            dwi = d_t_dw(ddi);
            sreg(reg, (dwi << 32) >> 32);
            sreg(reg + 1, dwi >> 32);
            break;
        case 102:
            int sending_int;
            sending_int = (int) greg(reg);
            printf("%d", sending_int);
            break;
        case 103:
            dword dwo;
            dwo = (greg(reg) + (greg(reg + 1) << 32));
            double ddo;
            ddo = dw_t_d(dwo);
            printf("%lg", ddo);
            break;
        case 104:
            char scanning_char;
            scanf("%c", &scanning_char);
            sreg(reg, scanning_char);
            break;
        case 105:
            char sending_char;
            sending_char = (char) greg(reg);
            printf("%c", sending_char);
            break;
    }
}

void add(word r1, word r2, word mod) {
    sreg(r1, greg(r1) + greg(r2) + mod);
}

void addi(word r1, word mod) {
    sreg(r1, greg(r1) + mod);
}

void sub(word r1, word r2, word mod) {
    sreg(r1, greg(r1) - greg(r2) - mod);
}

void subi(word r1 , word mod) {
    sreg(r1, greg(r1) - mod);
}

void mul(word r1, word r2) {
    dword res = greg(r1) * greg(r2);
    sreg(r1, (res << 32) >> 32);
    sreg(r1 + 1, res >> 32);
}

void muli(word r1, word mod) {
    dword res = greg(r1) * mod;
    sreg(r1, (res << 32) >> 32);
    sreg(r1 + 1, res >> 32);
}

void div(word r1, word r2) {
    dword fir = (greg(r1 + 1) << 32) + greg(r1);
    word di = fir / greg(r2);
    word re = fir % greg(r2);
    sreg(r1, di);
    sreg(r1 + 1, re);
}

void divi(word r1, word mod) {
    dword big = (greg(r1 + 1) << 32) + greg(r1);
    word di = big / mod;
    word re = big % mod;
    sreg(r1, di);
    sreg(r1 + 1, re);
}

void lc(word r1, word mod) {
    sreg(r1, mod);
}

void shl(word r1, word r2) {
    sreg(r1, greg(r1) << greg(r2));
}

void shli(word r1, word mod) {
    sreg(r1, greg(r1) << mod);
}

void shr(word r1, word r2) {
    sreg(r1, greg(r1) >> greg(r2));
}

void shri(word r1, word mod) {
    sreg(r1, greg(r1) >> mod);
}

void and1(word r1, word r2) {
    sreg(r1, greg(r1) & greg(r2));
}

void andi(word r1, word mod) {
    sreg(r1, greg(r1) & mod);
}

void or1(word r1, word r2) {
    sreg(r1, greg(r1) | greg(r2));
}

void ori(word r1, word mod) {
    sreg(r1, greg(r1) | mod);
}

void xor1(word r1, word r2) {
    sreg(r1, greg(r1) ^ greg(r2));
}

void xori(word r1, word mod) {
    sreg(r1, greg(r1) ^ mod);
}

void not1(word r1) {
    sreg(r1, ~greg(r1));
}

void mov(word r1, word r2, word mod) {
    sreg(r1, greg(r2) + mod);
}

void addd(word r1, word r2) {
    dword mul1 = (greg(r1 + 1) << 32) + greg(r1);
    dword mul2 = (greg(r2 + 1) << 32) + greg(r2);
    double mu1 = dw_t_d(mul1), mu2 = dw_t_d(mul2);
    dword res = d_t_dw(mu1+mu2);
    sreg(r1, (res << 32) >> 32);
    sreg(r1 + 1, res >> 32);
}

void subd(word r1, word r2) {
    dword mul1 = (greg(r1 + 1) << 32) + greg(r1);
    dword mul2 = (greg(r2 + 1) << 32) + greg(r2);
    double mu1 = dw_t_d(mul1), mu2 = dw_t_d(mul2);
    dword res = d_t_dw(mu1-mu2);
    sreg(r1, (res << 32) >> 32);
    sreg(r1 + 1, res >> 32);
}

void muld(word r1, word r2) {
    dword mul1 = (greg(r1 + 1) << 32) + greg(r1);
    dword mul2 = (greg(r2 + 1) << 32) + greg(r2);
    double mu1 = dw_t_d(mul1), mu2 = dw_t_d(mul2);
    dword res = d_t_dw(mu1*mu2);
    sreg(r1, (res << 32) >> 32);
    sreg(r1 + 1, res >> 32);
}

void divd(word r1, word r2) {
    dword mul1 = (greg(r1 + 1) << 32) + greg(r1);
    dword mul2 = (greg(r2 + 1) << 32) + greg(r2);
    double mu1 = dw_t_d(mul1), mu2 = dw_t_d(mul2);
    dword res = d_t_dw(mu1/mu2);
    sreg(r1, (res << 32) >> 32);
    sreg(r1 + 1, res >> 32);
}

void itod(word r1, word r2) {
    dword res;
    double sour = greg(r2); /////////////////////////////////////////////////////////////why???
    memcpy(&res, &sour, 8);
    sreg(r1, (res << 32) >> 32);
    sreg(r1 + 1, res >> 32);
}

void dtoi(word r1, word r2) {
    dword sour = (greg(r2 + 1) << 32) + greg(r2);
    double res;
    memcpy(&res, &sour, 8);
    word re = res;
    sreg(r1, re);
}

void push(word r1, word mod) {
    push_stack(greg(r1) + mod);
}

void pop(word r1, word mod) {
    sreg(r1, pop_stack() + mod);
}

void call(word r1, word r2, word mod) {
    push_stack(greg(15)+1);
    sreg(15, greg(r2)+mod-1);
    sreg(r1, greg(14));
}

void calli(word tail) {
    push_stack(greg(15)+1);
    sreg(15, tail - 1);
}

void ret(word mod) {
    sreg(15, pop_stack(mod + 1)-1);
}

void cmp(word r1, word r2) {
    if (greg(r1) == greg(r2)) sreg(16, 0);
    else if (greg(r1) < greg(r2)) sreg(16, 1);
    else sreg(16, 2);
}

void cmpi(word r1, word mod) {
    if (greg(r1) == mod) sreg(16, 0);
    else if (greg(r1) < mod) sreg(16, 1);
    else sreg(16, 2);
}

void cmpd(word r1, word r2) {
    dword first = (greg(r1 + 1) << 32) + greg(r1);
    dword second = (greg(r2 + 1) << 32) + greg(r2);
    double fir = dw_t_d(first), sec = dw_t_d(second);
    if (fir == sec) sreg(16, 0);
    else if (fir < sec) sreg(16, 1);
    else sreg(16, 2);
}

void jmp(word tail) {
    sreg(15, tail - 1);
}

void jne(word tail) {
    if (greg(16) > 0) sreg(15, tail - 1);
}

void jeq(word tail) {
    if (greg(16) == 0) sreg(15, tail - 1);
}

void jle(word tail) {
    if (greg(16) < 2) sreg(15, tail - 1);
}

void jl(word tail) {
    if (greg(16) == 1) sreg(15, tail - 1);
}

void jge(word tail) {
    if (greg(16) != 1) sreg(15, tail - 1);
}

void jg(word tail) {
    if (greg(16) == 2) sreg(15, tail - 1);
}

void load(word r1, word mod) {
    sreg(r1, gmem(mod));
}

void store(word r1, word mod) {
    smem(mod, greg(r1));
}

void load2(word r1, word mod) {
    sreg(r1, gmem(mod));
    sreg(r1 + 1, gmem(mod + 1));
}

void store2(word r1, word mod) {
    smem(mod, greg(r1));
    smem(mod + 1, greg(r1 + 1));
}

void loadr(word r1, word r2, word mod) {
    sreg(r1, gmem(greg(r2) + mod));
}

void loadr2(word r1, word r2, word mod) {
    sreg(r1, gmem(greg(r2) + mod));
    sreg(r1 + 1, gmem(greg(r2) + mod + 1));
}

void storer(word r1, word r2, word mod) {
    smem(mod + greg(r2), greg(r1));
}

void storer2(word r1, word r2, word mod) {
    smem(mod + greg(r2), greg(r1));
    smem(mod + 1 + greg(r2), greg(r1 + 1));
}

/**
 * Get type and args of command and call function
 * \param [type] - command to execute
 * \param [tail] - commands args
 */
void switch_c(word type, word tail) {
    word r1, r2, mod;
    if (TYPE.at(type)=="RR") {
        r1 = ts4(tail);
        r2 = tt4(tail);
        mod = tl16(tail);
    }
    else if (TYPE.at(type)=="RI") {
        mod = tl20(tail);
    }
    else if (TYPE.at(type)=="RM") {
        r1 = ts4(tail);
        mod = tl20(tail);
    }
    switch (type) {
        case 0:
            halt(mod);
            break;
        case 1:
            syscall(r1, mod);
            break;
        case 2:
            add(r1, r2, mod);
            break;
        case 3:
            addi(r1, mod);
            break;
        case 4:
            sub(r1, r2, mod);
            break;
        case 5:
            subi(r1, mod);
            break;
        case 6:
            mul(r1, r2);
            break;
        case 7:
            muli(r1, mod);
            break;
        case 8:
            div(r1, r2);
            break;
        case 9:
            divi(r1, mod);
            break;
        case 12:
            lc(r1, mod);
            break;
        case 13:
            shl(r1, r2);
            break;
        case 14:
            shli(r1, mod);
            break;
        case 15:
            shr(r1, r2);
            break;
        case 16:
            shri(r1, mod);
            break;
        case 17:
            and1(r1, r2);
            break;
        case 18:
            andi(r1, mod);
            break;
        case 19:
            or1(r1, r2);
            break;
        case 20:
            ori(r1, mod);
            break;
        case 21:
            xor1(r1, r2);
            break;
        case 22:
            xori(r1, mod);
            break;
        case 23:
            not1(r1);
            break;
        case 24:
            mov(r1, r2, mod);
            break;
        case 32:
            addd(r1, r2);
            break;
        case 33:
            subd(r1, r2);
            break;
        case 34:
            muld(r1, r2);
            break;
        case 35:
            divd(r1, r2);
            break;
        case 36:
            itod(r1, r2);
            break;
        case 37:
            dtoi(r1, r2);
            break;
        case 38:
            push(r1, mod);
            break;
        case 39:
            pop(r1, mod);
            break;
        case 40:
            call(r1, r2, mod);
            break;
        case 41:
            calli(tail);
            break;
        case 42:
            ret(mod);
            break;
        case 43:
            cmp(r1, r2);
            break;
        case 44:
            cmpi(r1, mod);
            break;
        case 45:
            cmpd(r1, r2);
            break;
        case 46:
            jmp(tail);
            break;
        case 47:
            jne(tail);
            break;
        case 48:
            jeq(tail);
            break;
        case 49:
            jle(tail);
            break;
        case 50:
            jl(tail);
            break;
        case 51:
            jge(tail);
            break;
        case 52:
            jg(tail);
            break;
        case 64:
            load(r1, mod);
            break;
        case 65:
            store(r1, mod);
            break;
        case 66:
            load2(r1, mod);
            break;
        case 67:
            store2(r1, mod);
            break;
        case 68:
            loadr(r1, r2, mod);
            break;
        case 69:
            loadr2(r1, r2, mod);
            break;
        case 70:
            storer(r1, r2, mod);
            break;
        case 71:
            storer2(r1, r2, mod);
            break;
    }
}

/**
 * main emulating function
 */
void emulate() {
    while (true) {
        word row_com = gmem(greg(15));
        word type_code = tf8(row_com);
        word tail = tl24(row_com);
        switch_c(type_code, tail);
        sreg(15, greg(15) + 1);
    }
}

int main() {
    file_input();
    assemble();
    //bin_input();
    emulate();
}