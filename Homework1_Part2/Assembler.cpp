#include <cstdint>

#include "Header_Assembler.h"
#include <sstream>
#include <iomanip>

static uint32_t currentDataAddress = 0x10010000;
static std::vector<uint8_t> pendingDataBytes;

std::vector<std::string> flushDataHex() {
    std::vector<std::string> result;
    if (pendingDataBytes.empty()) {
        return result;
    }

    // Pad with zeros to 4 bytes
    while (pendingDataBytes.size() < 4) {
        pendingDataBytes.push_back(0);
    }

    uint32_t word = 0;
    word |= (uint32_t)pendingDataBytes[0];
    word |= ((uint32_t)pendingDataBytes[1]) << 8;
    word |= ((uint32_t)pendingDataBytes[2]) << 16;
    word |= ((uint32_t)pendingDataBytes[3]) << 24;

    std::stringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << word;
    result.push_back(ss.str());

    pendingDataBytes.clear();
    return result;
}

std::vector<std::string> dataToHex(std::string line) {
    std::vector<std::string> result;

    // Trim line
    size_t first = line.find_first_not_of(" \t");
    if (first == std::string::npos) return result;
    size_t last = line.find_last_not_of(" \t");
    std::string trimmed = line.substr(first, (last - first + 1));

    // Check for comments
    size_t commentPos = trimmed.find('#');
    if (commentPos != std::string::npos) {
        trimmed = trimmed.substr(0, commentPos);
        if (trimmed.empty()) return result;
        last = trimmed.find_last_not_of(" \t");
        if (last == std::string::npos) return result;
        trimmed = trimmed.substr(0, last + 1);
    }

    if (trimmed.empty()) return result;

    // Check for label — a colon is only a label separator when it appears
    // before any quote character AND the token before it has no whitespace.
    // This avoids misidentifying colons inside string literals (e.g. "1,193,046: ").
    std::string content = trimmed;
    {
        size_t firstQuotePos = trimmed.find('"');
        size_t colonPos      = trimmed.find(':');
        bool hasLabel = (colonPos != std::string::npos) &&
                        (firstQuotePos == std::string::npos || colonPos < firstQuotePos);
        if (hasLabel) {
            std::string beforeColon = trimmed.substr(0, colonPos);
            if (beforeColon.find_first_of(" \t") == std::string::npos) {
                content = trimmed.substr(colonPos + 1);
                first   = content.find_first_not_of(" \t");
                if (first == std::string::npos) return result;
                last    = content.find_last_not_of(" \t");
                content = content.substr(first, (last - first + 1));
            }
        }
    }
    // Parse type
    size_t spacePos = content.find_first_of(" \t");
    std::string type;
    std::string data;

    if (spacePos == std::string::npos) {
        return result;
    }

    type = content.substr(0, spacePos);
    data = content.substr(spacePos + 1);

    // Trim data
    first = data.find_first_not_of(" \t");
    if (first != std::string::npos) {
        last = data.find_last_not_of(" \t");
        data = data.substr(first, (last - first + 1));
    }

    if (type == ".asciiz") {
        size_t firstQuote = data.find('"');
        size_t lastQuote = data.rfind('"');

        if (firstQuote != std::string::npos && lastQuote != std::string::npos && lastQuote > firstQuote) {
            std::string strContent = data.substr(firstQuote + 1, lastQuote - firstQuote - 1);

            // Process escape sequences (\n, \t, \\, etc.)
            for (size_t i = 0; i < strContent.size(); i++) {
                if (strContent[i] == '\\' && i + 1 < strContent.size()) {
                    switch (strContent[i + 1]) {
                        case 'n':  pendingDataBytes.push_back(0x0A); i++; break;
                        case 't':  pendingDataBytes.push_back(0x09); i++; break;
                        case 'r':  pendingDataBytes.push_back(0x0D); i++; break;
                        case '0':  pendingDataBytes.push_back(0x00); i++; break;
                        case '\\': pendingDataBytes.push_back(0x5C); i++; break;
                        case '"':  pendingDataBytes.push_back(0x22); i++; break;
                        default:   pendingDataBytes.push_back((uint8_t)strContent[i]); break;
                    }
                } else {
                    pendingDataBytes.push_back((uint8_t)strContent[i]);
                }
                currentDataAddress++;
            }
            // Null terminator
            pendingDataBytes.push_back(0);
            currentDataAddress++;
        }
    }

    // Process full words from buffer
    size_t processedCount = 0;
    while (pendingDataBytes.size() - processedCount >= 4) {
        uint32_t word = 0;
        word |= (uint32_t)pendingDataBytes[processedCount];
        word |= ((uint32_t)pendingDataBytes[processedCount + 1]) << 8;
        word |= ((uint32_t)pendingDataBytes[processedCount + 2]) << 16;
        word |= ((uint32_t)pendingDataBytes[processedCount + 3]) << 24;

        std::stringstream ss;
        ss << std::hex << std::setw(8) << std::setfill('0') << word;
        result.push_back(ss.str());

        processedCount += 4;
    }

    if (processedCount > 0) {
        pendingDataBytes.erase(pendingDataBytes.begin(), pendingDataBytes.begin() + processedCount);
    }

    return result;
}

// Helper to tokenize instruction string
std::vector<std::string> tokenize(std::string line) {
    size_t commentPos = line.find('#');
    if (commentPos != std::string::npos) {
        line = line.substr(0, commentPos);
    }

    std::vector<std::string> tokens;
    std::string current;
    for (char c : line) {
        if (c == ' ' || c == '\t' || c == ',' || c == '(' || c == ')') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

// ============================================================================
// resolveLabels
//
// Called in Pass 2 on each fully-expanded single instruction string BEFORE it
// is handed to instructionToHex.  Returns a vector<string> because memory
// reference labels expand into two instructions.
//
// Rules (matching real MIPS):
//
//   branch (beq, bne) — label in last operand ->
//       offset = (targetAddr - (PC + 4)) / 4   (signed 16-bit word offset)
//       Emitted as a decimal integer (may be negative).
//       Single instruction returned.
//
//   jump (j, jal) — label operand ->
//       word_addr = targetAddr / 4   (26-bit pseudo-direct)
//       Emitted as "0x...". Single instruction returned.
//
//   la / li — label operand ->
//       Replaced with the full 32-bit address "0x..." and passed back so that
//       expandPseudo can split it into lui+ori.  Single instruction returned.
//
//   lw / sw / lb / sb / lh / sh — label as address operand ->
//       Expands to TWO instructions:
//           lui $at, upper(addr)
//           lw/sw $rt, lower(addr)($at)
//       This mirrors the standard MIPS assembler pseudo expansion.
//       Two instructions returned.
// ============================================================================
std::vector<std::string> resolveLabels(const std::string& instr,
                                       const std::unordered_map<std::string, uint32_t>& textLabels,
                                       const std::unordered_map<std::string, uint32_t>& dataLabels,
                                       uint32_t currentPC)
{
    std::vector<std::string> result;
    std::vector<std::string> tokens = tokenize(instr);
    if (tokens.empty()) { result.push_back(instr); return result; }

    std::string op = tokens[0];

    // ---- Branch instructions: beq, bne ----
    // Format: beq $rs, $rt, LABEL   (label is tokens[3])
    if ((op == "beq" || op == "bne") && tokens.size() >= 4) {
        auto it = textLabels.find(tokens[3]);
        if (it != textLabels.end()) {
            int32_t offset = (int32_t)((it->second - (currentPC + 4)) / 4);
            std::ostringstream oss;
            oss << op << " " << tokens[1] << ", " << tokens[2] << ", " << offset;
            result.push_back(oss.str());
            return result;
        }
        result.push_back(instr);
        return result;
    }

    // ---- Jump instructions: j, jal ----
    if ((op == "j" || op == "jal") && tokens.size() >= 2) {
        auto it = textLabels.find(tokens[1]);
        if (it != textLabels.end()) {
            std::ostringstream oss;
            oss << op << " 0x" << std::hex << (it->second / 4);
            result.push_back(oss.str());
            return result;
        }
        result.push_back(instr);
        return result;
    }

    // ---- la / li: label operand -> full 32-bit address literal ----
    if ((op == "la" || op == "li") && tokens.size() >= 3) {
        uint32_t addr = 0;
        bool found = false;
        auto dit = dataLabels.find(tokens[2]);
        if (dit != dataLabels.end()) { addr = dit->second; found = true; }
        if (!found) {
            auto tit = textLabels.find(tokens[2]);
            if (tit != textLabels.end()) { addr = tit->second; found = true; }
        }
        if (found) {
            std::ostringstream oss;
            oss << op << " " << tokens[1] << ", 0x" << std::hex << addr;
            result.push_back(oss.str());
            return result;
        }
        result.push_back(instr);
        return result;
    }

    // ---- Memory instructions with a data label as the address ----
    // Supported forms written in MIPS source:
    //   lw $rt, myLabel          (tokens: op, rt, label)
    //   sw $rt, myLabel          (tokens: op, rt, label)
    //   lb/sb/lh/sh likewise
    //
    // Expands to:
    //   lui $at, upper(addr)
    //   lw  $rt, lower(addr)($at)
    bool isMemOp = (op == "lw" || op == "sw" ||
                    op == "lb" || op == "sb" ||
                    op == "lh" || op == "sh");

    if (isMemOp && tokens.size() >= 3) {
        // The address operand is tokens[2] when written as "lw $rt, myLabel"
        // (the tokenizer strips parens so "lw $rt, 0($sp)" gives tokens[2]="0",
        //  tokens[3]="$sp" — two tokens — whereas a bare label gives one token)
        std::string addrToken = tokens[2];
        bool hasBase = (tokens.size() >= 4); // already has explicit base reg

        if (!hasBase) {
            // Only try to resolve if the token is not a plain integer / hex literal
            bool isNumeric = (!addrToken.empty() &&
                              (addrToken[0] == '-' || isdigit((unsigned char)addrToken[0]) ||
                               addrToken.substr(0, 2) == "0x"));

            if (!isNumeric) {
                auto dit = dataLabels.find(addrToken);
                if (dit != dataLabels.end()) {
                    uint32_t addr  = dit->second;
                    uint16_t upper = (uint16_t)((addr >> 16) & 0xFFFF);
                    uint16_t lower = (uint16_t)(addr & 0xFFFF);

                    // lui $at, upper(addr)
                    std::ostringstream lui;
                    lui << "lui $at, 0x" << std::hex << upper;
                    result.push_back(lui.str());

                    // lw $rt, lower(addr)($at)
                    std::ostringstream mem;
                    mem << op << " " << tokens[1] << ", 0x"
                        << std::hex << lower << "($at)";
                    result.push_back(mem.str());
                    return result;
                }
            }
        }
    }

    // Not an instruction that uses labels — pass through unchanged
    result.push_back(instr);
    return result;
}

// ============================================================================
// expandPseudo  (updated to accept an optional label map so that la/li with
// label operands produce correct lui/ori pairs in pass 2)
// ============================================================================
std::vector<std::string> expandPseudo(const std::string& line,
                                      const std::unordered_map<std::string, uint32_t>* dataLabels,
                                      const std::unordered_map<std::string, uint32_t>* textLabels)
{
    std::vector<std::string> instructions;
    std::vector<std::string> tokens = tokenize(line);

    if (tokens.empty()) return instructions;

    std::string op = tokens[0];

    if (op == "move") {
        if (tokens.size() >= 3) {
            instructions.push_back("add " + tokens[1] + ", $zero, " + tokens[2]);
        }
    } else if (op == "blt") {
        if (tokens.size() >= 4) {
            instructions.push_back("slt $at, " + tokens[1] + ", " + tokens[2]);
            instructions.push_back("bne $at, $zero, " + tokens[3]);
        }
    } else if (op == "li" || op == "la") {
        if (tokens.size() >= 3) {
            long long imm = 0;
            bool is_label = false;
            std::string immStr = tokens[2];

            // 1. Try label maps first (only if provided)
            if (dataLabels) {
                auto it = dataLabels->find(immStr);
                if (it != dataLabels->end()) { imm = (long long)it->second; goto imm_resolved; }
            }
            if (textLabels) {
                auto it = textLabels->find(immStr);
                if (it != textLabels->end()) { imm = (long long)it->second; goto imm_resolved; }
            }
            // 2. Fall through to numeric literal parse (covers li $v0, 4 etc.)
            if (immStr.find("0x") != std::string::npos) {
                try { imm = std::stoll(immStr, nullptr, 16); }
                catch (...) { is_label = true; }
            } else {
                try {
                    size_t pos;
                    imm = std::stoll(immStr, &pos, 10);
                    if (pos != immStr.length()) is_label = true;
                } catch (...) { is_label = true; }
            }
            imm_resolved:;

            int upper = is_label ? 0 : (int)((imm >> 16) & 0xFFFF);
            int lower = is_label ? 0 : (int)(imm & 0xFFFF);

            std::stringstream ssLui, ssOri;
            ssLui << "lui $at, 0x" << std::hex << upper;
            instructions.push_back(ssLui.str());
            ssOri << "ori " << tokens[1] << ", $at, 0x" << std::hex << lower;
            instructions.push_back(ssOri.str());
        }
    } else {
        instructions.push_back(line);
    }

    return instructions;
}

//Edited with AI, created by boyajia4
int instructionToHex(const char *mipsInstruction) {
    if (mipsInstruction == nullptr)
        return -1;

    char concat_argv[100];
    strcpy(concat_argv, mipsInstruction);

    int index_of_cleaned = 0;
    char cleaned_argv[4][32] = {"\0","\0","\0","\0"};
    int arg_start = -1;

    for (int i = 0; i <= (int)strlen(concat_argv); i++) {
        if (concat_argv[i] == ' ' || concat_argv[i] == 9 || concat_argv[i] == ',' ||
            concat_argv[i] == '(' || concat_argv[i] == ')' ||
            concat_argv[i] == '\0' || concat_argv[i] == '#') {

            if (arg_start != -1) {
                char current_arg[100];
                int len = i - arg_start;
                for (int j = 0; j < len; j++) {
                    current_arg[j] = concat_argv[arg_start + j];
                }
                current_arg[len] = '\0';
                strcpy(cleaned_argv[index_of_cleaned], current_arg);
                ++index_of_cleaned;
                arg_start = -1;
            }
            if (concat_argv[i] == '#' || index_of_cleaned == 4)
                break;
            continue;
        }
        if (arg_start == -1)
            arg_start = i;
    }
    int new_argc = index_of_cleaned;

    if (new_argc == 1) {
        return Mnemonic_To_Opcode(cleaned_argv[0], 0);
    }

    unsigned result = 0;
    int temp = Mnemonic_To_Opcode(cleaned_argv[0], 0);
    if (temp < 0) return -1;
    temp <<= (32 - 6);
    result |= temp;

    if (new_argc == 2) {
        result |= Char_Array_To_Register(cleaned_argv[1]);
        return result;
    }

    if (new_argc == 3) {
        if (result >> (32 - 6) == 35 || result >> (32 - 6) == 43) {
            temp = Assembly_Name_To_Register(cleaned_argv[2]);
            if (temp < 0) return -7;
            temp <<= (32 - 6 - 5);
            result |= temp;

            temp = Assembly_Name_To_Register(cleaned_argv[1]);
            if (temp < 0) return -8;
            temp <<= (32 - 6 - 5 - 5);
            result |= temp;
        } else {
            // lui $rt, imm
            temp = Assembly_Name_To_Register(cleaned_argv[1]);
            if (temp < 0) return -2;
            temp <<= (32 - 6 - 5 - 5);
            result |= temp;
            result |= Char_Array_To_Register(cleaned_argv[2]);
        }
        return result;
    }

    if (!strstr(cleaned_argv[3], "$")) { // I-type
        if (result >> (32 - 6) == 4 || result >> (32 - 6) == 5) {
            // beq / bne: rs, rt, offset
            temp = Assembly_Name_To_Register(cleaned_argv[1]);
            if (temp < 0) return -3;
            temp <<= (32 - 6 - 5);
            result |= temp;

            temp = Assembly_Name_To_Register(cleaned_argv[2]);
            if (temp < 0) return -4;
            temp <<= (32 - 6 - 5 - 5);
            result |= temp;
        } else {
            temp = Assembly_Name_To_Register(cleaned_argv[2]);
            if (temp < 0) return -5;
            temp <<= (32 - 6 - 5);
            result |= temp;

            temp = Assembly_Name_To_Register(cleaned_argv[1]);
            if (temp < 0) return -6;
            temp <<= (32 - 6 - 5 - 5);
            result |= temp;
        }
        temp = Char_Array_To_Register(cleaned_argv[3]);
        temp &= 0xFFFF; // keep only 16 bits (handles negative offsets correctly)
        result |= temp;

    } else { // R-type (and lw/sw with offset)
        if (result >> (32 - 6) == 35 || result >> (32 - 6) == 43) {
            temp = Assembly_Name_To_Register(cleaned_argv[3]);
            if (temp < 0) return -7;
            temp <<= (32 - 6 - 5);
            result |= temp;

            temp = Assembly_Name_To_Register(cleaned_argv[1]);
            if (temp < 0) return -8;
            temp <<= (32 - 6 - 5 - 5);
            result |= temp;

            temp = Char_Array_To_Register(cleaned_argv[2]);
            if (temp < 0) return -9;
            temp <<= (32 - 6 - 5 - 5 - 16);
            result |= temp;
        } else {
            temp = Assembly_Name_To_Register(cleaned_argv[2]);
            if (temp < 0) return -10;
            temp <<= (32 - 6 - 5);
            result |= temp;

            temp = Assembly_Name_To_Register(cleaned_argv[3]);
            if (temp < 0) return -11;
            temp <<= (32 - 6 - 5 - 5);
            result |= temp;

            temp = Assembly_Name_To_Register(cleaned_argv[1]);
            if (temp < 0) return -12;
            temp <<= (32 - 6 - 5 - 5 - 5);
            result |= temp;

            temp = Mnemonic_To_Opcode(cleaned_argv[0], 1);
            if (temp < 0) return -13;
            result |= temp; // funct field sits in bits [5:0] — no shift needed
        }
    }

    return result;
}