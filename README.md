[![Linux Build Status](https://travis-ci.org/gtcasl/cash.png?branch=master)](https://travis-ci.org/gtcasl/cash) 
[![codecov.io](http://codecov.io/github/gtcasl/cash/coverage.svg?branch=master)](http://codecov.io/github/gtcasl/cash?branch=master)

# Cash

A C++ EDSL for Hardware Design and Simulation.

https://gtcasl.github.io/cash/

(c) 2016-2018 Blaise Tine

Release under the BSD license, see LICENSE for details.

Dependencies
------------
Tested compilers: **G++6, Clang++ 3.8.0**
Cash requires C++17 compiler to build with support for inline variables. 
It has been tested with gcc 7 and clang 5. 
Other dependencies include:
  - [Backward](https://github.com/bombela/backward-cpp)
  - [Catch](https://github.com/catchorg/Catch2)

Installation
------------
To install Cash you must clone the repository and create a build directory:

    $ git clone https://github.com/gtcasl/cash.git && cd cash
    $ mkdir build && cd build

Then use run cmake to generate the makefile and export the package informations:

    $ cmake ..

Optionally, you can also install Cash on your system:

    # make install
    
That's all!

Using Cash library
------------------
You must link the following library from the install location to your project

    libcash.so

Then you can include the header as follow:

    #include <cash.hpp>

Example
-------
A parametrizable binary adder.

```C++
#include <cash.h>

using namespace ch::core;
using namespace ch::sim;

// hardware description
template <unsigned N>
struct Adder {
  __io (
    __in(ch_uint<N>)  lhs,
    __in(ch_uint<N>)  rhs,
    __out(ch_uint<N>) out
  );

  void describe() {
    io.out = io.lhs + io.rhs;
  }
};

int main() {
  // instantiate a 4-bit adder
  ch_device<Adder<4>> my_adder;

  // assign input values
  my_adder.io.lhs = 1;
  my_adder.io.rhs = 3;

  // run simulation
  ch_simulator sim(adder);
  sim.run();
  
  // get result
  std::cout << "result = "  << my_adder.io.out << std::endl;

  return 0;
}
```
Contributing
------------
Contributions are welcome, you can email me at blaise.tine@gmail.com.
