// Test: If statement
void main() {
    int test_var_1 = 10;
    int test_var_2 = 0;

    if (test_var_1 > 5) {
        test_var_2 = 1;
    }
    print("if result: ", test_var_2);

    if (test_var_1 < 5) {
        test_var_2 = 99;
    }
    print(" unchanged: ", test_var_2);
}
