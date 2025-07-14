#import <Foundation/Foundation.h>
#import <stdio.h>

// Simple test function to isolate the issue
extern "C" void TestParameterPassing(const char* testParam) {
    printf("TestParameterPassing received pointer: %p\n", testParam);
    if (testParam) {
        printf("TestParameterPassing received string: %s\n", testParam);
        printf("TestParameterPassing strlen: %zu\n", strlen(testParam));
    } else {
        printf("TestParameterPassing received NULL!\n");
    }
}

// Test Objective-C++ calling into C function
void RunParameterTest() {
    const char* testString = "environment/Level1BackLayerBackground.png";
    printf("Caller has pointer: %p\n", testString);
    printf("Caller has string: %s\n", testString);
    printf("Caller strlen: %zu\n", strlen(testString));
    
    printf("About to call TestParameterPassing...\n");
    TestParameterPassing(testString);
    printf("Call completed.\n");
}

int main() {
    RunParameterTest();
    return 0;
}
