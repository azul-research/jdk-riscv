def compare_tos(tos, ref):
    if tos == ref:
        return True

    if abs(tos - ref) < 100:
        # too close to be coincidence
        return False

    return True


def parse_trace_line(line):
    if not line.startswith("["):
        return None
    sp = line.split()
    try:
        count = int(sp[1])
        idx = int(sp[2])
    except:
        return None

    tos = int(sp[3], 16) & (2 ** 32 - 1)
    bytecode = sp[5].replace("fast_", "")
    if "[" in bytecode:
        bytecode = bytecode[:bytecode.find("[")]
    if bytecode in ["linearswitch"]:
        bytecode = "lookupswitch"

    return count, idx, bytecode, tos

line = 0

with open("trace_riscv", "r") as riscv_f:
    with open("trace_zero", "r") as zero_f:
        while True:
            line += 1
            riscv_line = riscv_f.readline()
            zero_line = zero_f.readline()
            if not zero_line or not riscv_line:
                break
            a = parse_trace_line(riscv_line)
            b = parse_trace_line(zero_line)

            if a is None and b is not None:
                print("Mismatch at line", line)
                break

            if a is not None and b is None:
                print("Mismatch at line", line)
                break

            if a is None and b is None:
                print("Skip line", line)
                continue

            if a[2] == "iload2":
                zero_f.readline()
            elif a[1:3] != b[1:3] or not compare_tos(a[3], b[3]):
                print("Mismatch at line", line)
                print(a)
                print(b)
                input()

