// Test: Comparison operators
void main() {
    int test_var_1 = 5;
    int test_var_2 = 10;
    int test_var_3 = 5;

    bool test_var_lt = test_var_1 < test_var_2;
    bool test_var_gt = test_var_2 > test_var_1;
    bool test_var_le = test_var_1 <= test_var_3;
    bool test_var_ge = test_var_1 >= test_var_3;
    bool test_var_eq = test_var_1 == test_var_3;
    bool test_var_ne = test_var_1 != test_var_2;

    print("lt: ", test_var_lt);
    print(" gt: ", test_var_gt);
    print(" le: ", test_var_le);
    print(" ge: ", test_var_ge);
    print(" eq: ", test_var_eq);
    print(" ne: ", test_var_ne);
}
