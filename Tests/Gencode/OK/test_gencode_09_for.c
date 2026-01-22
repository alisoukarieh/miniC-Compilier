// Test: For loop
void main() {
    int test_var_1 = 0;
    int test_var_i = 0;

    for (test_var_i = 1; test_var_i <= 5; test_var_i = test_var_i + 1) {
        test_var_1 = test_var_1 + test_var_i;
    }
    print("sum 1-5: ", test_var_1);
}
