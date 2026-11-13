// Minimal test framework, no dependencies. Usage:
//
//   TEST(my_test){
//     CHECK(1 + 1 == 2);
//     CHECK_EQ(add(2, 2), 4);
//   }
#ifndef _TEST_FRAMEWORK_H_
#define _TEST_FRAMEWORK_H_

#include <cstdio>
#include <vector>

struct test_case{
  const char* name;
  void (*fn)();
};

inline std::vector<test_case>& test_registry(){
  static std::vector<test_case> tests;
  return tests;
}

inline int& test_failures(){
  static int failures = 0;
  return failures;
}

struct test_registrar{
  test_registrar(const char* name, void (*fn)()){
    test_registry().push_back({name, fn});
  }
};

#define TEST(name) \
  static void name(); \
  static test_registrar registrar_##name(#name, name); \
  static void name()

#define CHECK(cond) do{ \
    if(!(cond)){ \
      printf("    FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      test_failures()++; \
    } \
  } while(0)

#define CHECK_EQ(a, b) do{ \
    long long _a = (long long)(a); \
    long long _b = (long long)(b); \
    if(_a != _b){ \
      printf("    FAIL %s:%d: %s == %s (got %lld, expected %lld)\n", __FILE__, __LINE__, #a, #b, _a, _b); \
      test_failures()++; \
    } \
  } while(0)

#endif
