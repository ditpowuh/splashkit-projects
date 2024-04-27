// Using these libraries (Standard + SplashKit)
#include "splashkit.h"
#include <filesystem>
#include <cmath>
#include <list>
#include <map>

// Preprocessor Macro to replace stick_to_screen_x and stick_to_screen_y with to_world_x and to_world_y - this is made for personal use as it makes it easier for me to understand
#define stick_to_screen_x to_world_x
#define stick_to_screen_y to_world_y

// This is so that I don't have to use the scope resolution operator (::) everytime I use something from the std namespace
using namespace std;

// Sets important constants for the game
const int SCREEN_SIZE[] = {1280, 720};
const int TARGET_FRAMERATE = 60;

// Get the details of the primary display
const display DISPLAY_DETAILS = display_details(0);

// Mathematical constants for trigonometric calculations
const double PI = atan(1) * 4;
const double RADIANS_TO_DEGREES = 180 / PI;
const double DEGREES_TO_RADIANS = PI / 180;

// Declares the main window for the game
window main_window;

// Sets the position of the player as an array of doubles
double player_position[] = {0, 0};
// Sets the player speed as a double
double player_speed = 5;
// Creates an enumeration to represent direction
enum direction {
  UP,
  DOWN,
  LEFT,
  RIGHT
};
direction player_direction = DOWN;
int player_health = 100;
bool player_moving = false;

float dashing_angle;
float last_dashed;
int dash_amount = 0;

const double diagonalMovement = sqrt(2) / 2;
double applied_movement[] = {0, 0};

font main_font = load_font("Fibberish", "Graphics/Fibberish.ttf");

bitmap text_button1 = load_bitmap("Button1", "Graphics/Button.png");
bitmap text_button2 = load_bitmap("Button2", "Graphics/Button.png");
bitmap music_heart = load_bitmap("Music Heart", "Graphics/Music Heart.png");
bitmap mouse_cursor = load_bitmap("Mouse Cursor", "Graphics/Mouse Cursor.png");
bitmap crosshair = load_bitmap("Crosshair", "Graphics/Crosshair.png");
bitmap selected = load_bitmap("Selected Button", "Graphics/Selected Button.png");
bitmap fullscreen = load_bitmap("Fullscreen", "Graphics/Fullscreen.png");
bitmap windowed = load_bitmap("Windowed", "Graphics/Windowed.png");

string player_animation = "IdleDown";
map<string, int> animation_map;

int bpm = 120;

bool game_running = true;
bool game_pause = false;

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

float adjusted_mouse_x() {
  if (window_is_fullscreen(main_window)) {
    return mouse_x() * SCREEN_SIZE[0] / display_width(DISPLAY_DETAILS);
  }
  else {
    return mouse_x();
  }
}

float adjusted_mouse_y() {
  if (window_is_fullscreen(main_window)) {
    return mouse_y() * SCREEN_SIZE[1] / display_height(DISPLAY_DETAILS);
  }
  else {
    return mouse_y();
  }
}

bitmap draw_bitmap_with_animation(int elasped_frames, string playing_animation, double position_x, double position_y, int animation_speed = 5) {
  int current_frame = elasped_frames / animation_speed + 1;
  if (current_frame > animation_map[playing_animation]) {
    current_frame = current_frame - animation_map[playing_animation] * (elasped_frames / (animation_speed * animation_map[playing_animation]));
  }
  draw_bitmap(playing_animation + to_string(current_frame), position_x, position_y);
  return bitmap_named(playing_animation + to_string(current_frame));
}

float bounce(int sin_value, float strength, int sharpness = 500) {
  return abs(pow(sin(PI * (float)sin_value / (60 / (float)bpm * TARGET_FRAMERATE) + PI / 2), sharpness) * strength);
}

void process_player_movement() {
  applied_movement[0] = 0;
  applied_movement[1] = 0;

  if (!game_pause) {
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

  if (game_pause) {
    return;
  }
  if (applied_movement[0] != 0 && applied_movement[1] != 0) {
    applied_movement[0] = applied_movement[0] * diagonalMovement;
    applied_movement[1] = applied_movement[1] * diagonalMovement;
  }
  if (!(dash_amount > 0)) {
    player_position[0] = player_position[0] + player_speed * applied_movement[0];
    player_position[1] = player_position[1] + player_speed * applied_movement[1];
  }

}

void process_player_dash(int elasped_frames, float pointing_angle, bool in_time, int duration, int strength) {
  if (!game_pause) {
    if (dash_amount > 0) {
      dash_amount = dash_amount - 1;
      player_position[0] = player_position[0] + cos(dashing_angle * DEGREES_TO_RADIANS) * strength;
      player_position[1] = player_position[1] + sin(dashing_angle * DEGREES_TO_RADIANS) * strength;
      return;
    }
    if (key_typed(SPACE_KEY) && in_time) {
      dash_amount = duration;
      dashing_angle = pointing_angle;

      player_position[0] = player_position[0] + cos(dashing_angle * DEGREES_TO_RADIANS) * strength;
      player_position[1] = player_position[1] + sin(dashing_angle * DEGREES_TO_RADIANS) * strength;

      last_dashed = elasped_frames;
    }
  }
}

void process_pause_menu() {
  if (!game_pause) {
    return;
  }
  color background = rgba_color(0, 0, 0, 125);
  fill_rectangle(background, stick_to_screen_x(0), stick_to_screen_y(0), SCREEN_SIZE[0], SCREEN_SIZE[1]);
  draw_text_on_window(main_window, "Paused!", color_white(), main_font, 125, stick_to_screen_x(450), stick_to_screen_y(100));

  draw_bitmap(text_button1, stick_to_screen_x(570), stick_to_screen_y(225));
  draw_text_on_window(main_window, "Resume", color_white(), main_font, 35, stick_to_screen_x(585), stick_to_screen_y(245));

  draw_bitmap(text_button2, stick_to_screen_x(570), stick_to_screen_y(375));
  draw_text_on_window(main_window, "Quit", color_white(), main_font, 35, stick_to_screen_x(600), stick_to_screen_y(395));

  if (bitmap_collision(mouse_cursor, stick_to_screen_x(adjusted_mouse_x()), stick_to_screen_y(adjusted_mouse_y()), text_button1, stick_to_screen_x(585), stick_to_screen_y(245))) {
    draw_bitmap(selected, stick_to_screen_x(570 - 16), stick_to_screen_y(225 - 16));
    if (mouse_clicked(LEFT_BUTTON)) {
      game_pause = false;
    }
  }

  if (bitmap_collision(mouse_cursor, stick_to_screen_x(adjusted_mouse_x()), stick_to_screen_y(adjusted_mouse_y()), text_button2, stick_to_screen_x(570), stick_to_screen_y(375))) {
    draw_bitmap(selected, stick_to_screen_x(570 - 16), stick_to_screen_y(375 - 16));
    if (mouse_clicked(LEFT_BUTTON)) {
      game_running = false;
    }
  }

  if (window_is_fullscreen(main_window)) {
    draw_bitmap(windowed, stick_to_screen_x(SCREEN_SIZE[0] - 64), stick_to_screen_y(0));
    if (bitmap_collision(mouse_cursor, stick_to_screen_x(adjusted_mouse_x()), stick_to_screen_y(adjusted_mouse_y()), windowed, stick_to_screen_x(SCREEN_SIZE[0] - 64), stick_to_screen_y(0)) && mouse_clicked(LEFT_BUTTON)) {
      window_toggle_fullscreen(main_window);
      play_sound_effect("Toggle");
    }
  }
  else {
    draw_bitmap(fullscreen, stick_to_screen_x(SCREEN_SIZE[0] - 64), stick_to_screen_y(0));
    if (bitmap_collision(mouse_cursor, stick_to_screen_x(adjusted_mouse_x()), stick_to_screen_y(adjusted_mouse_y()), fullscreen, stick_to_screen_x(SCREEN_SIZE[0] - 64), stick_to_screen_y(0)) && mouse_clicked(LEFT_BUTTON)) {
      window_toggle_fullscreen(main_window);
      play_sound_effect("Toggle");
    }
  }

}

void draw_cursor() {
  if (!game_pause) {
    draw_bitmap(crosshair, stick_to_screen_x(adjusted_mouse_x() - 32), stick_to_screen_y(adjusted_mouse_y() - 32));
  }
  else {
    draw_bitmap(mouse_cursor, stick_to_screen_x(adjusted_mouse_x() - 8), stick_to_screen_y(adjusted_mouse_y() - 8));
  }
}

int main() {
  int elasped_frames = 0;

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

  hide_mouse();

  play_music("test"); //temp

  bitmap test = load_bitmap("test", "Graphics/test.png"); //temp

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
        play_sound_effect("Toggle");
      }
    }

    int y_difference = stick_to_screen_y(SCREEN_SIZE[1] - adjusted_mouse_y()) - (SCREEN_SIZE[1] - player_position[1] + 64);
    int x_difference = stick_to_screen_x(adjusted_mouse_x()) - (player_position[0] + 32);
    float pointing_angle = atan2(y_difference, x_difference) * RADIANS_TO_DEGREES;

    if (!game_pause) {
      set_camera_x(player_position[0] - SCREEN_SIZE[0] / 2 + 32 + cos(pointing_angle * DEGREES_TO_RADIANS) * abs((float)x_difference / 16)); //60
      set_camera_y((SCREEN_SIZE[1] - player_position[1]) - SCREEN_SIZE[1] / 2 + 64 - sin(pointing_angle * DEGREES_TO_RADIANS) * abs((float)y_difference / 9));
    }

    process_player_movement();
    process_player_dash(elasped_frames, pointing_angle, bounce(elasped_frames, 1, 1) > 0.5, 10, 15);

    clear_screen(color_white());
    draw_bitmap(test, 0, 0);

    bitmap player = draw_bitmap_with_animation(elasped_frames, player_animation, player_position[0], SCREEN_SIZE[1] - player_position[1], 7);

    color hp_bar = rgba_color(255, 195, 195, 175 + (int)bounce(elasped_frames, 20, 2));
    fill_rectangle(hp_bar, stick_to_screen_x(25), stick_to_screen_y(25), player_health * 4, 25);
    draw_bitmap(music_heart, stick_to_screen_x(5), stick_to_screen_y(50 - bounce(elasped_frames, 4, 300)));

    process_pause_menu();
    draw_cursor();

    if (!game_pause) {
      elasped_frames = elasped_frames + 1;
    }

    refresh_screen(TARGET_FRAMERATE);
    process_events();
  }

  return 0;
}
