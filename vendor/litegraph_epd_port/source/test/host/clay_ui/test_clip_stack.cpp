#include <cassert>

#include "epd2_9/clay_ui/render/ClipStack.h"

using epd2_9::ClipStack;

void test_clip_stack_empty_visibility() {
  ClipStack stack;
  stack.reset();
  assert(stack.current() == nullptr);
  assert(stack.isVisible(0, 0, 10, 10));
  int16_t x = 0;
  int16_t y = 0;
  int16_t w = 10;
  int16_t h = 10;
  assert(stack.clipRectangle(x, y, w, h));
  assert(x == 0 && y == 0 && w == 10 && h == 10);
}

void test_clip_stack_push_and_intersection() {
  ClipStack stack;
  stack.reset();
  stack.push(0, 0, 100, 100);
  const auto* first = stack.current();
  assert(first && first->active);
  assert(stack.isVisible(10, 10, 20, 20));
  stack.push(20, 20, 40, 40);
  const auto* second = stack.current();
  assert(second && second->active);
  assert(second->x == 20 && second->y == 20 && second->w == 40 && second->h == 40);
  assert(stack.isVisible(30, 30, 5, 5));
  assert(!stack.isVisible(10, 10, 5, 5));
}

void test_clip_stack_clip_rectangle() {
  ClipStack stack;
  stack.reset();
  stack.push(10, 10, 20, 20);
  int16_t x = 5, y = 5, w = 20, h = 20;
  bool clipped = stack.clipRectangle(x, y, w, h);
  assert(clipped);
  assert(x == 10 && y == 10 && w == 15 && h == 15);
  x = 0; y = 0; w = 5; h = 5;
  clipped = stack.clipRectangle(x, y, w, h);
  assert(!clipped);
}

void test_clip_stack_depth_limit() {
  ClipStack stack;
  stack.reset();
  for (int i = 0; i < 16; ++i) stack.push(i, i, 100 - i, 100 - i);
  const auto* top = stack.current();
  assert(top);
  assert(top->x >= 0);
  assert(top->w >= 0);
  assert(stack.isVisible(top->x, top->y, top->w, top->h));
}

void run_clip_stack_tests() {
  test_clip_stack_empty_visibility();
  test_clip_stack_push_and_intersection();
  test_clip_stack_clip_rectangle();
  test_clip_stack_depth_limit();
}
