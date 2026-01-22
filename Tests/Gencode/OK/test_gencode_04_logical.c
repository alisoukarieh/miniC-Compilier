// Test: Logical operators
void main() {
    bool test_var_1 = true;
    bool test_var_2 = false;

    bool test_var_and = test_var_1 && test_var_1;
    bool test_var_or = test_var_1 || test_var_2;
    bool test_var_not = !test_var_2;
    bool test_var_and_f = test_var_1 && test_var_2;
    bool test_var_or_f = test_var_2 || test_var_2;

    print("and: ", test_var_and);
    print(" or: ", test_var_or);
    print(" not: ", test_var_not);
    print(" and_f: ", test_var_and_f);
    print(" or_f: ", test_var_or_f);
}
