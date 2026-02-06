// Test: Modulo by zero (runtime error)
void main() {
    int a = 10;
    int b = 0;
    int c = a % b;
    print("should not reach: ", c);
}
