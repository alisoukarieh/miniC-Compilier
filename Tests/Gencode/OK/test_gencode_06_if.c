// Test: If statement
void main() {
    int x = 10;
    int r = 0;

    if (x > 5) {
        r = 1;
    }
    print("if result: ", r);

    if (x < 5) {
        r = 99;
    }
    print(" unchanged: ", r);
}
