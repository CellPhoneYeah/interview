#include "serialize.h"
#include <iostream>
#include "test.h"
void TestBasic(){
    SerializeOS sos;
    char c = 'c';
    unsigned char uc= 2;
    short int si = 3;
    unsigned short int usi = 4;
    unsigned int ui = 5;
    int i = 6;
    long int li = 7;
    unsigned long int uli = 8;
    float f = 9;
    long long int lli = 10;
    unsigned long long int ulli = 11;
    double d = 12;
    sos << c << uc << si << usi << ui << i << li << uli << f << lli << ulli << d;
    SerializeIS sis(sos.getStr());
    char c1;
    unsigned char uc1;
    short int si1;
    unsigned short int usi1;
    unsigned int ui1;
    int i1;
    long int li1;
    unsigned long int uli1;
    float f1;
    long long int lli1;
    unsigned long long int ulli1;
    double d1;
    sis >> c1 >> uc1 >> si1 >> usi1 >> ui1 >> i1 >> li1 >> uli1 >> f1 >> lli1 >> ulli1 >> d1;
    OPTION_EQ(c, c1);
    OPTION_EQ(uc, uc1);
    OPTION_EQ(si, si1);
    OPTION_EQ(usi, usi1);
    OPTION_EQ(ui, ui1);
    OPTION_EQ(i, i1);
    OPTION_EQ(li, li1);
    OPTION_EQ(uli, uli1);
    OPTION_EQ(f, f1);
    OPTION_EQ(lli, lli1);
    OPTION_EQ(ulli, ulli1);
    OPTION_EQ(d, d1);
    std::cout << "test basic success!" << std::endl;
}

int main(){
    TestBasic();
    return 0;
}