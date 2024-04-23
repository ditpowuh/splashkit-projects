#include "splashkit.h"
#include <cmath>

const int SCREEN_SIZE[] = {1280, 720};
const int TARGET_FRAMERATE = 60;

double player_position[] = {SCREEN_SIZE[0] / 2, SCREEN_SIZE[1] / 2};
double player_speed = 5;

const double diagonalMovement = sqrt(2) / 2;
double applied_movement[] = {0, 0};

void process_player_movement() {
  applied_movement[0] = 0;
  applied_movement[1] = 0;

  if (key_down(D_KEY)) {
    applied_movement[0] = 1;
  }
  if (key_down(A_KEY)) {
    applied_movement[0] = -1;
  }
  if (key_down(W_KEY)) {
    applied_movement[1] = 1;
  }
  if (key_down(S_KEY)) {
    applied_movement[1] = -1;
  }
  if (applied_movement[0] != 0 && applied_movement[1] != 0) {
    applied_movement[0] = applied_movement[0] * diagonalMovement;
    applied_movement[1] = applied_movement[1] * diagonalMovement;
  }
  player_position[0] = player_position[0] + player_speed * applied_movement[0];
  player_position[1] = player_position[1] + player_speed * applied_movement[1];
}

int main() {
  bool game_running = true;

  open_window("Game", SCREEN_SIZE[0], SCREEN_SIZE[1]);

  while (game_running && !quit_requested()) {
    if (key_typed(ESCAPE_KEY)) {
      game_running = false;
    }
    process_player_movement();

    clear_screen(color_white());
    fill_circle(color_black(), player_position[0], SCREEN_SIZE[1] - player_position[1], 25);

    refresh_screen(TARGET_FRAMERATE);
    process_events();
  }

  return 0;
}
