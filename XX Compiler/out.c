#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef int32_t i32;
typedef int64_t i64;
typedef float f32;
typedef double f64;
typedef void _;

i32 main();
_ main2(f64 val_i_need);

typedef struct Test { int value; } Test; i32 main() {
Test test = (Test) {30}; printf("%d\n", test.value); i32 val_test = 30;
main2(val_test); 
}
_ main2(f64 val_i_need) {
printf("%f\n", val_i_need + 3000000); i32 var_not_viable = 30;

}
