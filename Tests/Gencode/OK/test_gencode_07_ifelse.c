// Test: If-else statement
void main() {
    int test_var_1 = 10;
    int test_var_2 = 0;

    if (test_var_1 > 20) {
        test_var_2 = 1;
    } else {
        test_var_2 = 2;
    }
    print("else taken: ", test_var_2);

    if (test_var_1 < 20) {
        test_var_2 = 3;
    } else {
        test_var_2 = 4;
    }
    print(" if taken: ", test_var_2);
}
