// Test: Nested blocks
void main() {
    int test_var_1 = 10;
    {
        int test_var_2 = 20;
        {
            int test_var_3 = test_var_1 + test_var_2;
            print("nested: ", test_var_3);
        }
    }
    print(" outer: ", test_var_1);
}
