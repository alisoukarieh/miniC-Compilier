// Test: If-else statement
void main() {
    int x = 10;
    int r = 0;

    if (x > 20) {
        r = 1;
    } else {
        r = 2;
    }
    print("else taken: ", r);

    if (x < 20) {
        r = 3;
    } else {
        r = 4;
    }
    print(" if taken: ", r);
}
