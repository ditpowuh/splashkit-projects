// Using these libraries (Standard + SplashKit)
#include "splashkit.h"
#include <filesystem>
#include <cmath>
#include <vector>
#include <map>

#include <chrono>

// Preprocessor macro to replace stick_to_screen_x and stick_to_screen_y with to_world_x and to_world_y - this is made for personal use as it makes it easier for me to understand
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

struct settings_data {
  float music_volume;
  float sound_volume;
};
typedef struct settings_data settings;

struct stage_data {
  int bpm;
  string music;
};

// Sets the position of the player as an array of doubles
double player_position[] = {0, 0};
// Sets the player speed as a double
double player_speed = 6;
// Creates an enumeration to represent direction
enum direction {
  UP,
  DOWN,
  LEFT,
  RIGHT
};
direction player_direction = DOWN;
int player_health = 100;
int player_power = 100;
bool player_moving = false;
enum player_action {
  NOTHING,
  SWING,
  SHOOT,
  DASH
};
player_action player_current_action = NOTHING;

int invincibility_frames = 0;

float dashing_angle;
float last_dashed;

int action_amount = 0;

const double diagonalMovement = sqrt(2) / 2;
double applied_movement[] = {0, 0};

enum projectile_type {
  PLAYER,
  ENEMY
};

font main_font = load_font("Fibberish", "Graphics/Fibberish.ttf");

bitmap text_button1 = load_bitmap("Button1", "Graphics/Button.png");
bitmap text_button2 = load_bitmap("Button2", "Graphics/Button.png");
bitmap music_heart = load_bitmap("Music Heart", "Graphics/Music Heart.png");
bitmap mouse_cursor = load_bitmap("Mouse Cursor", "Graphics/Mouse Cursor.png");
bitmap crosshair = load_bitmap("Crosshair", "Graphics/Crosshair.png");
bitmap selected = load_bitmap("Selected Button", "Graphics/Selected Button.png");
bitmap fullscreen = load_bitmap("Fullscreen", "Graphics/Fullscreen.png");
bitmap windowed = load_bitmap("Windowed", "Graphics/Windowed.png");
bitmap pause_separator = load_bitmap("Pause Separator", "Graphics/Pause Separator.png");

string player_animation = "IdleDown";
map<string, int> animation_map;

int bpm = 160;

bool game_running = true;
bool game_pause = false;

class enemy {
  public:
    int hp;
    double position[2];
    string animation;
    enemy(int hp, double x, double y, string animation) {
      this->hp = hp;
      position[0] = x;
      position[1] = y;
      this->animation = animation;
    }
};

class projectile {
  public:
    projectile_type type;
    double position[2];
    string animation;
    float angle;
    float speed;
    int damage;
    projectile(projectile_type type, double x, double y, string animation, float angle, float speed, int damage) {
      this->type = type;
      position[0] = x;
      position[1] = y;
      this->animation = animation;
      this->angle = angle;
      this->speed = speed;
      this->damage = damage;
    }

    void move() {
      position[0] = position[0] + cos(angle * DEGREES_TO_RADIANS) * speed;
      position[1] = position[1] + sin(angle * DEGREES_TO_RADIANS) * speed;
    }

    bool contact() {
      if (type == PLAYER) {

      }
      if (type == ENEMY) {

      }
    }
};

class particle {
  private:
    int lifespan;

  public:
    double position[2];
    string animation;
    particle(string animation, double x, double y, int lifespan) {
      this->animation = animation;
      position[0] = x;
      position[1] = y;
      this->lifespan = lifespan + 1;
    }

    bool finished() {
      lifespan = lifespan - 1;
      if (lifespan <= 0) {
        return true;
      }
      return false;
    }
};

class collectible {
  private:
    string sprite;
    bool health_increase;

  public:
    string item_name;
    double position[2];
    collectible(string item_name, double x, double y, string sprite) {
      this->item_name = item_name;
      position[0] = x;
      position[1] = y;
      this->sprite = sprite;
    }
};

vector<enemy> enemies;
vector<projectile> projectiles;

vector<particle> front_particles;
vector<particle> back_particles;

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

float angle_in_360(float angle) {
  float new_angle = angle;
  if (new_angle < 0) {
    new_angle = new_angle + 360;
  }
  return 360 - new_angle;
}

float pythagorean_theorem(float x, float y) {
  return sqrt(pow(x, 2) + pow(y, 2));
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

void begin_stage(stage_data stage) {
  bpm = stage.bpm;
  play_music(stage.music);
}

void process_player_movement(float angle) {
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

  if (applied_movement[0] != 0 || applied_movement[1] != 0 || action_amount > 0)  {
    player_moving = true;
    player_animation = "Run";
  }
  else {
    player_moving = false;
    player_animation = "Idle";
  }
  int dashing_direciton = (int)round((angle_in_360(angle) + 90) / 90);
  if (action_amount > 0) {
    switch (dashing_direciton) {
      case 1:
      case 5:
        player_direction = RIGHT;
        break;
      case 2:
        player_direction = DOWN;
        break;
      case 3:
        player_direction = LEFT;
        break;
      case 4:
        player_direction = UP;
        break;
    }
  }
  switch (player_direction) {
    case UP: player_animation = player_animation + "Up"; break;
    case DOWN: player_animation = player_animation + "Down"; break;
    case RIGHT: player_animation = player_animation + "Right"; break;
    case LEFT: player_animation = player_animation + "Left"; break;
  }

  if (game_pause) {
    return;
  }
  if (applied_movement[0] != 0 && applied_movement[1] != 0) {
    applied_movement[0] = applied_movement[0] * diagonalMovement;
    applied_movement[1] = applied_movement[1] * diagonalMovement;
  }
  if (player_current_action == NOTHING) {
    player_position[0] = player_position[0] + player_speed * applied_movement[0];
    player_position[1] = player_position[1] + player_speed * applied_movement[1];
  }

}

void process_player_dash(int elasped_frames, float angle, bool in_time, int duration, int strength) {
  if (game_pause) {
    return;
  }
  if ((action_amount > 0 && player_current_action == DASH) || (key_typed(SPACE_KEY) && in_time && player_current_action == NOTHING)) {
    if (action_amount > 0 && player_current_action == DASH) {
      action_amount = action_amount - 1;
      if (action_amount == 0) {
        player_current_action = NOTHING;
      }
    }
    if (key_typed(SPACE_KEY) && in_time && player_current_action == NOTHING) {
      player_current_action = DASH;
      action_amount = duration;
      dashing_angle = angle;

      last_dashed = elasped_frames; // maybe TEMP
    }
    string particle_animation = "Dash";
    switch (player_direction) {
      case UP: particle_animation = particle_animation + "Up"; break;
      case DOWN: particle_animation = particle_animation + "Down"; break;
      case RIGHT: particle_animation = particle_animation + "Right"; break;
      case LEFT: particle_animation = particle_animation + "Left"; break;
    }
    particle dash_particle(particle_animation, player_position[0], player_position[1], 8);
    back_particles.push_back(dash_particle);

    player_position[0] = player_position[0] + cos(dashing_angle * DEGREES_TO_RADIANS) * strength;
    player_position[1] = player_position[1] + sin(dashing_angle * DEGREES_TO_RADIANS) * strength;
  }
}

void process_player_attack(float angle) {
  if (game_pause) {
    return;
  }
  if (action_amount > 0 && player_current_action == SWING) {
    action_amount = action_amount - 1;
    if (action_amount == 0) {
      player_current_action = NOTHING;
    }
  }
  if (action_amount > 0 && player_current_action == SHOOT) {
    action_amount = action_amount - 1;
    if (action_amount == 0) {
      player_current_action = NOTHING;
    }
  }
  if (mouse_clicked(LEFT_BUTTON)) {
    player_current_action = SWING;
    action_amount = 8;
  }
  if (mouse_clicked(RIGHT_BUTTON) && player_power >= 5) {
    player_current_action = SHOOT;
    action_amount = 8;
    projectile player_projectile(PLAYER, player_position[0] + 16, player_position[1] - 32, "PlayerProjectile", angle, 12.5, 10);
    projectiles.push_back(player_projectile);
    player_power = player_power - 5;
  }
  if (player_current_action == SWING) {
    player_position[0] = player_position[0] + cos(angle * DEGREES_TO_RADIANS) * 5;
    player_position[1] = player_position[1] + sin(angle * DEGREES_TO_RADIANS) * 5;
  }
  if (player_current_action == SHOOT) {
    player_position[0] = player_position[0] + cos((angle - 180) * DEGREES_TO_RADIANS) * 2;
    player_position[1] = player_position[1] + sin((angle - 180) * DEGREES_TO_RADIANS) * 2;
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

  draw_bitmap(pause_separator, stick_to_screen_x(570), stick_to_screen_y(290));

  draw_bitmap(text_button2, stick_to_screen_x(570), stick_to_screen_y(420));
  draw_text_on_window(main_window, "Quit", color_white(), main_font, 35, stick_to_screen_x(600), stick_to_screen_y(440));

  draw_rectangle(color_white(), stick_to_screen_x(0), stick_to_screen_y(0), SCREEN_SIZE[0], SCREEN_SIZE[1]);

  if (bitmap_collision(mouse_cursor, stick_to_screen_x(adjusted_mouse_x()), stick_to_screen_y(adjusted_mouse_y()), text_button1, stick_to_screen_x(585), stick_to_screen_y(245))) {
    draw_bitmap(selected, stick_to_screen_x(570 - 16), stick_to_screen_y(225 - 16));
    if (mouse_clicked(LEFT_BUTTON)) {
      game_pause = false;
      resume_music();
    }
  }

  if (bitmap_collision(mouse_cursor, stick_to_screen_x(adjusted_mouse_x()), stick_to_screen_y(adjusted_mouse_y()), text_button2, stick_to_screen_x(570), stick_to_screen_y(420))) {
    draw_bitmap(selected, stick_to_screen_x(570 - 16), stick_to_screen_y(420 - 16));
    if (mouse_clicked(LEFT_BUTTON)) {
      game_running = false;
    }
  }

  if (window_is_fullscreen(main_window)) {
    draw_bitmap(windowed, stick_to_screen_x(SCREEN_SIZE[0] - 64 - 1), stick_to_screen_y(1));
    if (bitmap_collision(mouse_cursor, stick_to_screen_x(adjusted_mouse_x()), stick_to_screen_y(adjusted_mouse_y()), windowed, stick_to_screen_x(SCREEN_SIZE[0] - 64), stick_to_screen_y(1)) && mouse_clicked(LEFT_BUTTON)) {
      window_toggle_fullscreen(main_window);
      play_sound_effect("Toggle");
    }
  }
  else {
    draw_bitmap(fullscreen, stick_to_screen_x(SCREEN_SIZE[0] - 64 - 1), stick_to_screen_y(1));
    if (bitmap_collision(mouse_cursor, stick_to_screen_x(adjusted_mouse_x()), stick_to_screen_y(adjusted_mouse_y()), fullscreen, stick_to_screen_x(SCREEN_SIZE[0] - 64), stick_to_screen_y(1)) && mouse_clicked(LEFT_BUTTON)) {
      window_toggle_fullscreen(main_window);
      play_sound_effect("Toggle");
    }
  }

}

void draw_cursor(bool force_cursor = false) {
  if (force_cursor) {
    draw_bitmap(mouse_cursor, stick_to_screen_x(adjusted_mouse_x() - 8), stick_to_screen_y(adjusted_mouse_y() - 8));
    return;
  }
  if (!game_pause) {
    draw_bitmap(crosshair, stick_to_screen_x(adjusted_mouse_x() - 32), stick_to_screen_y(adjusted_mouse_y() - 32));
  }
  else {
    draw_bitmap(mouse_cursor, stick_to_screen_x(adjusted_mouse_x() - 8), stick_to_screen_y(adjusted_mouse_y() - 8));
  }
}

void run_splash_screen(bitmap splash_text) {
  for (int i = 0; i < 2; i++) {
    for (int a = 255; a > 0; a--) {
      if (quit_requested()) {
        exit(0);
      }
      if (key_typed(F11_KEY)) {
        window_toggle_fullscreen(main_window);
        play_sound_effect("Toggle");
      }
      clear_screen(color_black());
      switch (i) {
        case 0:
          draw_bitmap(splash_text, SCREEN_SIZE[0] / 2 - 170, SCREEN_SIZE[1] / 2 - 76);
          break;
        case 1:
          draw_text_on_window(main_window, "FULLSCREEN RECOMMENDED", color_white(), main_font, 50, SCREEN_SIZE[0] / 2 - 250, SCREEN_SIZE[1] / 2 - 30);
          break;
      }
      color fade_effect = rgba_color(0, 0, 0, a);
      fill_rectangle(fade_effect, 0, 0, SCREEN_SIZE[0], SCREEN_SIZE[1]);

      ::draw_cursor(true);

      refresh_screen(TARGET_FRAMERATE);
      process_events();
    }
    for (int a = 0; a < 255; a++) {
      if (quit_requested()) {
        exit(0);
      }
      if (key_typed(F11_KEY)) {
        window_toggle_fullscreen(main_window);
        play_sound_effect("Toggle");
      }
      clear_screen(color_black());
      switch (i) {
        case 0:
          draw_bitmap(splash_text, SCREEN_SIZE[0] / 2 - 170, SCREEN_SIZE[1] / 2 - 76);
          break;
        case 1:
          draw_text_on_window(main_window, "FULLSCREEN RECOMMENDED", color_white(), main_font, 50, SCREEN_SIZE[0] / 2 - 250, SCREEN_SIZE[1] / 2 - 30);
          break;
      }
      color fade_effect = rgba_color(0, 0, 0, a);
      fill_rectangle(fade_effect, 0, 0, SCREEN_SIZE[0], SCREEN_SIZE[1]);

      ::draw_cursor(true);

      refresh_screen(TARGET_FRAMERATE);
      process_events();
    }
  }
}

int main() {
  // Loading all folders in Graphics folder
  for (const auto &sub_directories : filesystem::directory_iterator("Graphics")) {
    string sub_directory = sub_directories.path().filename().string();
    if (sub_directory.find(".") != string::npos) {
      continue;
    }
    int file_count = 0;
    // Loading all files for all the folders in the Graphics folder
    for (const auto &graphic_file : filesystem::directory_iterator("Graphics\\" + sub_directory)) {
      filesystem::path graphic_path = graphic_file.path();
      load_bitmap(sub_directory + graphic_path.filename().replace_extension().string(), graphic_path.string());
      file_count = file_count + 1;
    }
    animation_map[sub_directory] = file_count;
  }

  // Loading all music files in the Music folder
  for (const auto &music_file : filesystem::directory_iterator("Music")) {
    filesystem::path music_path = music_file.path();
    load_music(music_path.filename().replace_extension().string(), music_path.string());
  }

  // Loading all the sound files in the Sound folder
  for (const auto &sound_file : filesystem::directory_iterator("Sounds")) {
    filesystem::path sound_path = sound_file.path();
    load_sound_effect(sound_path.filename().replace_extension().string(), sound_path.string());
  }

  // Setting up the main window
  main_window = open_window("Alterheart", SCREEN_SIZE[0], SCREEN_SIZE[1]);
  window_toggle_border(main_window);
  hide_mouse();

  stage_data stage1 = {120, "test1"};

  bitmap test = load_bitmap("test", "Graphics/test1.png"); //TEMP

  bitmap splash_text = load_bitmap("Logo", "Graphics/Logo.png");
  //run_splash_screen(splash_text);
  free_bitmap(splash_text);

  // maybe main menu after in here

  int elasped_frames = 0;
  play_music("test"); //TEMP

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

    if (!window_has_focus(main_window) && !game_pause) {
      game_pause = true;
      pause_music();
      play_sound_effect("Pause");
    }

    int y_difference = stick_to_screen_y(SCREEN_SIZE[1] - adjusted_mouse_y()) - (SCREEN_SIZE[1] - player_position[1] + 64);
    int x_difference = stick_to_screen_x(adjusted_mouse_x()) - (player_position[0] + 32);
    float pointing_angle = atan2(y_difference, x_difference) * RADIANS_TO_DEGREES;
    float distance_difference = pythagorean_theorem(x_difference, y_difference);

    if (!game_pause) {
      set_camera_x(player_position[0] - SCREEN_SIZE[0] / 2 + 32 + cos(pointing_angle * DEGREES_TO_RADIANS) * abs((float)distance_difference / 16 * 2));
      set_camera_y((SCREEN_SIZE[1] - player_position[1]) - SCREEN_SIZE[1] / 2 + 64 - sin(pointing_angle * DEGREES_TO_RADIANS) * abs((float)distance_difference / 9 * 2));
    }

    process_player_movement(pointing_angle);
    process_player_dash(elasped_frames, pointing_angle, bounce(elasped_frames, 1, 1) > 0.5, 10, 15);
    process_player_attack(pointing_angle);

    clear_screen(color_white());
    draw_bitmap(test, 0, 0);

    for (int i = 0; i < back_particles.size(); i++) {
      draw_bitmap_with_animation(elasped_frames, back_particles[i].animation, back_particles[i].position[0], SCREEN_SIZE[1] - back_particles[i].position[1], 1);
      if (!game_pause) {
        if (back_particles[i].finished()) {
          back_particles.erase(back_particles.begin() + i);
        }
      }
    }

    if (invincibility_frames > 0) {
      if (elasped_frames % 6 == 0 || elasped_frames % 6 == 1 || elasped_frames % 6 == 2) {
        bitmap player = draw_bitmap_with_animation(elasped_frames, player_animation, player_position[0], SCREEN_SIZE[1] - player_position[1], 6);
      }
    }
    else {
      bitmap player = draw_bitmap_with_animation(elasped_frames, player_animation, player_position[0], SCREEN_SIZE[1] - player_position[1], 6);
    }

    for (int i = 0; i < projectiles.size(); i++) {
      draw_bitmap_with_animation(elasped_frames, projectiles[i].animation, projectiles[i].position[0], SCREEN_SIZE[1] - projectiles[i].position[1], 1);
      projectiles[i].move();
    }

    color hp_bar = rgba_color(255, 195, 195, 175 + (int)bounce(elasped_frames, 20, 2));
    fill_rectangle(hp_bar, stick_to_screen_x(25), stick_to_screen_y(25), player_health * 4, 25);
    color power_bar = rgba_color(192, 178, 209, 175 + (int)bounce(elasped_frames, 20, 2));
    fill_rectangle(power_bar, stick_to_screen_x(25), stick_to_screen_y(55), player_power * 4, 10);
    draw_bitmap(music_heart, stick_to_screen_x(5), stick_to_screen_y(65 - bounce(elasped_frames, 4, 300)));

    process_pause_menu();
    draw_cursor();

    if (!game_pause) {
      elasped_frames = elasped_frames + 1;
    }

    if (bounce(elasped_frames, 1, 2) > 0.9 && player_current_action != NOTHING) {
      if (!sound_effect_playing("Bling")) {
        play_sound_effect("Bling");
      }
    }

    refresh_screen(TARGET_FRAMERATE);
    process_events();
  }

  return 0;
}
