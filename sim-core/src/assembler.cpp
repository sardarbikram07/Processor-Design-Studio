#include "assembler.h"
#include "isa.h"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

// ─── Helpers ─────────────────────────────────────────────────────────────────

static std::string trim(const std::string &s) {
  size_t start = s.find_first_not_of(" \t\r\n");
  if (start == std::string::npos)
    return "";
  size_t end = s.find_last_not_of(" \t\r\n");
  return s.substr(start, end - start + 1);
}

static std::string strip_comment(const std::string &s) {
  size_t pos = s.find('#');
  return (pos == std::string::npos) ? s : s.substr(0, pos);
}

static std::string to_upper(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), ::toupper);
  return s;
}

// Parse "$r<N>" → register index 0–15
static uint8_t parse_reg(const std::string &tok) {
  if (tok.size() < 3 || (tok[0] != '$' && tok[0] != 'R'))
    throw std::runtime_error("Invalid register: " + tok);
  std::string t = tok;
  if (t[0] == '$')
    t = t.substr(1); // strip '$'
  t = to_upper(t);
  if (t[0] != 'R')
    throw std::runtime_error("Invalid register: " + tok);
  int n = std::stoi(t.substr(1));
  if (n < 0 || n > 15)
    throw std::runtime_error("Register out of range: " + tok);
  return static_cast<uint8_t>(n);
}

// Parse an integer literal (decimal or hex).
static int32_t parse_imm(const std::string &tok) {
  try {
    size_t pos;
    int32_t v = static_cast<int32_t>(std::stol(tok, &pos, 0));
    if (pos != tok.size())
      throw std::runtime_error("");
    return v;
  } catch (...) {
    throw std::runtime_error("Invalid immediate: " + tok);
  }
}

// Tokenize a comma/space-separated operand list.
static std::vector<std::string> split_operands(const std::string &s) {
  std::vector<std::string> result;
  std::string cur;
  for (char c : s) {
    if (c == ',' || c == ' ' || c == '\t') {
      std::string t = trim(cur);
      if (!t.empty())
        result.push_back(t);
      cur.clear();
    } else {
      cur += c;
    }
  }
  std::string t = trim(cur);
  if (!t.empty())
    result.push_back(t);
  return result;
}

// Parse "N($rM)" style memory operand → {imm, base_reg}
static std::pair<int32_t, uint8_t> parse_mem_operand(const std::string &s) {
  size_t lp = s.find('(');
  size_t rp = s.find(')');
  if (lp == std::string::npos || rp == std::string::npos)
    throw std::runtime_error("Invalid memory operand: " + s);
  int32_t offset = parse_imm(trim(s.substr(0, lp)));
  uint8_t base = parse_reg(trim(s.substr(lp + 1, rp - lp - 1)));
  return {offset, base};
}

// Encode R-type
static uint32_t encode_r(Funct funct, uint8_t rs, uint8_t rt, uint8_t rd,
                         uint8_t shamt) {
  uint32_t word = 0;
  word |= (static_cast<uint32_t>(Opcode::RTYPE) & 0x3F) << 26;
  word |= (static_cast<uint32_t>(rs) & 0xF) << 22;
  word |= (static_cast<uint32_t>(rt) & 0xF) << 18;
  word |= (static_cast<uint32_t>(rd) & 0xF) << 14;
  word |= (static_cast<uint32_t>(shamt) & 0x3F) << 8;
  word |= (static_cast<uint32_t>(funct) & 0xFF);
  return word;
}

// Encode I-type
static uint32_t encode_i(Opcode op, uint8_t rs, uint8_t rt, int32_t imm) {
  if (imm < -(1 << 17) || imm >= (1 << 17))
    throw std::runtime_error("Immediate out of 18-bit signed range: " +
                             std::to_string(imm));
  uint32_t word = 0;
  word |= (static_cast<uint32_t>(op) & 0x3F) << 26;
  word |= (static_cast<uint32_t>(rs) & 0xF) << 22;
  word |= (static_cast<uint32_t>(rt) & 0xF) << 18;
  word |= (static_cast<uint32_t>(imm) & 0x3FFFF);
  return word;
}

// Encode J-type
static uint32_t encode_j(Opcode op, uint32_t word_addr) {
  uint32_t word = 0;
  word |= (static_cast<uint32_t>(op) & 0x3F) << 26;
  word |= (word_addr & 0x3FFFFFF);
  return word;
}

// ─── Two-Pass Assembler
// ───────────────────────────────────────────────────────

std::vector<uint32_t> assemble(const std::string &source) {
  std::istringstream ss(source);
  std::string line;
  std::vector<std::string>
      instr_lines; // instruction text lines (no labels, no comments)
  std::unordered_map<std::string, uint32_t> labels; // label → word address

  // ── Pass 1: build label table ─────────────────────────────────────────
  uint32_t word_addr = 0;
  while (std::getline(ss, line)) {
    line = trim(strip_comment(line));
    if (line.empty())
      continue;

    // Check if line starts with or is a label
    while (true) {
      size_t colon = line.find(':');
      if (colon == std::string::npos)
        break;

      // Everything before ':' is the label name
      std::string label = trim(line.substr(0, colon));
      if (label.find(' ') != std::string::npos ||
          label.find('\t') != std::string::npos) {
        // Colon is inside an instruction (e.g., BEQ syntax) — not a label
        break;
      }
      labels[label] = word_addr;
      line = trim(line.substr(colon + 1));
      if (line.empty())
        break;
    }

    if (!line.empty()) {
      instr_lines.push_back(line);
      word_addr++;
    }
  }

  // ── Pass 2: encode instructions ───────────────────────────────────────
  std::vector<uint32_t> program;
  program.reserve(instr_lines.size());

  for (uint32_t idx = 0; idx < instr_lines.size(); ++idx) {
    const std::string &iline = instr_lines[idx];
    std::istringstream ls(iline);
    std::string mnemonic;
    ls >> mnemonic;
    mnemonic = to_upper(mnemonic);

    // Rest of line is operands
    std::string rest;
    std::getline(ls, rest);
    rest = trim(rest);

    uint32_t encoded = 0;

    // ── R-type instructions ───────────────────────────────────────────
    if (mnemonic == "ADD" || mnemonic == "SUB" || mnemonic == "AND" ||
        mnemonic == "OR" || mnemonic == "XOR") {
      auto ops = split_operands(rest);
      if (ops.size() != 3)
        throw std::runtime_error("Line " + std::to_string(idx) + ": " +
                                 mnemonic + " requires rd, rs, rt");
      uint8_t rd = parse_reg(ops[0]);
      uint8_t rs = parse_reg(ops[1]);
      uint8_t rt = parse_reg(ops[2]);
      Funct f = (mnemonic == "ADD")   ? Funct::ADD
                : (mnemonic == "SUB") ? Funct::SUB
                : (mnemonic == "AND") ? Funct::AND
                : (mnemonic == "OR")  ? Funct::OR
                                      : Funct::XOR;
      encoded = encode_r(f, rs, rt, rd, 0);
    } else if (mnemonic == "SLL" || mnemonic == "SRL") {
      auto ops = split_operands(rest);
      if (ops.size() != 3)
        throw std::runtime_error("Line " + std::to_string(idx) + ": " +
                                 mnemonic + " requires rd, rt, shamt");
      uint8_t rd = parse_reg(ops[0]);
      uint8_t rt = parse_reg(ops[1]);
      uint8_t shamt = static_cast<uint8_t>(parse_imm(ops[2]));
      Funct f = (mnemonic == "SLL") ? Funct::SLL : Funct::SRL;
      encoded = encode_r(f, 0, rt, rd, shamt);
    } else if (mnemonic == "JR") {
      auto ops = split_operands(rest);
      if (ops.size() != 1)
        throw std::runtime_error("JR requires one register");
      uint8_t rs = parse_reg(ops[0]);
      encoded = encode_r(Funct::JR, rs, 0, 0, 0);
    }
    // ── I-type: arithmetic/logic ──────────────────────────────────────
    else if (mnemonic == "ADDI" || mnemonic == "ANDI" || mnemonic == "ORI") {
      auto ops = split_operands(rest);
      if (ops.size() != 3)
        throw std::runtime_error("Line " + std::to_string(idx) + ": " +
                                 mnemonic + " requires rt, rs, imm");
      uint8_t rt = parse_reg(ops[0]);
      uint8_t rs = parse_reg(ops[1]);
      int32_t imm = parse_imm(ops[2]);
      Opcode op = (mnemonic == "ADDI")   ? Opcode::ADDI
                  : (mnemonic == "ANDI") ? Opcode::ANDI
                                         : Opcode::ORI;
      encoded = encode_i(op, rs, rt, imm);
    }
    // ── I-type: memory ────────────────────────────────────────────────
    else if (mnemonic == "LW") {
      // LW rt, offset(base)
      auto ops = split_operands(rest);
      uint8_t rt;
      int32_t imm;
      uint8_t rs;
      if (ops.size() == 2) {
        rt = parse_reg(ops[0]);
        auto [off, base] = parse_mem_operand(ops[1]);
        imm = off;
        rs = base;
      } else if (ops.size() == 3) {
        rt = parse_reg(ops[0]);
        rs = parse_reg(ops[1]);
        imm = parse_imm(ops[2]);
      } else {
        throw std::runtime_error("LW: invalid operands");
      }
      encoded = encode_i(Opcode::LW, rs, rt, imm);
    } else if (mnemonic == "SW") {
      auto ops = split_operands(rest);
      uint8_t rt;
      int32_t imm;
      uint8_t rs;
      if (ops.size() == 2) {
        rt = parse_reg(ops[0]);
        auto [off, base] = parse_mem_operand(ops[1]);
        imm = off;
        rs = base;
      } else if (ops.size() == 3) {
        rt = parse_reg(ops[0]);
        rs = parse_reg(ops[1]);
        imm = parse_imm(ops[2]);
      } else {
        throw std::runtime_error("SW: invalid operands");
      }
      encoded = encode_i(Opcode::SW, rs, rt, imm);
    }
    // ── I-type: branches ─────────────────────────────────────────────
    else if (mnemonic == "BEQ" || mnemonic == "BNE" || mnemonic == "BLT" ||
             mnemonic == "BGE") {
      auto ops = split_operands(rest);
      if (ops.size() != 3)
        throw std::runtime_error(mnemonic + " requires rs, rt, label/offset");
      uint8_t rs = parse_reg(ops[0]);
      uint8_t rt = parse_reg(ops[1]);
      int32_t offset;
      // Operand 3 may be a label or a numeric offset
      if (labels.count(ops[2])) {
        // PC-relative: offset = target_word_addr - (current_word_addr + 1)
        int32_t target = static_cast<int32_t>(labels[ops[2]]);
        offset = target - static_cast<int32_t>(idx + 1);
      } else {
        offset = parse_imm(ops[2]);
      }
      Opcode op = (mnemonic == "BEQ")   ? Opcode::BEQ
                  : (mnemonic == "BNE") ? Opcode::BNE
                  : (mnemonic == "BLT") ? Opcode::BLT
                                        : Opcode::BGE;
      encoded = encode_i(op, rs, rt, offset);
    }
    // ── J-type ────────────────────────────────────────────────────────
    else if (mnemonic == "J" || mnemonic == "JAL") {
      auto ops = split_operands(rest);
      if (ops.size() != 1)
        throw std::runtime_error(mnemonic + " requires one label/address");
      uint32_t waddr;
      if (labels.count(ops[0])) {
        waddr = labels[ops[0]];
      } else {
        waddr = static_cast<uint32_t>(parse_imm(ops[0]));
      }
      Opcode op = (mnemonic == "J") ? Opcode::J : Opcode::JAL;
      encoded = encode_j(op, waddr);
    }
    // ── Special ───────────────────────────────────────────────────────
    else if (mnemonic == "NOP") {
      encoded = static_cast<uint32_t>(Opcode::NOP) << 26;
    } else if (mnemonic == "HALT") {
      encoded = static_cast<uint32_t>(Opcode::HALT) << 26;
    } else {
      throw std::runtime_error("Unknown mnemonic: " + mnemonic);
    }

    program.push_back(encoded);
  }

  return program;
}

std::vector<uint32_t> assemble_file(const std::string &path) {
  std::ifstream f(path);
  if (!f.is_open())
    throw std::runtime_error("Cannot open file: " + path);
  std::ostringstream ss;
  ss << f.rdbuf();
  return assemble(ss.str());
}
