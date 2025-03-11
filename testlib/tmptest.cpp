#include <iostream>
#include <thread>
#include <vector>
int number;
bool ready = false;
void writer() {
number = 42;
ready = true;
}
void reader() {
while (!ready) {}
if(number != 42)
    std::cout << "Number: " << number << std::endl;
}
int main() {
std::thread t1(writer);

std::vector<std::thread> v;
for (int i = 0; i < 20; i++)
{
    v.emplace_back(reader);
}

t1.join();
for (auto &&i : v)
{
    i.join();
}

return 0;
}