// Test: Comparison operators
void main() {
    int a = 5;
    int b = 10;
    int c = 5;

    bool lt = a < b;
    bool gt = b > a;
    bool le = a <= c;
    bool ge = a >= c;
    bool eq = a == c;
    bool ne = a != b;

    print("lt: ", lt);
    print(" gt: ", gt);
    print(" le: ", le);
    print(" ge: ", ge);
    print(" eq: ", eq);
    print(" ne: ", ne);
}
