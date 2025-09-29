#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "raylib/clay_renderer_raylib.c"
#include "raylib/raylib.h"

void HandleClayErrors(Clay_ErrorData errorData) {
  printf("%s", errorData.errorText.chars);
}

const float ScreenWidth = 1000.0f;
const float ScreenHeight = 500.0f;

int main(void) {
  // init raylib
  Clay_Raylib_Initialize((int)ScreenWidth, (int)ScreenHeight, "Success", 0);

  // init Clay
  uint64_t clayMemorySize = Clay_MinMemorySize();
  Clay_Arena memoryArena = {
      .memory = malloc(clayMemorySize),
      .capacity = clayMemorySize,
  };
  Clay_Dimensions dimensions = {
      .width = ScreenWidth,
      .height = ScreenHeight,
  };
  Clay_Initialize(memoryArena, dimensions,
                  (Clay_ErrorHandler){HandleClayErrors});

  // ...more init??

  // main application loop
  while (!WindowShouldClose()) {
    // logic code

    // render stuff
    BeginDrawing();
    ClearBackground(BLACK);
    // TODO: render the clay layout
    EndDrawing();
  }

  return 0;
}
