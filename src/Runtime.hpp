#pragma once

extern "C" void print_i32(long i) { std::cout << i; }

extern "C" void print_bool(char i) { std::cout << (i ? "True" : "False"); }

extern "C" void print_float(float i) { std::cout << i; }

extern "C" void print_char(char i) { std::cout << i; }

extern "C" void print_and_free(long *ptr) {
  std::cout << "freeing:" << ptr << std::endl;
  free(ptr);
  std::cout << "freed" << std::endl;
}

extern "C" long *print_and_malloc(long size) {
  std::cout << "mallocing" << std::endl;
  auto p = (long *)malloc(size);
  std::cout << "malloced:" << p << std::endl;
  return p;
}
