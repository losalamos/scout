// RUN: %clang_cc1 -fms-extensions -rewrite-objc -fobjc-runtime=macosx-fragile-10.5 -x objective-c++ -fblocks -o - %s
// REQUIRES: scoutdisable

extern "C" __declspec(dllexport) void BreakTheRewriter(void) {
        __apple_block int aBlockVariable = 0;
        void (^aBlock)(void) = ^ {
                aBlockVariable = 42;
        };
        aBlockVariable++;
        void (^bBlocks)(void) = ^ {
                aBlockVariable = 43;
        };
        void (^c)(void) = ^ {
                aBlockVariable = 44;
        };

}
__declspec(dllexport) extern "C" void AnotherBreakTheRewriter(int *p1, double d) {

        __apple_block int bBlockVariable = 0;
        void (^aBlock)(void) = ^ {
                bBlockVariable = 42;
        };
        bBlockVariable++;
        void (^bBlocks)(void) = ^ {
                bBlockVariable = 43;
        };
        void (^c)(void) = ^ {
                bBlockVariable = 44;
        };

}
