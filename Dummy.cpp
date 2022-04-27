#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <vector>

class Dummy{

    public:
        void init(){
            //do threads here

            std::vector<std::thread> threads;
            for (int i=0; i<5; i++){
                threads.push_back(std::thread(&Dummy::f, this, i));
                // printf("here");
            }

            for (auto& th : threads) th.join();

            printf("done");
        }

    void f(int i){
        printf("%d\n", i);
    }

};

int main(int argc, char** argv){
    printf("let's begin:");
    Dummy dummy = Dummy();
    dummy.init();
}

// #include <iostream>       // std::cout
// #include <thread>         // std::thread
 
// void foo() 
// {
//   // do stuff...
// }

// void bar(int x)
// {
//   // do stuff...
// }

// int main() 
// {
//   std::thread first (foo);     // spawn new thread that calls foo()
//   std::thread second (bar,0);  // spawn new thread that calls bar(0)

//   std::cout << "main, foo and bar now execute concurrently...\n";

//   // synchronize threads:
//   first.join();                // pauses until first finishes
//   second.join();               // pauses until second finishes

//   std::cout << "foo and bar completed.\n";

//   return 0;
// }