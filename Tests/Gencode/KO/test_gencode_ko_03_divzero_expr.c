// Test: Division by zero in expression (runtime error)
void main() {
    int test_var_1 = 10;
    int test_var_2 = 5;
    int test_var_3 = test_var_1 / (test_var_2 - 5);
    print("should not reach: ", test_var_3);
}
