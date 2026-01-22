// Test: Global variables
int test_var_global = 100;
int test_var_global2 = 50;

void main() {
    int test_var_local = test_var_global + test_var_global2;
    print("global sum: ", test_var_local);

    test_var_global = 200;
    print(" modified: ", test_var_global);
}
