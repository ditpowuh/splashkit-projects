// Using these libraries (Standard + SplashKit)
#include "splashkit.h"
#include <filesystem>
#include <chrono>
#include <cmath>
#include <vector>
#include <queue>
#include <map>

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

// Constant for when the player moves diagonally
const double DIAGONAL_MOVEMENT = sqrt(2) / 2;

// Declares the main window for the game
window main_window;
// Declares a boolean to indicate whether fullscreen is available or not
bool fullscreen_available = true;

struct settings_data {
  float music_volume;
  float sound_volume;
};
settings_data settings = {1.0, 1.0};

struct stage_data {
  int bpm;
  string music;
  string name;
  bool boss;
};

struct position {
  double x;
  double y;
};

// Creates the position of the player
position player_position;
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
  SHOOT,
  CAST,
  DASH
};
player_action player_current_action = NOTHING;

int invincibility_frames = 0;
float falling_frames = 0;

position last_dash_position;
float last_dashed;
float dashing_angle;

double action_amount = 0;
float shake_amount = 0;

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
bitmap separator = load_bitmap("Menu Separator", "Graphics/Menu Separator.png");
bitmap health_icon = load_bitmap("Health Icon", "Graphics/Health Icon.png");
bitmap power_icon = load_bitmap("Power Icon", "Graphics/Power Icon.png");

string player_animation = "PlayerIdleDown";
map<string, int> animation_map;

int bpm = 160;

bool game_running = true;
bool game_pause = false;
bool game_lose = false;

double delta_time;

class enemy {
  public:
    int hp;
    position position;
    string animation;
    enemy(int hp, double x, double y, string animation) {
      this->hp = hp;
      position.x = x;
      position.y = y;
      this->animation = animation;
    }

    void move_by_angle(float angle, float speed) {
      position.x = position.x + cos(angle * DEGREES_TO_RADIANS) * speed * delta_time;
      position.y = position.y + sin(angle * DEGREES_TO_RADIANS) * speed * delta_time;
    }

    void move_by_value(double movement_x, double movement_y, float speed) {
      position.x = position.x + speed * delta_time;
      position.y = position.y + speed * delta_time;
    }

    void avoid_falling(bitmap map) {
      // position point_x = {player_position.x + 32 + applied_movement[0] * 8, SCREEN_SIZE[1] - player_position.y + 104};
      // position point_y = {player_position.x + 32, SCREEN_SIZE[1] - player_position.y + 104 - applied_movement[1] * 8};
      //
      // if (bitmap_point_collision(map, 0, 0, point_x.x, point_y.y)) {
      //   position.x = position.x + player_speed * applied_movement[0] * delta_time;
      // }
      // if (bitmap_point_collision(map, 0, 0, point_x.x, point_y.y)) {
      //   position.y = position.y + player_speed * applied_movement[1] * delta_time;
      // }
    }

};

class hoppers : public enemy {

};

class projectile {
  private:
    int animation_offset = 0;

  public:
    projectile_type type;
    position position;
    string animation;
    float angle;
    float speed;
    int animation_speed;
    int damage;
    projectile(projectile_type type, double x, double y, string animation, float angle, float speed, int animation_speed, int damage, int animation_offset = 0) {
      this->type = type;
      position.x = x;
      position.y = y;
      this->animation = animation;
      this->angle = angle;
      this->speed = speed;
      this->animation_speed = animation_speed;
      this->damage = damage;
      this->animation_offset = animation_offset;
    }

    int offset() {
      return animation_offset;
    }

    void move() {
      position.x = position.x + cos(angle * DEGREES_TO_RADIANS) * speed * delta_time;
      position.y = position.y + sin(angle * DEGREES_TO_RADIANS) * speed * delta_time;
    }

    bool out_of_range() {
      // Returns true if the position is arbitrarily far away from the player
      return position.x > 6000 || position.x < -200 || position.y > 1000 || position.y < -2200;
    }

};

class particle {
  private:
    int lifespan;

  public:
    position position;
    string animation;
    particle(string animation, double x, double y, int lifespan) {
      this->animation = animation;
      position.x = x;
      position.y = y;
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

bitmap draw_bitmap_with_animation(int elasped_time, string playing_animation, double position_x, double position_y, int animation_speed = 5, int offset = 0) {
  int current_frame = (elasped_time + offset) / animation_speed + 1;
  if (current_frame > animation_map[playing_animation]) {
    current_frame = current_frame - animation_map[playing_animation] * ((elasped_time + offset) / (animation_speed * animation_map[playing_animation]));
  }
  draw_bitmap(playing_animation + to_string(current_frame), position_x, position_y);
  return bitmap_named(playing_animation + to_string(current_frame));
}

float bounce(double sin_value, float strength, int sharpness = 500) {
  return abs(pow(sin(PI * sin_value / (60 / (float)bpm * TARGET_FRAMERATE) + PI / 2), sharpness) * strength);
}

float shake(float &shake_amount) {
  shake_amount = shake_amount * 0.9;
  return (rnd() * 2 - 1) * shake_amount;
}

void begin_stage(stage_data stage) {
  bpm = stage.bpm;
  while (!music_playing()) {
    play_music(stage.music);
  }
}

void prepare_stage(double &stage_display_time) {
  stage_display_time = 180;
}

void process_player_movement(float angle, bitmap map) {
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
    player_animation = "PlayerRun";
  }
  else {
    player_moving = false;
    player_animation = "PlayerIdle";
  }
  int action_direction = (int)round((angle_in_360(angle) + 90) / 90);
  if (action_amount > 0) {
    switch (action_direction) {
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
    applied_movement[0] = applied_movement[0] * DIAGONAL_MOVEMENT;
    applied_movement[1] = applied_movement[1] * DIAGONAL_MOVEMENT;
  }
  if (player_current_action == NOTHING) {
    position player_point_x = {player_position.x + 32 + applied_movement[0] * 8, SCREEN_SIZE[1] - player_position.y + 104};
    position player_point_y = {player_position.x + 32, SCREEN_SIZE[1] - player_position.y + 104 - applied_movement[1] * 8};

    if (bitmap_point_collision(map, 0, 0, player_point_x.x, player_point_x.y)) {
      player_position.x = player_position.x + player_speed * applied_movement[0] * delta_time;
    }
    if (bitmap_point_collision(map, 0, 0, player_point_y.x, player_point_y.y)) {
      player_position.y = player_position.y + player_speed * applied_movement[1] * delta_time;
    }

  }

}

void process_player_dash(double elasped_time, float angle, bool in_time, int duration, float strength) {
  if (game_pause) {
    return;
  }
  if ((action_amount > 0 && player_current_action == DASH) || (key_typed(SPACE_KEY) && in_time && player_current_action == NOTHING)) {
    if (action_amount > 0 && player_current_action == DASH) {
      action_amount = action_amount - delta_time;
      if (action_amount <= 0) {
        player_current_action = NOTHING;
      }
    }
    if (key_typed(SPACE_KEY) && in_time && player_current_action == NOTHING && elasped_time > last_dashed + (60.0 / bpm * TARGET_FRAMERATE) / 2) {
      player_current_action = DASH;
      action_amount = duration;
      dashing_angle = angle;

      last_dashed = elasped_time;
      last_dash_position = player_position;
      shake_amount = 25;
    }
    string particle_animation = "PlayerDash";
    switch (player_direction) {
      case UP: particle_animation = particle_animation + "Up"; break;
      case DOWN: particle_animation = particle_animation + "Down"; break;
      case RIGHT: particle_animation = particle_animation + "Right"; break;
      case LEFT: particle_animation = particle_animation + "Left"; break;
    }
    particle dash_particle(particle_animation, player_position.x, player_position.y, 8);
    back_particles.push_back(dash_particle);

    player_position.x = player_position.x + cos(dashing_angle * DEGREES_TO_RADIANS) * strength * delta_time;
    player_position.y = player_position.y + sin(dashing_angle * DEGREES_TO_RADIANS) * strength * delta_time;
  }
}

void process_player_attack(int elasped_time, float angle, bitmap map) {
  if (game_pause) {
    return;
  }
  if (action_amount > 0 && (player_current_action == CAST || player_current_action == SHOOT)) {
    action_amount = action_amount - delta_time;
    if (action_amount <= 0) {
      player_current_action = NOTHING;
    }
  }
  if (mouse_clicked(LEFT_BUTTON) && player_power >= 12) {
    player_current_action = SHOOT;
    action_amount = 8;
    projectile player_projectile(PLAYER, player_position.x + 16, player_position.y - 32, "PlayerProjectile", angle + rnd(-10, 10), 12.5, 2, 10, elasped_time);
    projectiles.push_back(player_projectile);
    play_sound_effect("Shoot");
    player_power = player_power - 12;
    shake_amount = 15;
  }
  if (mouse_clicked(RIGHT_BUTTON) && player_power >= 55) {
    player_current_action = CAST;
    action_amount = 8;
    player_health = player_health + 5;
    particle melee_particle("Sigil", player_position.x - 32, player_position.y - 16, 60);
    back_particles.push_back(melee_particle);
    play_sound_effect("Sigil");
    for (int i = 0; i < 1; i++) {
      // NOTE: check for every enemy if distance is shorter than 5 or etc. then damage
    }
    if (player_health > 100) {
      player_health = 100;
    }
    player_power = player_power - 55;
    shake_amount = 500;
  }
  if (key_typed(Q_KEY) && player_power >= 45) {
    player_current_action = SHOOT;
    action_amount = 8;
    for (int i = 0; i < 3; i++) {
      projectile player_projectile(PLAYER, player_position.x + 16, player_position.y - 32, "PlayerProjectile", angle - (30 * i - 30) + rnd(-10, 10), 12.5, 2, 10, elasped_time);
      projectiles.push_back(player_projectile);
    }
    play_sound_effect("Blast");
    player_power = player_power - 45;
    shake_amount = 25;
  }
  if (player_current_action == SHOOT) {
    position player_point_x = {player_position.x + 32 + cos((angle - 180) * DEGREES_TO_RADIANS) * 8, SCREEN_SIZE[1] - player_position.y + 104};
    position player_point_y = {player_position.x + 32, SCREEN_SIZE[1] - player_position.y + 104 - sin((angle - 180) * DEGREES_TO_RADIANS) * 8};

    if (bitmap_point_collision(map, 0, 0, player_point_x.x, player_point_x.y)) {
      player_position.x = player_position.x + cos((angle - 180) * DEGREES_TO_RADIANS) * 2 * delta_time;
    }
    if (bitmap_point_collision(map, 0, 0, player_point_y.x, player_point_y.y)) {
      player_position.y = player_position.y + sin((angle - 180) * DEGREES_TO_RADIANS) * 2 * delta_time;
    }
  }
}

void process_pause_menu() {
  if (!game_pause) {
    return;
  }
  fill_rectangle(rgba_color(0, 0, 0, 125), stick_to_screen_x(0), stick_to_screen_y(0), SCREEN_SIZE[0], SCREEN_SIZE[1]);
  draw_text_on_window(main_window, "Paused!", color_white(), main_font, 125, stick_to_screen_x(450), stick_to_screen_y(100));

  draw_bitmap(text_button1, stick_to_screen_x(570), stick_to_screen_y(225));
  draw_text_on_window(main_window, "Resume", color_white(), main_font, 35, stick_to_screen_x(585), stick_to_screen_y(245));

  draw_bitmap(separator, stick_to_screen_x(570), stick_to_screen_y(290));

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

  if (!fullscreen_available) {
    return;
  }
  if (window_is_fullscreen(main_window)) {
    draw_bitmap(windowed, stick_to_screen_x(SCREEN_SIZE[0] - 64 - 1), stick_to_screen_y(1));
    if (bitmap_collision(mouse_cursor, stick_to_screen_x(adjusted_mouse_x()), stick_to_screen_y(adjusted_mouse_y()), windowed, stick_to_screen_x(SCREEN_SIZE[0] - 64), stick_to_screen_y(1)) && mouse_clicked(LEFT_BUTTON)) {
      window_toggle_fullscreen(main_window);
      play_sound_effect("Click");
    }
  }
  else {
    draw_bitmap(fullscreen, stick_to_screen_x(SCREEN_SIZE[0] - 64 - 1), stick_to_screen_y(1));
    if (bitmap_collision(mouse_cursor, stick_to_screen_x(adjusted_mouse_x()), stick_to_screen_y(adjusted_mouse_y()), fullscreen, stick_to_screen_x(SCREEN_SIZE[0] - 64), stick_to_screen_y(1)) && mouse_clicked(LEFT_BUTTON)) {
      window_toggle_fullscreen(main_window);
      play_sound_effect("Click");
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
  for (int i = 0; i < 3; i++) {
    for (int a = 85; a > 0; a--) {
      if (quit_requested()) exit(0);
      if (key_typed(ESCAPE_KEY)) {
        play_sound_effect("Click");
        return;
      }
      if (key_typed(F11_KEY) && fullscreen_available) {
        window_toggle_fullscreen(main_window);
        play_sound_effect("Click");
      }
      clear_screen(color_black());
      switch (i) {
        case 0:
          draw_bitmap(splash_text, SCREEN_SIZE[0] / 2 - 170, SCREEN_SIZE[1] / 2 - 76);
          break;
        case 1:
          draw_text_on_window(main_window, "MAY CONTAIN FLASHING LIGHTS", color_white(), main_font, 50, SCREEN_SIZE[0] / 2 - 300, SCREEN_SIZE[1] / 2 - 20);
          break;
        case 2:
          draw_text_on_window(main_window, "FULLSCREEN & HEADPHONES", color_white(), main_font, 50, SCREEN_SIZE[0] / 2 - 250, SCREEN_SIZE[1] / 2 - 40);
          draw_text_on_window(main_window, "RECOMMENDED", color_white(), main_font, 50, SCREEN_SIZE[0] / 2 - 125, SCREEN_SIZE[1] / 2);
          break;
      }
      color fade_effect = rgba_color(0, 0, 0, a * 3);
      fill_rectangle(fade_effect, 0, 0, SCREEN_SIZE[0], SCREEN_SIZE[1]);

      draw_cursor(true);

      refresh_screen(TARGET_FRAMERATE);
      process_events();
    }
    int counter = 0;
    while (counter < 90) {
      if (quit_requested()) exit(0);
      if (key_typed(ESCAPE_KEY)) {
        play_sound_effect("Click");
        return;
      }
      if (key_typed(F11_KEY) && fullscreen_available) {
        window_toggle_fullscreen(main_window);
        play_sound_effect("Click");
      }
      clear_screen(color_black());
      switch (i) {
        case 0:
          draw_bitmap(splash_text, SCREEN_SIZE[0] / 2 - 170, SCREEN_SIZE[1] / 2 - 76);
          break;
        case 1:
          draw_text_on_window(main_window, "MAY CONTAIN FLASHING LIGHTS", color_white(), main_font, 50, SCREEN_SIZE[0] / 2 - 300, SCREEN_SIZE[1] / 2 - 20);
          break;
        case 2:
          draw_text_on_window(main_window, "FULLSCREEN & HEADPHONES", color_white(), main_font, 50, SCREEN_SIZE[0] / 2 - 250, SCREEN_SIZE[1] / 2 - 40);
          draw_text_on_window(main_window, "RECOMMENDED", color_white(), main_font, 50, SCREEN_SIZE[0] / 2 - 125, SCREEN_SIZE[1] / 2);
          break;
      }
      counter = counter + 1;

      draw_cursor(true);

      refresh_screen(TARGET_FRAMERATE);
      process_events();
    }
    for (int a = 0; a < 85; a++) {
      if (quit_requested()) exit(0);
      if (key_typed(ESCAPE_KEY)) {
        play_sound_effect("Click");
        return;
      }
      if (key_typed(F11_KEY) && fullscreen_available) {
        window_toggle_fullscreen(main_window);
        play_sound_effect("Click");
      }
      clear_screen(color_black());
      switch (i) {
        case 0:
          draw_bitmap(splash_text, SCREEN_SIZE[0] / 2 - 170, SCREEN_SIZE[1] / 2 - 76);
          break;
        case 1:
          draw_text_on_window(main_window, "MAY CONTAIN FLASHING LIGHTS", color_white(), main_font, 50, SCREEN_SIZE[0] / 2 - 300, SCREEN_SIZE[1] / 2 - 20);
          break;
        case 2:
          draw_text_on_window(main_window, "FULLSCREEN & HEADPHONES", color_white(), main_font, 50, SCREEN_SIZE[0] / 2 - 250, SCREEN_SIZE[1] / 2 - 40);
          draw_text_on_window(main_window, "RECOMMENDED", color_white(), main_font, 50, SCREEN_SIZE[0] / 2 - 125, SCREEN_SIZE[1] / 2);
          break;
      }
      color fade_effect = rgba_color(0, 0, 0, a * 3);
      fill_rectangle(fade_effect, 0, 0, SCREEN_SIZE[0], SCREEN_SIZE[1]);

      draw_cursor(true);

      refresh_screen(TARGET_FRAMERATE);
      process_events();
    }
  }
  int counter = 0;
  while (!quit_requested() && counter < 60) {
    if (key_typed(ESCAPE_KEY)) {
      play_sound_effect("Click");
      return;
    }
    if (key_typed(F11_KEY) && fullscreen_available) {
      window_toggle_fullscreen(main_window);
      play_sound_effect("Click");
    }
    counter = counter + 1;
    clear_screen(color_black());
    draw_cursor(true);
    refresh_screen(TARGET_FRAMERATE);
    process_events();
  }
}

void take_damage_player(int health) {
  player_health = player_health - health;
  invincibility_frames = 60;
  play_sound_effect("Damage");
}

int main() {
  // If the user's display is smaller in any dimension to the game's window...
  if (display_width(DISPLAY_DETAILS) < SCREEN_SIZE[0] || display_height(DISPLAY_DETAILS) < SCREEN_SIZE[1]) {
    // ...the user will get a message saying that it will be problematic
    write_line("The display you are using is too small! Please use the resolution of 1280x720 at minimum.");
    // The program stops here as a result
    return 0;
  }
  // However, if the user's display is the exact resolution to the game's window...
  else if (display_width(DISPLAY_DETAILS) == SCREEN_SIZE[0] && display_height(DISPLAY_DETAILS) == SCREEN_SIZE[1]) {
    // ...fullscreen will be disabled as it does not have any effect
    fullscreen_available = false;
  }
  write_line("Starting...");

  // Writes to the console what is happening
  write_line("Loading animations...");
  // Loading all folders in Graphics folder
  for (const auto &sub_directories : filesystem::directory_iterator("Graphics")) {
    string sub_directory = sub_directories.path().filename().string();
    // If it is not a subdirectory, and it is a file, then it is skipped
    if (sub_directory.find(".") != string::npos) {
      continue;
    }
    // Sets/resets the file count
    int file_count = 0;
    // Loading all files for all the folders in the Graphics folder
    for (const auto &graphic_file : filesystem::directory_iterator("Graphics\\" + sub_directory)) {
      // Gets the path to the graphic file
      filesystem::path graphic_path = graphic_file.path();
      // Loads it as the name of the folder and the file name without the extension
      load_bitmap(sub_directory + graphic_path.filename().replace_extension().string(), graphic_path.string());
      // Increments the file count
      file_count = file_count + 1;
    }
    // Maps the file count to the name of the folder
    animation_map[sub_directory] = file_count;
  }

  // Writes to the console what is happening
  write_line("Loading music...");
  // Loading all music files in the Music folder
  for (const auto &music_file : filesystem::directory_iterator("Music")) {
    // Gets the path to the music file
    filesystem::path music_path = music_file.path();
    // Loads the music as the name of the file
    load_music(music_path.filename().replace_extension().string(), music_path.string());
  }

  // Writes to the console what is happening
  write_line("Loading sounds...");
  // Loading all the sound files in the Sound folder
  for (const auto &sound_file : filesystem::directory_iterator("Sounds")) {
    // Gets the path to the sounds file
    filesystem::path sound_path = sound_file.path();
    load_sound_effect(sound_path.filename().replace_extension().string(), sound_path.string());
  }

  // Writes to the console what is happening
  write_line("Setting up Alterheart...");
  // Setting up the main window
  main_window = open_window("Alterheart", SCREEN_SIZE[0], SCREEN_SIZE[1]);
  window_toggle_border(main_window);
  hide_mouse();

  queue<stage_data> stages;
  stages.push({120, "Alterheart OST 02 - Heartbeat Horizon", "Stage 1", false});
  stages.push({140, "Alterheart OST 03 - Pulsing Pursuit", "Stage 2", false});
  stages.push({160, "Alterheart OST 04 - Calls from the Core", "Boss!!!", true});

  bitmap traversable_map = load_bitmap("Map1", "Graphics/Island.png");
  bitmap visual_map = load_bitmap("Map2", "Graphics/Under Island.png");

  // Writes to the console what is happening
  write_line("Opened Alterheart!");
  // Checks if the display the user is using is at 16:9 display ratio
  if ((float)display_width(DISPLAY_DETAILS) / (float)display_height(DISPLAY_DETAILS) != (float)16 / (float)9) {
    // Disables the ability to fullscren the application
    fullscreen_available = false;
    while (true) {
      if (quit_requested()) {
        return 0;
      }
      if (any_key_pressed() || mouse_clicked(LEFT_BUTTON)) {
        play_sound_effect("Click");
        break;
      }
      clear_screen(color_black());
      draw_text_on_window(main_window, "FULLSCREEN IS DISABLED AS YOUR SCREEN", color_white(), main_font, 50, SCREEN_SIZE[0] / 2 - 425, SCREEN_SIZE[1] / 2 - 60);
      draw_text_on_window(main_window, "RESOLUTION IS NOT IN THE 16:9 RATIO!", color_white(), main_font, 50, SCREEN_SIZE[0] / 2 - 400, SCREEN_SIZE[1] / 2 - 20);
      draw_text_on_window(main_window, "Press any key or left click if understood.", color_white(), main_font, 20, SCREEN_SIZE[0] / 2 - 150, SCREEN_SIZE[1] / 2 + 25);

      draw_cursor(true);

      refresh_screen(TARGET_FRAMERATE);
      process_events();
    }
  }

  bitmap splash_text = load_bitmap("Initial Logo", "Graphics/Developer Logo.png");
  run_splash_screen(splash_text);
  free_bitmap(splash_text);

  bitmap game_logo = load_bitmap("Game Logo", "Graphics/Game Logo.png");
  float bounce_effect = 0;

  while (!quit_requested()) {
    if (key_typed(F11_KEY) && fullscreen_available) {
      window_toggle_fullscreen(main_window);
      play_sound_effect("Click");
    }
    if (!music_playing()) {
      play_music("Alterheart OST 01 - Alterheart Ambience");
    }
    clear_screen(color_black());

    draw_bitmap(game_logo, 325, 20 + sin(bounce_effect / 10) * 3);

    draw_bitmap(text_button1, 580, 235);
    draw_text_on_window(main_window, "Play", color_white(), main_font, 35, 615, 255);

    draw_bitmap(separator, 580, 300);

    draw_bitmap(text_button2, 580, 430);
    draw_text_on_window(main_window, "Quit", color_white(), main_font, 35, 610, 450);

    if (bitmap_collision(mouse_cursor, adjusted_mouse_x(), adjusted_mouse_y(), text_button1, 580, 235)) {
      draw_bitmap(selected, 580 - 16, 235 - 16);
      if (mouse_clicked(LEFT_BUTTON)) {
        play_sound_effect("Play");
        shake_amount = 1000;
        stop_music();
        break;
      }
    }

    if (bitmap_collision(mouse_cursor, adjusted_mouse_x(), adjusted_mouse_y(), text_button2, 580, 430)) {
      draw_bitmap(selected, 580 - 16, 430 - 16);
      if (mouse_clicked(LEFT_BUTTON)) {
        exit(0);
      }
    }

    draw_cursor(true);
    bounce_effect = bounce_effect + 1;

    refresh_screen(TARGET_FRAMERATE);
    process_events();
  }
  free_bitmap(game_logo);

  position spawn_point = {2910, -630};
  player_position = spawn_point;
  stage_data current_stage;

  double elasped_time = 0;
  double beat_offset = 0;
  double no_music_playing = 0;

  bool new_stage = true;
  double preparing_stage;
  prepare_stage(preparing_stage);

  chrono::steady_clock::time_point previous_time = chrono::steady_clock::now();
  while (game_running && !quit_requested()) {

    chrono::steady_clock::time_point current_time = chrono::steady_clock::now();
    delta_time = chrono::duration_cast<chrono::duration<double>>(current_time - previous_time).count();
    delta_time = delta_time * TARGET_FRAMERATE * (60.0 / TARGET_FRAMERATE);
    previous_time = chrono::steady_clock::now();

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
    if (key_typed(F11_KEY) && fullscreen_available) {
      if (game_pause) {
        window_toggle_fullscreen(main_window);
        play_sound_effect("Click");
      }
    }

    if (!window_has_focus(main_window) && !game_pause) {
      game_pause = true;
      pause_music();
      play_sound_effect("Pause");
    }

    int y_difference = stick_to_screen_y(SCREEN_SIZE[1] - adjusted_mouse_y()) - (SCREEN_SIZE[1] - player_position.y + 64);
    int x_difference = stick_to_screen_x(adjusted_mouse_x()) - (player_position.x + 32);
    float pointing_angle = atan2(y_difference, x_difference) * RADIANS_TO_DEGREES;
    float distance_difference = pythagorean_theorem(x_difference, y_difference);

    if (!game_pause) {
      set_camera_x(player_position.x - SCREEN_SIZE[0] / 2 + 32 + cos(pointing_angle * DEGREES_TO_RADIANS) * abs((float)distance_difference / 16 * 2) + shake(shake_amount));
      set_camera_y((SCREEN_SIZE[1] - player_position.y) - SCREEN_SIZE[1] / 2 + 64 - sin(pointing_angle * DEGREES_TO_RADIANS) * abs((float)distance_difference / 9 * 2) + shake(shake_amount));
    }

    process_player_movement(pointing_angle, traversable_map);
    if (!new_stage) {
      process_player_dash(elasped_time, pointing_angle, bounce(elasped_time - beat_offset, 1, 1) > 0.5, 10, 17.5);
      process_player_attack(elasped_time, pointing_angle, traversable_map);
    }

    clear_screen(rgba_color(219, 157, 225, 255));

    draw_bitmap_with_animation(elasped_time, "Water", 0, 0, 3);
    draw_bitmap(visual_map, 0, 0);
    draw_bitmap(traversable_map, 0, 0);

    position player_point = {player_position.x + 32, SCREEN_SIZE[1] - player_position.y + 104};
    if (!bitmap_point_collision(traversable_map, 0, 0, player_point.x, player_point.y) && player_current_action != DASH && !game_pause && !new_stage) {
      falling_frames = falling_frames + delta_time;
      if (falling_frames > 9) {
        play_sound_effect("Fall");
        falling_frames = 0;
        player_position = last_dash_position;
        take_damage_player(10);
      }
    }

    for (int i = 0; i < back_particles.size(); i++) {
      draw_bitmap_with_animation(elasped_time, back_particles[i].animation, back_particles[i].position.x, SCREEN_SIZE[1] - back_particles[i].position.y, 1);
      if (!game_pause) {
        if (back_particles[i].finished()) {
          back_particles.erase(back_particles.begin() + i);
        }
      }
    }

    bitmap player;
    if (invincibility_frames > 0) {
      invincibility_frames = invincibility_frames - 1;
      if ((int)elasped_time % 6 == 0 || (int)elasped_time % 6 == 1 || (int)elasped_time % 6 == 2) {
        player = draw_bitmap_with_animation(elasped_time, player_animation, player_position.x, SCREEN_SIZE[1] - player_position.y, 6);
      }
    }
    else {
      player = draw_bitmap_with_animation(elasped_time, player_animation, player_position.x, SCREEN_SIZE[1] - player_position.y, 6);
    }

    for (int i = 0; i < projectiles.size(); i++) {
      draw_bitmap_with_animation(elasped_time, projectiles[i].animation, projectiles[i].position.x, SCREEN_SIZE[1] - projectiles[i].position.y, projectiles[i].animation_speed, projectiles[i].offset());
      if (!game_pause) {
        projectiles[i].move();
        if (projectiles[i].out_of_range()) {
          projectiles.erase(projectiles.begin() + i);
        }
        // NOTE: check for collision with every enemy
      }
    }

    if (bounce(elasped_time, 1, 150) > 0.5 && player_current_action != SHOOT && !game_pause) {
      player_power = player_power + 8;
      if (player_power > 100) {
        player_power = 100;
      }
    }

    // TEMP: testing out changing stages
    if (key_typed(E_KEY)) {
      shake_amount = 1000;
      new_stage = true;
      stop_music();
    }

    if (!new_stage) {
      color hp_bar = rgba_color(255, 195, 210, 180 + (int)bounce(elasped_time - beat_offset, 25, 2));
      fill_rectangle(hp_bar, stick_to_screen_x(25), stick_to_screen_y(25), player_health * 4.5, 32);
      draw_bitmap(health_icon, stick_to_screen_x(25), stick_to_screen_y(25));
      color power_bar = rgba_color(192, 178, 209, 180 + (int)bounce(elasped_time - beat_offset, 25, 2));
      fill_rectangle(power_bar, stick_to_screen_x(25), stick_to_screen_y(65), player_power * 4.5, 16);
      draw_bitmap(power_icon, stick_to_screen_x(25), stick_to_screen_y(65));
      draw_bitmap(music_heart, stick_to_screen_x(5), stick_to_screen_y(75 - bounce(elasped_time - beat_offset, 4, 300)));
    }
    else {
      color hp_bar = rgba_color(255, 195, 210, 180);
      fill_rectangle(hp_bar, stick_to_screen_x(25), stick_to_screen_y(25), player_health * 4.5, 32);
      draw_bitmap(health_icon, stick_to_screen_x(25), stick_to_screen_y(25));
      color power_bar = rgba_color(192, 178, 209, 180);
      fill_rectangle(power_bar, stick_to_screen_x(25), stick_to_screen_y(65), player_power * 4.5, 16);
      draw_bitmap(power_icon, stick_to_screen_x(25), stick_to_screen_y(65));
      draw_bitmap(music_heart, stick_to_screen_x(5), stick_to_screen_y(75));
    }

    if (new_stage && !game_pause) {
      stage_data next_stage = stages.front();
      if (preparing_stage > 0) {
        draw_text_on_window(main_window, next_stage.name, color_white(), main_font, 64, stick_to_screen_x(SCREEN_SIZE[0] / 2 - 96), stick_to_screen_y(SCREEN_SIZE[1] / 2 - 24));
        preparing_stage = preparing_stage - delta_time;
      }
      else {
        prepare_stage(preparing_stage);
        current_stage = next_stage;
        stages.pop();
        begin_stage(current_stage);
        beat_offset = elasped_time;
        new_stage = false;
      }
    }
    process_pause_menu();
    draw_cursor();

    if (!game_pause) {
      elasped_time = elasped_time + delta_time;
    }

    if (player_health < 0) {
      player_health = 0;
    }
    if (player_health == 0) {
      game_running = false;
      game_lose = true;
    }

    if (!music_playing() && !new_stage && !game_pause) {
      no_music_playing = no_music_playing + delta_time;
      if (no_music_playing > 180) {
        game_running = false;
        game_lose = true;
      }
    }
    else {
      no_music_playing = 0;
    }

    int current_fps = (int)round(60 / delta_time);
    draw_text_on_window(main_window, to_string(current_fps), color_white(), main_font, 16, stick_to_screen_x(8), stick_to_screen_y(SCREEN_SIZE[1] - 20));

    refresh_screen(TARGET_FRAMERATE);
    process_events();
  }

  while (game_lose) {
    break;
    // NOTE: lose screen
  }

  return 0;
}
