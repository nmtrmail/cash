#include "common.h"

namespace {

__struct(X, (
  (ch_bit2) q,
  (ch_bit2) p
));

__union(U, (
  (ch_bit1) q,
  (ch_bit2) p
));

using V2 = ch_vec<ch_bit2, 2>;

__enum(E, 2, (a, b, c, d));

}

TEST_CASE("registers", "[registers]") {
  SECTION("delay", "[delay]") {
    TEST([]()->ch_bool {
      ch_bit2 a;
      a = ch_delay(11_b);
      ch_bit2 e = ch_case(ch_now(), 3, 11_b)(a);
      //ch_print("t={0}, a={1}, e={2}", ch_now(), a, e);
      return (a == e);
    }, 1);

    TEST([]()->ch_bool {
      ch_bit2 a;
      a = ch_delay(10_b, 4);
      ch_bit2 e = ch_case(ch_now(), 7, 10_b)(a);
      // ch_print("t={0}, a={1}, e={2}", ch_now(), a, e);
      return (a == e);
    }, 4);

    TEST([]()->ch_bool {
      ch_bit2 a;
      a = ch_delay(10_b, 4, 1);
      ch_bit2 e = ch_case(ch_now(), 9, 10_b)(1);
      //ch_print("t={0}, a={1}, e={2}", ch_now(), a, e);
      return (a == e);
    }, 4);

    TEST([]()->ch_bool {
      auto a = ch_delay(V2{3, 1});
      auto e = ch_case(ch_now(), 3, 1101_b)(a.as_uint());
      //ch_print("t={0}, a={1}, e={2}", ch_now(), a, e);
      return (a.as_bit() == e);
    }, 1);

    TEST([]()->ch_bool {
      auto a = ch_delay(X{3, 1});
      auto e = ch_case(ch_now(), 3, 1101_b)(a.as_uint());
      //ch_print("t={0}, a={1}, e={2}", ch_now(), a, e);
      return (a.as_bit() == e);
    }, 1);

    TEST([]()->ch_bool {
      auto a = ch_delay(U{10_b});
      auto e = ch_case(ch_now(), 3, 10_b)(a.as_uint());
      //ch_print("t={0}, a={1}, e={2}", ch_now(), a, e);
      return (a.as_bit() == e);
    }, 1);

    TEST([]()->ch_bool {
      auto a = ch_delay<E>(E::c);
      auto e = ch_case<E>(ch_now(), 3, E::c)(a);
      //ch_print("t={0}, a={1}, e={2}", ch_now(), a, e);
      return (a == e);
    }, 1);

    TEST([]()->ch_bool {
      ch_uint2 a;
      a = ch_delay(ch_sel(ch_reset(), 1, a + 1));
      ch_bit2 e = ch_case(ch_now(), 3, 10_b)(5, 11_b)(7, 00_b)(9, 01_b)(a);
      //ch_print("t={0}, a={1}, e={2}", ch_now(), a, e);
      return (a == e);
    }, 4);

    TEST([]()->ch_bool {
      auto x = ch_delay<E>(E::c);
      auto e = ch_case<E>(ch_now(), 3, E::c)(x);
      return (x == e);
    }, 1);
  }

  SECTION("reg", "[reg]") {
    TEST([]()->ch_bool {
      ch_reg<ch_uint2> a(0);
      a->next = a + 1;
      auto e = ch_case(ch_now(), 3, 01_b)(5, 10_b)(7, 11_b)(9, 00_b)(a);
      //ch_print("t={0}, a={1}, e={2}", ch_now(), a, e);
      return (e == a);
    }, 4);

    TEST([]()->ch_bool {
      ch_reg<V2> a, b(0), c(0000_b);
      a->next = V2{3, 1};
      auto e = ch_case(ch_now(), 3, 1101_b)(a.as_uint());
      //ch_print("t={0}, a={1}, e={2}", ch_now(), a, e);
      return (a.as_bit() == e);
    }, 1);

    TEST([]()->ch_bool {
      ch_reg<X> a, b{00_b, 00_b};
      a->next = X{3, 1};
      auto e = ch_case(ch_now(), 3, 1101_b)(a.as_uint());
      //ch_print("t={0}, a={1}, e={2}", ch_now(), a, e);
      return (a.as_bit() == e);
    }, 1);

    TEST([]()->ch_bool {
      ch_reg<U> a{00_b}, b{1_b};
      a->next = U{10_b};
      auto e = ch_case(ch_now(), 3, 10_b)(a.as_uint());
      //ch_print("t={0}, a={1}, e={2}", ch_now(), a, e);
      return (a.as_bit() == e);
    }, 1);

    TEST([]()->ch_bool {
      ch_reg<E> a;
      a->next = E::c;
      auto e = ch_case<E>(ch_now(), 3, E::c)(a);
      //ch_print("t={0}, a={1}, e={2}", ch_now(), a, e);
      return (a == e);
    }, 1);

    TEST([]()->ch_bool {
      ch_reg<ch_uint2> a(1);
      a->next = a + 1;
      auto e = ch_case(ch_now(), 3, 10_b)(5, 11_b)(7, 00_b)(9, 01_b)(a);
      //ch_print("t={0}, a={1}", ch_now(), a);
      return (e == a);
    }, 4);

    TEST([]()->ch_bool {
      ch_reg<ch_int4> a(0), b(0);
      ch_int4 c = a + 1;
      ch_int4 d = c - 1;
      a->next = a + c;
      b->next = b + d;
      auto x = a + b + c + d;
      auto e = ch_case<ch_int4>(ch_now(), 9, 0x9)(7, 0xa)(5, 0xb)(3, 0x4)(1);
      //ch_print("t={0}, x={1}, e={2}", ch_now(), x, e);
      return (x == e);
    }, 4);

    TEST([]()->ch_bool {
      ch_reg<ch_bit2> a;
      auto x = ch_case(ch_now(), 8, 11_b)(6, 0)(4, 2)(2, 1)(0);
      a->next = x;
      auto e = ch_delay(x);
      //ch_print("t={0}, clk={1}, x={2}, a={3}, e={4}", ch_now(), ch_clock(), x, a, e);
      return (a == e);
    }, 4);

    TEST([]()->ch_bool {
      ch_reg<ch_bit2> a, e;
      auto b = ch_case(ch_now(), 9, 11_b)(7, 0)(5, 2)(3, 1)(0);
      e->next = ch_case(ch_now(), 9, 11_b)(7, 1)(5, 0)(3, 2)(1, 1)(0);

      __if (b == 1) {
        a->next = 2;
      } __elif (b == 0) {
         a->next = 1;
      } __elif (b == 2) {
        a->next = 0;
      } __else {
        a->next = 3;
      };

      //ch_print("t={0}, clk={1}, a={2}, b={3}, e={4}", ch_now(), ch_clock(), a, b, e);
      return (a == e);
    }, 4);

    TEST([]()->ch_bool {
      ch_reg<ch_bit2> a, e;
      auto b = ch_case(ch_now(), 11, 11_b)(9, 3)(7, 0)(5, 2)(3, 1)(0);
      e->next = ch_case(ch_now(), 11, 01_b)(9, 1)(7, 1)(5, 0)(3, 2)(1, 1)(0);

      __if (b == 1) {
        a->next = 2;
      } __elif (b == 0) {
         a->next = 1;
      } __elif (b == 2) {
        a->next = 0;
      } __else {
        a->next = a;
      };

      //ch_print("t={0}, clk={1}, a={2}, b={3}, e={4}", ch_now(), ch_clock(), a, b, e);
      return (a == e);
    }, 5);

    TEST([]()->ch_bool {
      ch_reg<ch_bit2> a, e;
      auto b = ch_case(ch_now(), 9, 11_b)(7, 0)(5, 2)(3, 1)(0);
      e->next = ch_case(ch_now(), 9, 01_b)(7, 1)(5, 0)(3, 2)(1, 1)(0);

      __switch (b)
      __case (1) { a->next = 2; }
      __case (0) { a->next = 1; }
      __case (2) { a->next = 0; }
      __default  { a->next = a; };

      //ch_print("t={0}, clk={1}, a={2}, b={3}, e={4}", ch_now(), ch_clock(), a, b, e);
      return (a == e);
    }, 4);


    TEST([]()->ch_bool {
      ch_reg<ch_bit2> a, e;
      auto b = ch_case(ch_now(), 9, 11_b)(7, 0)(5, 2)(3, 1)(0);
      auto v = ch_case(ch_now(), 9, 11_b)(7, 1)(5, 0)(3, 2)(0);
      e->next = v.slice<2>();

      __if (b == 1) {
        a->next = v.slice<2>();
      } __elif (b == 0) {
         a->next = v.slice<2>();
      } __elif (b == 2) {
        a->next = 0;
      } __else {
        a->next = 3;
      };

      //ch_print("t={0}, clk={1}, a={2}, b={3}, e={4}", ch_now(), ch_clock(), a, b, e);
      return (a == e);
    }, 4);

    TEST([]()->ch_bool {
      std::array<ch_reg<ch_bit2>, 2> x;
      ch_bit2 a;
      ch_reg<ch_bit2> e;

      x[0]->next = a;
      x[1]->next = ~a;

      a = ch_case(ch_now(), 9, 00_b)(7, 2)(5, 3)(3, 1)(0);
      e->next = a;

      //ch_print("t={0}, clk={1}, x0={2}, x1={3}, e={4}", ch_now(), ch_clock(), x[0], x[1], e);
      return (x[0] == e) && (x[1] == ~e);
    }, 4);

    TEST([]()->ch_bool {
      ch_reg<X> x;
      ch_reg<ch_bit2> e;

      auto a = ch_case(ch_now(), 8, 00_b)(6, 2)(4, 3)(2, 1)(0);

      x->next = X{~a, a};
      e->next = a;

      //ch_print("t={0}, clk={1}, x.p={2}, x.q={3}, e={4}", ch_now(), ch_clock(), x.p, x.q, e);
      return (x.q == e) && (x.p == ~e);
    }, 4);

    TEST([]()->ch_bool {
      auto clk   = ch_case(ch_now(), 8, 1_b)(6, 1)(4, 1)(2, 1)(0);
      auto reset = ch_case(ch_now(), 6, 1_b)(0);
      auto next  = ch_case(ch_now(), 8, 11_b)(7, 3)(6, 3)(5, 3)(4, 2)(3, 3)(2, 1)(1, 3)(0);

      ch_pushcd(clk, reset);

      auto r = ch_delay(ch_sel(ch_reset(), 0, next));
      auto e = ch_case(ch_now(), 8, 11_b)(7, 0)(6, 0)(5, 2)(4, 2)(3, 1)(2, 1)(r);

      //ch_print("t={0}, clk={1}, rst={2}, next={3}, out={4}, expected={5}", ch_now(), clk, reset, next, r, e);

      ch_popcd();

      return (r == e);
    }, 4);
  }
}
