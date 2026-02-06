// Test: Division by zero in expression (runtime error)
void main() {
    int a = 10;
    int b = 5;
    int c = a / (b - 5);
    print("should not reach: ", c);
}
