// Test: Nested blocks
void main() {
    int a = 10;
    {
        int b = 20;
        {
            int c = a + b;
            print("nested: ", c);
        }
    }
    print(" outer: ", a);
}
