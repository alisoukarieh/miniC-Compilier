// Test: Logical operators
void main() {
    bool t = true;
    bool f = false;

    bool r1 = t && t;
    bool r2 = t || f;
    bool r3 = !f;
    bool r4 = t && f;
    bool r5 = f || f;

    print("and: ", r1);
    print(" or: ", r2);
    print(" not: ", r3);
    print(" and_f: ", r4);
    print(" or_f: ", r5);
}
