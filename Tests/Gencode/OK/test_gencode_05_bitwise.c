// Test: Bitwise operators
void main() {
    int test_var_1 = 12;
    int test_var_2 = 10;

    int test_var_band = test_var_1 & test_var_2;
    int test_var_bor = test_var_1 | test_var_2;
    int test_var_bxor = test_var_1 ^ test_var_2;
    int test_var_bnot = ~test_var_1;
    int test_var_sll = test_var_1 << 2;
    int test_var_sra = test_var_1 >> 1;

    print("band: ", test_var_band);
    print(" bor: ", test_var_bor);
    print(" bxor: ", test_var_bxor);
    print(" bnot: ", test_var_bnot);
    print(" sll: ", test_var_sll);
    print(" sra: ", test_var_sra);
}
