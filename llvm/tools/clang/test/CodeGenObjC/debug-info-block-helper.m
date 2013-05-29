// REQUIRES: x86-64-registered-target
// RUN: %clang_cc1 -emit-llvm -fblocks -g -triple x86_64-apple-darwin10 -fobjc-runtime=macosx-fragile-10.5 %s -o - | FileCheck %s
extern void foo(void(^)(void));

// CHECK: [ DW_TAG_subprogram ] {{.*}} [__destroy_helper_block_]

@interface NSObject {
  struct objc_object *isa;
}
@end

@interface A:NSObject @end
@implementation A
- (void) helper {
 int master = 0;
 __apple_block int m2 = 0;
 __apple_block int dbTransaction = 0;
 int (^x)(void) = ^(void) { (void) self; 
	(void) master; 
	(void) dbTransaction; 
	m2++;
	return m2;

	};
  master = x();
}
@end

void foo(void(^x)(void)) {}

