// Test: Global variables
int g1 = 100;
int g2 = 50;

void main() {
    int loc = g1 + g2;
    print("global sum: ", loc);

    g1 = 200;
    print(" modified: ", g1);
}
