//===- MipsDisassembler.cpp - Disassembler for Mips -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is part of the Mips Disassembler.
//
//===----------------------------------------------------------------------===//

#include "Mips.h"
#include "MipsSubtarget.h"
#include "llvm/MC/EDInstInfo.h"
#include "llvm/MC/MCDisassembler.h"
#include "llvm/Support/MemoryObject.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/Support/MathExtras.h"


#include "MipsGenEDInfo.inc"

using namespace llvm;

typedef MCDisassembler::DecodeStatus DecodeStatus;

namespace {

/// MipsDisassembler - a disasembler class for Mips32.
class MipsDisassembler : public MCDisassembler {
public:
  /// Constructor     - Initializes the disassembler.
  ///
  MipsDisassembler(const MCSubtargetInfo &STI, bool bigEndian) :
    MCDisassembler(STI), isBigEndian(bigEndian) {
  }

  ~MipsDisassembler() {
  }

  /// getInstruction - See MCDisassembler.
  DecodeStatus getInstruction(MCInst &instr,
                              uint64_t &size,
                              const MemoryObject &region,
                              uint64_t address,
                              raw_ostream &vStream,
                              raw_ostream &cStream) const;

  /// getEDInfo - See MCDisassembler.
  const EDInstInfo *getEDInfo() const;

private:
  bool isBigEndian;
};


/// Mips64Disassembler - a disasembler class for Mips64.
class Mips64Disassembler : public MCDisassembler {
public:
  /// Constructor     - Initializes the disassembler.
  ///
  Mips64Disassembler(const MCSubtargetInfo &STI, bool bigEndian) :
    MCDisassembler(STI), isBigEndian(bigEndian) {
  }

  ~Mips64Disassembler() {
  }

  /// getInstruction - See MCDisassembler.
  DecodeStatus getInstruction(MCInst &instr,
                              uint64_t &size,
                              const MemoryObject &region,
                              uint64_t address,
                              raw_ostream &vStream,
                              raw_ostream &cStream) const;

  /// getEDInfo - See MCDisassembler.
  const EDInstInfo *getEDInfo() const;

private:
  bool isBigEndian;
};

} // end anonymous namespace

const EDInstInfo *MipsDisassembler::getEDInfo() const {
  return instInfoMips;
}

const EDInstInfo *Mips64Disassembler::getEDInfo() const {
  return instInfoMips;
}

// Decoder tables for Mips register
static const uint16_t CPURegsTable[] = {
  Mips::ZERO, Mips::AT, Mips::V0, Mips::V1,
  Mips::A0, Mips::A1, Mips::A2, Mips::A3,
  Mips::T0, Mips::T1, Mips::T2, Mips::T3,
  Mips::T4, Mips::T5, Mips::T6, Mips::T7,
  Mips::S0, Mips::S1, Mips::S2, Mips::S3,
  Mips::S4, Mips::S5, Mips::S6, Mips::S7,
  Mips::T8, Mips::T9, Mips::K0, Mips::K1,
  Mips::GP, Mips::SP, Mips::FP, Mips::RA
};

static const uint16_t FGR32RegsTable[] = {
  Mips::F0, Mips::F1, Mips::F2, Mips::F3,
  Mips::F4, Mips::F5, Mips::F6, Mips::F7,
  Mips::F8, Mips::F9, Mips::F10, Mips::F11,
  Mips::F12, Mips::F13, Mips::F14, Mips::F15,
  Mips::F16, Mips::F17, Mips::F18, Mips::F18,
  Mips::F20, Mips::F21, Mips::F22, Mips::F23,
  Mips::F24, Mips::F25, Mips::F26, Mips::F27,
  Mips::F28, Mips::F29, Mips::F30, Mips::F31
};

static const uint16_t CPU64RegsTable[] = {
  Mips::ZERO_64, Mips::AT_64, Mips::V0_64, Mips::V1_64,
  Mips::A0_64, Mips::A1_64, Mips::A2_64, Mips::A3_64,
  Mips::T0_64, Mips::T1_64, Mips::T2_64, Mips::T3_64,
  Mips::T4_64, Mips::T5_64, Mips::T6_64, Mips::T7_64,
  Mips::S0_64, Mips::S1_64, Mips::S2_64, Mips::S3_64,
  Mips::S4_64, Mips::S5_64, Mips::S6_64, Mips::S7_64,
  Mips::T8_64, Mips::T9_64, Mips::K0_64, Mips::K1_64,
  Mips::GP_64, Mips::SP_64, Mips::FP_64, Mips::RA_64
};

static const uint16_t FGR64RegsTable[] = {
  Mips::D0_64,  Mips::D1_64,  Mips::D2_64,  Mips::D3_64,
  Mips::D4_64,  Mips::D5_64,  Mips::D6_64,  Mips::D7_64,
  Mips::D8_64,  Mips::D9_64,  Mips::D10_64, Mips::D11_64,
  Mips::D12_64, Mips::D13_64, Mips::D14_64, Mips::D15_64,
  Mips::D16_64, Mips::D17_64, Mips::D18_64, Mips::D19_64,
  Mips::D20_64, Mips::D21_64, Mips::D22_64, Mips::D23_64,
  Mips::D24_64, Mips::D25_64, Mips::D26_64, Mips::D27_64,
  Mips::D28_64, Mips::D29_64, Mips::D30_64, Mips::D31_64
};

static const uint16_t AFGR64RegsTable[] = {
  Mips::D0,  Mips::D1,  Mips::D2,  Mips::D3,
  Mips::D4,  Mips::D5,  Mips::D6,  Mips::D7,
  Mips::D8,  Mips::D9,  Mips::D10, Mips::D11,
  Mips::D12, Mips::D13, Mips::D14, Mips::D15
};

// Forward declare these because the autogenerated code will reference them.
// Definitions are further down.
static DecodeStatus DecodeCPU64RegsRegisterClass(MCInst &Inst,
                                                 unsigned RegNo,
                                                 uint64_t Address,
                                                 const void *Decoder);

static DecodeStatus DecodeCPURegsRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder);

static DecodeStatus DecodeFGR64RegisterClass(MCInst &Inst,
                                             unsigned RegNo,
                                             uint64_t Address,
                                             const void *Decoder);

static DecodeStatus DecodeFGR32RegisterClass(MCInst &Inst,
                                             unsigned RegNo,
                                             uint64_t Address,
                                             const void *Decoder);

static DecodeStatus DecodeCCRRegisterClass(MCInst &Inst,
                                           unsigned RegNo,
                                           uint64_t Address,
                                           const void *Decoder);

static DecodeStatus DecodeHWRegsRegisterClass(MCInst &Inst,
                                              unsigned Insn,
                                              uint64_t Address,
                                              const void *Decoder);

static DecodeStatus DecodeAFGR64RegisterClass(MCInst &Inst,
                                              unsigned RegNo,
                                              uint64_t Address,
                                              const void *Decoder);

static DecodeStatus DecodeHWRegs64RegisterClass(MCInst &Inst,
                                                unsigned Insn,
                                                uint64_t Address,
                                                const void *Decoder);

static DecodeStatus DecodeBranchTarget(MCInst &Inst,
                                       unsigned Offset,
                                       uint64_t Address,
                                       const void *Decoder);

static DecodeStatus DecodeBC1(MCInst &Inst,
                              unsigned Insn,
                              uint64_t Address,
                              const void *Decoder);


static DecodeStatus DecodeJumpTarget(MCInst &Inst,
                                     unsigned Insn,
                                     uint64_t Address,
                                     const void *Decoder);

static DecodeStatus DecodeMem(MCInst &Inst,
                              unsigned Insn,
                              uint64_t Address,
                              const void *Decoder);

static DecodeStatus DecodeFMem(MCInst &Inst, unsigned Insn,
                               uint64_t Address,
                               const void *Decoder);

static DecodeStatus DecodeSimm16(MCInst &Inst,
                                 unsigned Insn,
                                 uint64_t Address,
                                 const void *Decoder);

static DecodeStatus DecodeCondCode(MCInst &Inst,
                                   unsigned Insn,
                                   uint64_t Address,
                                   const void *Decoder);

static DecodeStatus DecodeInsSize(MCInst &Inst,
                                  unsigned Insn,
                                  uint64_t Address,
                                  const void *Decoder);

static DecodeStatus DecodeExtSize(MCInst &Inst,
                                  unsigned Insn,
                                  uint64_t Address,
                                  const void *Decoder);

namespace llvm {
extern Target TheMipselTarget, TheMipsTarget, TheMips64Target,
              TheMips64elTarget;
}

static MCDisassembler *createMipsDisassembler(
                       const Target &T,
                       const MCSubtargetInfo &STI) {
  return new MipsDisassembler(STI,true);
}

static MCDisassembler *createMipselDisassembler(
                       const Target &T,
                       const MCSubtargetInfo &STI) {
  return new MipsDisassembler(STI,false);
}

static MCDisassembler *createMips64Disassembler(
                       const Target &T,
                       const MCSubtargetInfo &STI) {
  return new Mips64Disassembler(STI,true);
}

static MCDisassembler *createMips64elDisassembler(
                       const Target &T,
                       const MCSubtargetInfo &STI) {
  return new Mips64Disassembler(STI, false);
}

extern "C" void LLVMInitializeMipsDisassembler() {
  // Register the disassembler.
  TargetRegistry::RegisterMCDisassembler(TheMipsTarget,
                                         createMipsDisassembler);
  TargetRegistry::RegisterMCDisassembler(TheMipselTarget,
                                         createMipselDisassembler);
  TargetRegistry::RegisterMCDisassembler(TheMips64Target,
                                         createMips64Disassembler);
  TargetRegistry::RegisterMCDisassembler(TheMips64elTarget,
                                         createMips64elDisassembler);
}


#include "MipsGenDisassemblerTables.inc"

  /// readInstruction - read four bytes from the MemoryObject
  /// and return 32 bit word sorted according to the given endianess
static DecodeStatus readInstruction32(const MemoryObject &region,
                                      uint64_t address,
                                      uint64_t &size,
                                      uint32_t &insn,
                                      bool isBigEndian) {
  uint8_t Bytes[4];

  // We want to read exactly 4 Bytes of data.
  if (region.readBytes(address, 4, (uint8_t*)Bytes, NULL) == -1) {
    size = 0;
    return MCDisassembler::Fail;
  }

  if (isBigEndian) {
    // Encoded as a big-endian 32-bit word in the stream.
    insn = (Bytes[3] <<  0) |
           (Bytes[2] <<  8) |
           (Bytes[1] << 16) |
           (Bytes[0] << 24);
  }
  else {
    // Encoded as a small-endian 32-bit word in the stream.
    insn = (Bytes[0] <<  0) |
           (Bytes[1] <<  8) |
           (Bytes[2] << 16) |
           (Bytes[3] << 24);
  }

  return MCDisassembler::Success;
}

DecodeStatus
MipsDisassembler::getInstruction(MCInst &instr,
                                 uint64_t &Size,
                                 const MemoryObject &Region,
                                 uint64_t Address,
                                 raw_ostream &vStream,
                                 raw_ostream &cStream) const {
  uint32_t Insn;

  DecodeStatus Result = readInstruction32(Region, Address, Size,
                                          Insn, isBigEndian);
  if (Result == MCDisassembler::Fail)
    return MCDisassembler::Fail;

  // Calling the auto-generated decoder function.
  Result = decodeMipsInstruction32(instr, Insn, Address, this, STI);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    return Result;
  }

  return MCDisassembler::Fail;
}

DecodeStatus
Mips64Disassembler::getInstruction(MCInst &instr,
                                   uint64_t &Size,
                                   const MemoryObject &Region,
                                   uint64_t Address,
                                   raw_ostream &vStream,
                                   raw_ostream &cStream) const {
  uint32_t Insn;

  DecodeStatus Result = readInstruction32(Region, Address, Size,
                                          Insn, isBigEndian);
  if (Result == MCDisassembler::Fail)
    return MCDisassembler::Fail;

  // Calling the auto-generated decoder function.
  Result = decodeMips64Instruction32(instr, Insn, Address, this, STI);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    return Result;
  }
  // If we fail to decode in Mips64 decoder space we can try in Mips32
  Result = decodeMipsInstruction32(instr, Insn, Address, this, STI);
  if (Result != MCDisassembler::Fail) {
    Size = 4;
    return Result;
  }

  return MCDisassembler::Fail;
}

static DecodeStatus DecodeCPU64RegsRegisterClass(MCInst &Inst,
                                                 unsigned RegNo,
                                                 uint64_t Address,
                                                 const void *Decoder) {

  if (RegNo > 31)
    return MCDisassembler::Fail;

  Inst.addOperand(MCOperand::CreateReg(CPU64RegsTable[RegNo]));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeCPURegsRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;

  Inst.addOperand(MCOperand::CreateReg(CPURegsTable[RegNo]));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeFGR64RegisterClass(MCInst &Inst,
                                             unsigned RegNo,
                                             uint64_t Address,
                                             const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;

  Inst.addOperand(MCOperand::CreateReg(FGR64RegsTable[RegNo]));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeFGR32RegisterClass(MCInst &Inst,
                                             unsigned RegNo,
                                             uint64_t Address,
                                             const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;

  Inst.addOperand(MCOperand::CreateReg(FGR32RegsTable[RegNo]));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeCCRRegisterClass(MCInst &Inst,
                                           unsigned RegNo,
                                           uint64_t Address,
                                           const void *Decoder) {
  Inst.addOperand(MCOperand::CreateReg(RegNo));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeMem(MCInst &Inst,
                              unsigned Insn,
                              uint64_t Address,
                              const void *Decoder) {
  int Offset = SignExtend32<16>(Insn & 0xffff);
  int Reg = (int)fieldFromInstruction32(Insn, 16, 5);
  int Base = (int)fieldFromInstruction32(Insn, 21, 5);

  if(Inst.getOpcode() == Mips::SC){
    Inst.addOperand(MCOperand::CreateReg(CPURegsTable[Reg]));
  }

  Inst.addOperand(MCOperand::CreateReg(CPURegsTable[Reg]));
  Inst.addOperand(MCOperand::CreateReg(CPURegsTable[Base]));
  Inst.addOperand(MCOperand::CreateImm(Offset));

  return MCDisassembler::Success;
}

static DecodeStatus DecodeFMem(MCInst &Inst,
                               unsigned Insn,
                               uint64_t Address,
                               const void *Decoder) {
  int Offset = SignExtend32<16>(Insn & 0xffff);
  int Reg = (int)fieldFromInstruction32(Insn, 16, 5);
  int Base = (int)fieldFromInstruction32(Insn, 21, 5);

  Inst.addOperand(MCOperand::CreateReg(FGR64RegsTable[Reg]));
  Inst.addOperand(MCOperand::CreateReg(CPURegsTable[Base]));
  Inst.addOperand(MCOperand::CreateImm(Offset));

  return MCDisassembler::Success;
}


static DecodeStatus DecodeHWRegsRegisterClass(MCInst &Inst,
                                              unsigned RegNo,
                                              uint64_t Address,
                                              const void *Decoder) {
  // Currently only hardware register 29 is supported.
  if (RegNo != 29)
    return  MCDisassembler::Fail;
  Inst.addOperand(MCOperand::CreateReg(Mips::HWR29));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeCondCode(MCInst &Inst,
                                   unsigned Insn,
                                   uint64_t Address,
                                   const void *Decoder) {
  int CondCode = Insn & 0xf;
  Inst.addOperand(MCOperand::CreateImm(CondCode));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeAFGR64RegisterClass(MCInst &Inst,
                                              unsigned RegNo,
                                              uint64_t Address,
                                              const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;

  Inst.addOperand(MCOperand::CreateReg(AFGR64RegsTable[RegNo]));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeHWRegs64RegisterClass(MCInst &Inst,
                                                unsigned RegNo,
                                                uint64_t Address,
                                                const void *Decoder) {
  //Currently only hardware register 29 is supported
  if (RegNo != 29)
    return  MCDisassembler::Fail;
  Inst.addOperand(MCOperand::CreateReg(Mips::HWR29));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeBranchTarget(MCInst &Inst,
                                       unsigned Offset,
                                       uint64_t Address,
                                       const void *Decoder) {
  unsigned BranchOffset = Offset & 0xffff;
  BranchOffset = SignExtend32<18>(BranchOffset << 2) + 4;
  Inst.addOperand(MCOperand::CreateImm(BranchOffset));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeBC1(MCInst &Inst,
                              unsigned Insn,
                              uint64_t Address,
                              const void *Decoder) {
  unsigned BranchOffset = Insn & 0xffff;
  BranchOffset = SignExtend32<18>(BranchOffset << 2) + 4;
  Inst.addOperand(MCOperand::CreateImm(BranchOffset));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeJumpTarget(MCInst &Inst,
                                     unsigned Insn,
                                     uint64_t Address,
                                     const void *Decoder) {

  unsigned JumpOffset = fieldFromInstruction32(Insn, 0, 26) << 2;
  Inst.addOperand(MCOperand::CreateImm(JumpOffset));
  return MCDisassembler::Success;
}


static DecodeStatus DecodeSimm16(MCInst &Inst,
                                 unsigned Insn,
                                 uint64_t Address,
                                 const void *Decoder) {
  Inst.addOperand(MCOperand::CreateImm(SignExtend32<16>(Insn)));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeInsSize(MCInst &Inst,
                                  unsigned Insn,
                                  uint64_t Address,
                                  const void *Decoder) {
  // First we need to grab the pos(lsb) from MCInst.
  int Pos = Inst.getOperand(2).getImm();
  int Size = (int) Insn - Pos + 1;
  Inst.addOperand(MCOperand::CreateImm(SignExtend32<16>(Size)));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeExtSize(MCInst &Inst,
                                  unsigned Insn,
                                  uint64_t Address,
                                  const void *Decoder) {
  int Size = (int) Insn  + 1;
  Inst.addOperand(MCOperand::CreateImm(SignExtend32<16>(Size)));
  return MCDisassembler::Success;
}