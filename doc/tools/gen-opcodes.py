#!/usr/bin/env python3
"""Generate the instruction set table for the user's manual.

Reads the mnemonic list from as/keywords.c, assembles each instruction in
every addressing mode with as01, and takes the opcodes from the listing, so
the table always matches what the assembler really emits.

usage: gen-opcodes.py <keywords.c> <as01> > opcodes.md
"""

import os
import re
import subprocess
import sys
import tempfile

DESCRIPTIONS = {
    "ABA": "Add B to A",
    "ABX": "Add B to X (unsigned)",
    "ADCA": "Add with carry to A",
    "ADCB": "Add with carry to B",
    "ADDA": "Add to A",
    "ADDB": "Add to B",
    "ADDD": "Add to D",
    "ANDA": "AND into A",
    "ANDB": "AND into B",
    "ASL": "Arithmetic shift left memory",
    "ASLA": "Arithmetic shift left A",
    "ASLB": "Arithmetic shift left B",
    "ASLD": "Arithmetic shift left D",
    "ASR": "Arithmetic shift right memory",
    "ASRA": "Arithmetic shift right A",
    "ASRB": "Arithmetic shift right B",
    "BCC": "Branch if carry clear",
    "BCS": "Branch if carry set",
    "BEQ": "Branch if equal",
    "BGE": "Branch if greater or equal (signed)",
    "BGT": "Branch if greater (signed)",
    "BHI": "Branch if higher (unsigned)",
    "BHS": "Branch if higher or same (= BCC)",
    "BITA": "Bit test A",
    "BITB": "Bit test B",
    "BLE": "Branch if less or equal (signed)",
    "BLO": "Branch if lower (= BCS)",
    "BLS": "Branch if lower or same (unsigned)",
    "BLT": "Branch if less (signed)",
    "BMI": "Branch if minus",
    "BNE": "Branch if not equal",
    "BPL": "Branch if plus",
    "BRA": "Branch always",
    "BRN": "Branch never",
    "BSR": "Branch to subroutine",
    "BVC": "Branch if overflow clear",
    "BVS": "Branch if overflow set",
    "CBA": "Compare B with A",
    "CLC": "Clear carry",
    "CLI": "Clear interrupt mask",
    "CLR": "Clear memory",
    "CLRA": "Clear A",
    "CLRB": "Clear B",
    "CLV": "Clear overflow",
    "CMPA": "Compare A",
    "CMPB": "Compare B",
    "CMPX": "Compare X (= CPX)",
    "COM": "Complement memory",
    "COMA": "Complement A",
    "COMB": "Complement B",
    "CPX": "Compare X",
    "DAA": "Decimal adjust A",
    "DEC": "Decrement memory",
    "DECA": "Decrement A",
    "DECB": "Decrement B",
    "DES": "Decrement stack pointer",
    "DEX": "Decrement X",
    "EORA": "Exclusive OR into A",
    "EORB": "Exclusive OR into B",
    "INC": "Increment memory",
    "INCA": "Increment A",
    "INCB": "Increment B",
    "INS": "Increment stack pointer",
    "INX": "Increment X",
    "JMP": "Jump",
    "JSR": "Jump to subroutine",
    "LDAA": "Load A",
    "LDAB": "Load B",
    "LDD": "Load D",
    "LDS": "Load stack pointer",
    "LDX": "Load X",
    "LSL": "Logical shift left memory (= ASL)",
    "LSLA": "Logical shift left A (= ASLA)",
    "LSLB": "Logical shift left B (= ASLB)",
    "LSLD": "Logical shift left D (= ASLD)",
    "LSR": "Logical shift right memory",
    "LSRA": "Logical shift right A",
    "LSRB": "Logical shift right B",
    "LSRD": "Logical shift right D",
    "MUL": "Multiply A by B into D (unsigned)",
    "NEG": "Negate memory",
    "NEGA": "Negate A",
    "NEGB": "Negate B",
    "NOP": "No operation",
    "ORAA": "OR into A",
    "ORAB": "OR into B",
    "PSHA": "Push A",
    "PSHB": "Push B",
    "PSHX": "Push X",
    "PULA": "Pull A",
    "PULB": "Pull B",
    "PULX": "Pull X",
    "ROL": "Rotate left memory",
    "ROLA": "Rotate left A",
    "ROLB": "Rotate left B",
    "ROR": "Rotate right memory",
    "RORA": "Rotate right A",
    "RORB": "Rotate right B",
    "RTI": "Return from interrupt",
    "RTS": "Return from subroutine",
    "SBA": "Subtract B from A",
    "SBCA": "Subtract with carry from A",
    "SBCB": "Subtract with carry from B",
    "SEC": "Set carry",
    "SEI": "Set interrupt mask",
    "SEV": "Set overflow",
    "STAA": "Store A",
    "STAB": "Store B",
    "STD": "Store D",
    "STS": "Store stack pointer",
    "STX": "Store X",
    "SUBA": "Subtract from A",
    "SUBB": "Subtract from B",
    "SUBD": "Subtract from D",
    "SWI": "Software interrupt",
    "TAB": "Transfer A to B",
    "TAP": "Transfer A to condition codes",
    "TBA": "Transfer B to A",
    "TPA": "Transfer condition codes to A",
    "TST": "Test memory",
    "TSTA": "Test A",
    "TSTB": "Test B",
    "TSX": "Transfer stack pointer + 1 to X",
    "TXS": "Transfer X - 1 to stack pointer",
    "WAI": "Wait for interrupt",
}

# addressing mode -> operand used to try it
MODES = [
    ("INH", ""),
    ("IMM", "#1"),
    ("DIR", "<$80"),
    ("IDX", "0,x"),
    ("EXT", ">$1234"),
    ("REL", "*"),
]

LISTING_LINE = re.compile(r"^\d{5} [0-9A-F]{4}\s+([0-9A-F]{2})\s")


def mnemonics(keywords_c):
    """Return the hardware mnemonics from the ops[] table."""
    text = open(keywords_c).read()
    text = text[text.index("/* hardware ops."):]
    for m in re.finditer(r"\d+,((?:\s*'[A-Z0-9]',)+)\s*([A-Z]+),", text):
        yield "".join(re.findall(r"'(.)'", m.group(1)))


def opcode(as01, workdir, mnemonic, operand):
    """Assemble one instruction; return its opcode, or None if illegal."""
    src = os.path.join(workdir, "t.s")
    lst = os.path.join(workdir, "t.lst")
    with open(src, "w") as f:
        f.write("\t%s\t%s\n" % (mnemonic.lower(), operand))
    subprocess.run([as01, src, "-l", lst], stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL)
    lines = open(lst).read().splitlines()
    if any(line.startswith("*****") for line in lines):
        return None
    for line in lines:
        m = LISTING_LINE.match(line)
        if m:
            return m.group(1)
    return None


def main():
    keywords_c, as01 = sys.argv[1], os.path.abspath(sys.argv[2])
    names = sorted(set(mnemonics(keywords_c)))
    rows = []
    with tempfile.TemporaryDirectory() as workdir:
        for name in names:
            codes = []
            for mode, operand in MODES:
                code = opcode(as01, workdir, name, operand)
                codes.append(code)
            # "*" is also a valid extended address, so it only means
            # relative for instructions with no memory modes
            if any(codes[1:5]):
                codes[5] = None
            rows.append((name, codes))

    print("| Mnemonic | INH | IMM | DIR | IDX | EXT | REL | Operation |")
    print("|:---------|:---:|:---:|:---:|:---:|:---:|:---:|:----------|")
    for name, codes in rows:
        cells = ["%s" % c if c else "" for c in codes]
        print("| **%s** | %s | %s |" % (name, " | ".join(cells),
                                         DESCRIPTIONS.get(name, "")))


if __name__ == "__main__":
    main()
