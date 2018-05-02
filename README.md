# ArgumentViewer
Simple cpp library for command line argument parsing.

## Building
The library depends on two other libraries:
[MealyMachine](https://github.com/dormon/MealyMachine)
[TxtUtils](https://github.com/dormon/TxtUtils)
```
$ mkdir build
$ cd build
$ cmake-gui .. # or ccmake ..
$ #set MEALYMACHINE_DIR and TXTUTILS_DIR to paths to MealyMachineConfig.cmake and TxtUtilsConfig.cmake
$ make
```
You have to build and install these libraries first.

## How to use this library
```cpp

#include<memory>
#include<ArgumentViewer/ArgumentViewer.h>

int main(int argc,char*argv[]){
  
  auto args = std::make_shared<ArgumentViewer>(argc,argv);

  auto modelName  = args->gets("--model","2tri.3ds","model file name");

  auto windowSize = arg->getu32v("--window-size",{512,512},"window size" );

  auto lightPosition = arg->getf32v("--light",{0.f,1000.f,0.f,1.f},"light position");

  bool printHelp = arg->isPresent("-h","prints this help");

  printHelp = printHelp || !arg->validate();
  if(printHelp){
    std::cerr<<arg->toStr();
    exit(0);
  }

  //...
}

```

## Features
* Arguments in text file
* Context of arguments
* Automatic help / nice help output
