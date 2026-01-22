// Test: Division by zero (runtime error)
void main() {
    int test_var_1 = 10;
    int test_var_2 = 0;
    int test_var_3 = test_var_1 / test_var_2;
    print("should not reach: ", test_var_3);
}
