// Test: Bitwise operators
void main() {
    int a = 12;
    int b = 10;

    int band = a & b;
    int bor = a | b;
    int bxor = a ^ b;
    int bnot = ~a;
    int sll = a << 2;
    int sra = a >> 1;

    print("band: ", band);
    print(" bor: ", bor);
    print(" bxor: ", bxor);
    print(" bnot: ", bnot);
    print(" sll: ", sll);
    print(" sra: ", sra);
}
