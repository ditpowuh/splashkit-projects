#include "splashkit.h"
#include <filesystem>
#include <cmath>
#include <list>
#include <map>

// Preprocessor Macro to replace stick_to_screen_x and stick_to_screen_y with to_world_x and to_world_y - this is made for personal use as it makes it easier for me to understand
#define stick_to_screen_x to_world_x
#define stick_to_screen_y to_world_y

using namespace std;

const int SCREEN_SIZE[] = {1280, 720};
const int TARGET_FRAMERATE = 60;

// Get the details of the primary display
const display DISPLAY_DETAILS = display_details(0);

// Mathematical constants for trigonometric calculations
const double PI = atan(1) * 4;
const double RADIANS_TO_DEGREES = 180 / PI;
const double DEGREES_TO_RADIANS = PI / 180;

window main_window;

double player_position[] = {(double)SCREEN_SIZE[0] / 2, (double)SCREEN_SIZE[1] / 2};
double player_speed = 5;
enum direction {
  UP,
  DOWN,
  LEFT,
  RIGHT
};
direction player_direction = DOWN;
int player_health = 100;
bool player_moving = false;

int player_dashing = 0;
double dashing_angle;

const double diagonalMovement = sqrt(2) / 2;
double applied_movement[] = {0, 0};

font main_font = load_font("Fibberish", "Graphics/Fibberish.ttf");
bitmap music_heart = load_bitmap("Music Heart", "Graphics/Music Heart.png");
bitmap mouse_cursor = load_bitmap("Mouse Cursor", "Graphics/Mouse Cursor.png");
bitmap crosshair = load_bitmap("Crosshair", "Graphics/Crosshair.png");

string player_animation = "IdleDown";
map<string, int> animation_map;

int bpm = 120;

class enemy {
  public:
    int hp;
    double position[2];
    enemy(int hp, double x, double y) {
      this->hp = hp;
      position[0] = x;
      position[1] = y;
    }
};

class particle {
  private:
    int lifespan;

  public:
    particle(int lifespan) {
      this->lifespan = lifespan;
    }
};

void process_player_movement(bool stop_movement) {
  applied_movement[0] = 0;
  applied_movement[1] = 0;

  if (!stop_movement) {
    if (key_down(W_KEY)) {
      applied_movement[1] = 1;
      player_direction = UP;
    }
    if (key_down(S_KEY)) {
      applied_movement[1] = -1;
      player_direction = DOWN;
    }
    if (key_down(D_KEY)) {
      applied_movement[0] = 1;
      player_direction = RIGHT;
    }
    if (key_down(A_KEY)) {
      applied_movement[0] = -1;
      player_direction = LEFT;
    }
  }

  if (applied_movement[0] != 0 || applied_movement[1] != 0) {
    player_moving = true;
    player_animation = "Run";
  }
  else {
    player_moving = false;
    player_animation = "Idle";
  }
  switch (player_direction) {
    case UP:
      player_animation = player_animation + "Up";
      break;
    case DOWN:
      player_animation = player_animation + "Down";
      break;
    case RIGHT:
      player_animation = player_animation + "Right";
      break;
    case LEFT:
      player_animation = player_animation + "Left";
      break;
  }

  if (stop_movement) {
    return;
  }
  if (applied_movement[0] != 0 && applied_movement[1] != 0) {
    applied_movement[0] = applied_movement[0] * diagonalMovement;
    applied_movement[1] = applied_movement[1] * diagonalMovement;
  }
  if (!(player_dashing > 0)) {
    player_position[0] = player_position[0] + player_speed * applied_movement[0];
    player_position[1] = player_position[1] + player_speed * applied_movement[1];
  }

}

void process_player_dash(bool stop_movement, bool in_time) {
  if (!stop_movement) {
    if (player_dashing > 0) {
      player_dashing = player_dashing - 1;
      player_position[0] = player_position[0] + cos(dashing_angle * DEGREES_TO_RADIANS) * 14;
      player_position[1] = player_position[1] + sin(dashing_angle * DEGREES_TO_RADIANS) * 14;
      return;
    }
    if (key_typed(SPACE_KEY) && in_time) {
      player_dashing = 12;

      int y_difference = stick_to_screen_y(SCREEN_SIZE[1] - mouse_y()) - (SCREEN_SIZE[1] - player_position[1] + 64);
      int x_difference = stick_to_screen_x(mouse_x()) - (player_position[0] + 32);
      dashing_angle = atan2(y_difference, x_difference) * RADIANS_TO_DEGREES;

      player_position[0] = player_position[0] + cos(dashing_angle * DEGREES_TO_RADIANS) * 14;
      player_position[1] = player_position[1] + sin(dashing_angle * DEGREES_TO_RADIANS) * 14;
    }
  }
}

void process_pause_menu(bool paused) {
  if (!paused) {
    return;
  }
  color background = rgba_color(0, 0, 0, 125);
  fill_rectangle(background, stick_to_screen_x(0), stick_to_screen_y(0), SCREEN_SIZE[0], SCREEN_SIZE[1]);
  draw_text_on_window(main_window, "Paused!", color_white(), main_font, 125, stick_to_screen_x(450), stick_to_screen_y(100));
  //fill_rectangle(color_)
}

void draw_cursor(bool paused) {
  if (!paused) {
    if (window_is_fullscreen(main_window)) {
      draw_bitmap(crosshair, stick_to_screen_x(mouse_x() * SCREEN_SIZE[0] / display_width(DISPLAY_DETAILS) - 32), stick_to_screen_y(mouse_y() * SCREEN_SIZE[1] / display_height(DISPLAY_DETAILS) - 32));
    }
    else {
      draw_bitmap(crosshair, stick_to_screen_x(mouse_x() - 32), stick_to_screen_y(mouse_y() - 32));
    }
  }
  else {
    if (window_is_fullscreen(main_window)) {
      draw_bitmap(mouse_cursor, stick_to_screen_x(mouse_x() * SCREEN_SIZE[0] / display_width(DISPLAY_DETAILS) - 8), stick_to_screen_y(mouse_y() * SCREEN_SIZE[1] / display_height(DISPLAY_DETAILS) - 8));
    }
    else {
      draw_bitmap(mouse_cursor, stick_to_screen_x(mouse_x() - 8), stick_to_screen_y(mouse_y() - 8));
    }
  }
}

bitmap draw_bitmap_with_animation(int elaspedFrames, string playing_animation, double position_x, double position_y, int animation_speed = 5) {
  int current_frame = elaspedFrames / animation_speed + 1;
  if (current_frame > animation_map[playing_animation]) {
    current_frame = current_frame - animation_map[playing_animation] * (elaspedFrames / (animation_speed * animation_map[playing_animation]));
  }
  draw_bitmap(playing_animation + to_string(current_frame), position_x, position_y);
  return bitmap_named(playing_animation + to_string(current_frame));
}

float bounce(int sin_value, float strength, int sharpness = 500) {
  return abs(pow(sin(PI * (float)sin_value / (bpm / 4) + PI / 2), sharpness) * strength);
}

int main() {
  bool game_running = true;
  bool game_pause = false;

  int elaspedFrames = 0;

  for (const auto &sub_directories : filesystem::directory_iterator("Graphics")) {
    string sub_directory = sub_directories.path().filename().string();
    if (sub_directory.find(".") != string::npos) {
      continue;
    }
    int file_count = 0;
    for (const auto &graphic_file : filesystem::directory_iterator("Graphics\\" + sub_directory)) {
      filesystem::path graphic_path = graphic_file.path();
      load_bitmap(sub_directory + graphic_path.filename().replace_extension().string(), graphic_path.string());
      //write_line(sub_directory);
      //write_line(file_path.filename().replace_extension().string());
      //write_line(file_path.string());
      file_count = file_count + 1;
    }
    animation_map[sub_directory] = file_count;
  }

  for (const auto &music_file : filesystem::directory_iterator("Music")) {
    filesystem::path music_path = music_file.path();
    load_music(music_path.filename().replace_extension().string(), music_path.string());
  }

  for (const auto &sound_file : filesystem::directory_iterator("Sounds")) {
    filesystem::path sound_path = sound_file.path();
    load_sound_effect(sound_path.filename().replace_extension().string(), sound_path.string());
  }

  main_window = open_window("Alterheart", SCREEN_SIZE[0], SCREEN_SIZE[1]);
  window_toggle_border(main_window);

  //hide_mouse();

  play_music("test");

  bitmap test = load_bitmap("test", "Graphics/test.png");

  list<enemy> enemies;

  while (game_running && !quit_requested()) {
    if (key_typed(ESCAPE_KEY)) {
      game_pause = !game_pause;
      if (game_pause) {
        pause_music();
        play_sound_effect("Pause");
      }
      else {
        resume_music();
      }

    }
    if (key_typed(F11_KEY)) {
      if (game_pause) {
        window_toggle_fullscreen(main_window);
      }
    }

    set_camera_x(player_position[0] - SCREEN_SIZE[0] / 2 + 32);
    set_camera_y((SCREEN_SIZE[1] - player_position[1]) - SCREEN_SIZE[1] / 2 + 64);

    process_player_movement(game_pause);
    process_player_dash(game_pause, bounce(elaspedFrames, 1, 1) > 0.5);
    //write_line(bounce(elaspedFrames, 1, 250));
    //process_player_attack(game_pause);

    clear_screen(color_white());
    draw_bitmap(test, 0, 0);
    bitmap player = draw_bitmap_with_animation(elaspedFrames, player_animation, player_position[0], SCREEN_SIZE[1] - player_position[1], 7);
    //fill_circle(color_dark_green(), player_position[0] + 32, SCREEN_SIZE[1] - player_position[1] + 64, 10);

    color hp_bar = rgba_color(255, 195, 195, 175 + (int)bounce(elaspedFrames, 20, 2));
    fill_rectangle(hp_bar, stick_to_screen_x(25), stick_to_screen_y(25), player_health * 4, 25);
    draw_bitmap(music_heart, stick_to_screen_x(5), stick_to_screen_y(50 - bounce(elaspedFrames, 3, 300)));

    process_pause_menu(game_pause);
    draw_cursor(game_pause);

    if (!game_pause) {
      elaspedFrames = elaspedFrames + 1;
    }

    if (key_typed(C_KEY)) {
      write_line(stick_to_screen_y(SCREEN_SIZE[1] - mouse_y()) - (SCREEN_SIZE[1] - player_position[1] - 64));
      write_line(stick_to_screen_x(mouse_x()) - (player_position[0] + 32));
    }

    refresh_screen(TARGET_FRAMERATE);
    process_events();
  }

  return 0;
}
