// RUN: %clang_cc1 -fsyntax-only -verify -Wno-c++11-extensions %s

struct A {};
struct B {};
struct D {
  A fizbin;  // expected-note 2 {{declared here}}
  A foobar;  // expected-note 2 {{declared here}}
  B roxbin;  // expected-note 2 {{declared here}}
  B toobad;  // expected-note 2 {{declared here}}
  void BooHoo();
  void FoxBox();
};

void something(A, B);
void test() {
  D obj;
  something(obj.fixbin,   // expected-error {{did you mean 'fizbin'?}}
            obj.toobat);  // expected-error {{did you mean 'toobad'?}}
  something(obj.toobat,   // expected-error {{did you mean 'foobar'?}}
            obj.fixbin);  // expected-error {{did you mean 'roxbin'?}}
  something(obj.fixbin,   // expected-error {{did you mean 'fizbin'?}}
            obj.fixbin);  // expected-error {{did you mean 'roxbin'?}}
  something(obj.toobat,   // expected-error {{did you mean 'foobar'?}}
            obj.toobat);  // expected-error {{did you mean 'toobad'?}}
  // Both members could be corrected to methods, but that isn't valid.
  something(obj.boohoo,   // expected-error-re {{no member named 'boohoo' in 'D'{{$}}}}
            obj.foxbox);  // expected-error-re {{no member named 'foxbox' in 'D'{{$}}}}
  // The first argument has a usable correction but the second doesn't.
  something(obj.boobar,   // expected-error-re {{no member named 'boobar' in 'D'{{$}}}}
            obj.foxbox);  // expected-error-re {{no member named 'foxbox' in 'D'{{$}}}}
}

// Ensure the delayed typo correction does the right thing when trying to
// recover using a seemingly-valid correction for which a valid expression to
// replace the TypoExpr cannot be created (but which does have a second
// correction candidate that would be a valid and usable correction).
class Foo {
public:
  template <> void testIt();  // expected-error {{no function template matches}}
  void textIt();  // expected-note {{'textIt' declared here}}
};
void testMemberExpr(Foo *f) {
  f->TestIt();  // expected-error {{no member named 'TestIt' in 'Foo'; did you mean 'textIt'?}}
}

void callee(double, double);
void testNoCandidates() {
  callee(xxxxxx,   // expected-error-re {{use of undeclared identifier 'xxxxxx'{{$}}}}
         zzzzzz);  // expected-error-re {{use of undeclared identifier 'zzzzzz'{{$}}}}
}

class string {};
struct Item {
  void Nest();
  string text();
  Item* next();  // expected-note {{'next' declared here}}
};
void testExprFilter(Item *i) {
  Item *j;
  j = i->Next();  // expected-error {{no member named 'Next' in 'Item'; did you mean 'next'?}}
}

// Test that initializer expressions are handled correctly and that the type
// being initialized is taken into account when choosing a correction.
namespace initializerCorrections {
struct Node {
  string text() const;
  // Node* Next() is not implemented yet
};
void f(Node *node) {
  // text is only an edit distance of 1 from Next, but would trigger type
  // conversion errors if used in this initialization expression.
  Node *next = node->Next();  // expected-error-re {{no member named 'Next' in 'initializerCorrections::Node'{{$}}}}
}

struct LinkedNode {
  LinkedNode* next();  // expected-note {{'next' declared here}}
  string text() const;
};
void f(LinkedNode *node) {
  // text and next are equidistant from Next, but only one results in a valid
  // initialization expression.
  LinkedNode *next = node->Next();  // expected-error {{no member named 'Next' in 'initializerCorrections::LinkedNode'; did you mean 'next'?}}
}

struct NestedNode {
  NestedNode* Nest();
  NestedNode* next();
  string text() const;
};
void f(NestedNode *node) {
  // There are two equidistant, usable corrections for Next: next and Nest
  NestedNode *next = node->Next();  // expected-error-re {{no member named 'Next' in 'initializerCorrections::NestedNode'{{$}}}}
}
}
