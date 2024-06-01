// Using these libraries (SplashKit + Standard)
#include "splashkit.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
// Using different data structures from standard
#include <unordered_map>
#include <vector>
#include <queue>

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

// Create a struct for settings and creates settings with 1 as the default
struct settings_data {
  float music_volume;
  float sound_volume;
};
settings_data settings = {1.0, 1.0};

struct stage_data {
  int bpm;
  string music;
  string name;
  int enemies_to_pass;
  string boss = "";
};

// Creates a struct to represent a position
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
// Sets the player's direction to down
direction player_direction = DOWN;
// Sets player's health to 100 by default
int player_health = 100;
// Sets the player's power to 100 by default
int player_power = 100;
// Creates an enumeration to represent the actions of the player
enum player_action {
  NOTHING,
  SHOOT,
  CAST,
  DASH
};
// Sets the player's current action is nothing
player_action player_current_action = NOTHING;

// Sets frame counters for invincibility frames and falling frames
int invincibility_frames = 0;
float falling_frames = 0;

// Initalises different variables for dashing
position last_dash_position;
float last_dashed;
float dashing_angle;

// Sets action amount to 0
double action_amount = 0;
// Sets shake amount to 0
float shake_amount = 0;

// Creates an enumeration for the different projectile types
enum projectile_type {
  PLAYER,
  ENEMY
};

// Creates and loads the font Fibberish
font main_font = load_font("Fibberish", "Graphics/Fibberish.ttf");

// Loads different bitmaps that are to be static
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

// Create a string for the player's animation
string player_animation = "PlayerIdleDown";
// Creates an unordered map to map the length of each animation
unordered_map<string, int> animation_map;

// Creates an integer to represent the BPM
int bpm = 100;

// Creates booleans to represent the game's state
bool game_running = true;
bool game_pause = false;
bool game_lose = false;

// Initalises the delta time variable
double delta_time;

class enemy {
  public:
    int hp;
    position current_position;
    string animation;
    bool floating;
    double speed;
    enemy(int hp, double x, double y, string animation, bool floating, double speed) {
      this->hp = hp;
      current_position.x = x;
      current_position.y = y;
      this->animation = animation;
      this->floating = floating;
      this->speed = speed;
    }

    void move(bitmap map, double applied_x, double applied_y) {
      position point_x = {current_position.x + applied_x * 8, SCREEN_SIZE[1] - current_position.y};
      position point_y = {current_position.x, SCREEN_SIZE[1] - current_position.y - applied_y * 8};

      if (bitmap_point_collision(map, 0, 0, point_x.x, point_x.y) || floating) {
        current_position.x = current_position.x + applied_x * speed * delta_time;
      }
      if (bitmap_point_collision(map, 0, 0, point_y.x, point_y.y) || floating) {
        current_position.y = current_position.y + applied_y * speed * delta_time;
      }
    }

    void move(bitmap map, float angle) {
      position point_x = {current_position.x + cos(angle * DEGREES_TO_RADIANS) * 8, SCREEN_SIZE[1] - current_position.y};
      position point_y = {current_position.x, SCREEN_SIZE[1] - current_position.y - sin(angle * DEGREES_TO_RADIANS) * 8};

      if (bitmap_point_collision(map, 0, 0, point_x.x, point_x.y) || floating) {
        current_position.x = current_position.x + cos(angle * DEGREES_TO_RADIANS) * speed * delta_time;
      }
      if (bitmap_point_collision(map, 0, 0, point_y.x, point_y.y) || floating) {
        current_position.y = current_position.y + sin(angle * DEGREES_TO_RADIANS) * speed * delta_time;
      }
    }

    void damage(int damage_done) {
      play_sound_effect("Hit", settings.sound_volume);
      hp = hp - damage_done;
      if (hp < 0) {
        hp = 0;
      }
    }

    virtual ~enemy() = default;
};

class boss : public enemy {
  protected:
    int pattern_ID = 1;

  public:
    string boss_name;
    boss(int hp, string boss_name, double x, double y, string animation, bool floating, double speed) : enemy(hp, x, y, animation, floating, speed) {
      this->boss_name = boss_name;
    }
};

class fear : public boss {
  public:
    fear(double x, double y) : boss(2000, "Fear", x, y, "FearFloating", true, 4) {}
};

class wraith : public enemy {
  private:
    static const int patterns = 2;

    int pattern_ID = 1;
    bool complete_pattern = false;

  public:
    int attack_action = 0;
    wraith(double x, double y) : enemy(75, x, y, "WraithRunRight", false, 4) {}

    void next_pattern() {
      pattern_ID = pattern_ID + 1;
      if (pattern_ID > patterns) {
        pattern_ID = 1;
      }
    }

    int pattern() {
      return pattern_ID;
    }
};

class mage : public enemy {
  private:
    int frequency = 120;
    int charge = 0;

  public:
    int type = 1;
    mage(double x, double y, int type) : enemy(100, x, y, "MageFloat", true, 1) {
      this->type = type;
    }

    bool ready_to_attack() {
      charge = charge + 1;
      if (charge > frequency) {
        charge = 0;
        return true;
      }
      return false;
    }
};

class drone : public enemy {
  public:
    drone(double x, double y) : enemy(25, x, y, "DroneBomb", true, 7) {}
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

// Creates a vector of unique pointers for enemies
vector<unique_ptr<enemy>> enemies;
// Creates a vector of projectiles
vector<projectile> projectiles;

// Creates vectors of particles to display at the front or back
vector<particle> front_particles;
vector<particle> back_particles;

// Adjusts the mouse x position (for fullscreen)
float adjusted_mouse_x() {
  if (window_is_fullscreen(main_window)) {
    return mouse_x() * SCREEN_SIZE[0] / display_width(DISPLAY_DETAILS);
  }
  else {
    return mouse_x();
  }
}

// Adjusts the mouse y position (for fullscreen)
float adjusted_mouse_y() {
  if (window_is_fullscreen(main_window)) {
    return mouse_y() * SCREEN_SIZE[1] / display_height(DISPLAY_DETAILS);
  }
  else {
    return mouse_y();
  }
}

// Gets the angle by using the difference in x and y and arctan
float get_angle(float y_difference, float x_difference) {
  return atan2(y_difference, x_difference) * RADIANS_TO_DEGREES;
}

// Converts an angle to 360 degrees
float angle_in_360(float angle) {
  float new_angle = angle;
  if (new_angle < 0) {
    new_angle = new_angle + 360;
  }
  return 360 - new_angle;
}

// Calculates the distance using the differences in x and y
float calculate_distance(float x_difference, float y_difference) {
  return sqrt(pow(x_difference, 2) + pow(y_difference, 2));
}

// Calculates the distance using values using
float calculate_distance(float x1, float y1, float x2, float y2) {
  return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

// Calculates the distance using positions
float calculate_distance(position position1, position position2) {
  return sqrt(pow(position2.x - position1.x, 2) + pow(position2.y - position1.y, 2));
}

// Draws bitmap with animation, defaulting the animation to display a new bitmap every 5 frames
bitmap draw_bitmap_with_animation(int elasped_time, string playing_animation, double position_x, double position_y, int animation_speed = 5, int offset = 0) {
  // Gets the current frame
  int current_frame = (elasped_time + offset) / animation_speed + 1;
  // If the current frame is bigger than the map can handle, make it so that it is and within range
  if (current_frame > animation_map[playing_animation]) {
    current_frame = current_frame - animation_map[playing_animation] * ((elasped_time + offset) / (animation_speed * animation_map[playing_animation]));
  }
  // Draws the bitmap using the associated string
  draw_bitmap(playing_animation + to_string(current_frame), position_x, position_y);
  // Returns the bitmap that is being displayed
  return bitmap_named(playing_animation + to_string(current_frame));
}

// Returns a float after performing mathematics to make a bounce effect based on the BPM
float bounce(double sin_value, float strength, int sharpness = 500) {
  return abs(pow(sin(PI * sin_value / (60 / (float)bpm * TARGET_FRAMERATE) + PI / 2), sharpness) * strength);
}

// Changes the shake amount and returns a random value to move the camera by
float shake(float &shake_amount) {
  // Expontentially decreases the shake amount by multiplying 0.9
  shake_amount = shake_amount * pow(0.9, delta_time);
  // Returns the a random float between negative shake_amount and shake_amount
  return (rnd() * 2 - 1) * shake_amount;
}

// My own implementation of the A* / Astar algorithm
vector<position> perform_modified_astar(bitmap map, position starting_point, position ending_point, double increments = 1) {
  // Creates a new struct in the scope of this function (this is as I'm not going to use nodes anywhere else)
  struct node {
    position node_position;
    double distance_from_start;
    double distance_to_end;
  };
  // Creates a struct that is to be used for the priority queue - it checks for the lowest combined distance from the start and end together
  struct compare_node {
    bool operator()(const node& node1, const node& node2) {
      if (node1.distance_from_start + node1.distance_to_end == node2.distance_from_start + node2.distance_to_end) {
        return node1.distance_to_end > node2.distance_to_end;
      }
      return node1.distance_from_start + node1.distance_to_end > node2.distance_from_start + node2.distance_to_end;
    }
  };
  // Creates a priority queue of nodes, setting the container as a vector of nodes and the comparison to compare_node
  priority_queue<node, vector<node>, compare_node> nodes;
  // Creates a map that has a string as the key and a node as the value
  unordered_map<string, node> predecessors;
  node starting_node = {starting_point, 0, calculate_distance(starting_point, ending_point)};
  nodes.push(starting_node);

  // Defines the 8 directions as two arrays
  int directions_x[] = {0, 1, 1, 1, 0, -1, -1, -1};
  int directions_y[] = {1, 1, 0, -1, -1, -1, 0, 1};

  // Creates a vector for the explored nodes
  vector<node> explored_nodes;
  // While the priority queue of nodes is not empty, this will run
  while (!nodes.empty()) {
    // Gets the node with the smallest combined distance
    node current_node = nodes.top();
    // Adds it to the vector of explored nodes
    explored_nodes.push_back(current_node);
    // Removes the node
    nodes.pop();

    // If for some reason, the current node is somehow not colliding with the traversable map, it will be skipped
    if (!bitmap_point_collision(map, 0, 0, current_node.node_position.x, SCREEN_SIZE[1] - current_node.node_position.y)) {
      continue;
    }

    // If the target has been approximately reached, or the calculations start to get too big...
    if (calculate_distance(current_node.node_position, ending_point) < 0.75 * increments || nodes.size() > 500) {
      // Create a new vector that shows the final path
      vector<position> path;
      // While there is a predecessor for the current...
      while (predecessors.find(to_string(current_node.node_position.x) + " " + to_string(current_node.node_position.y)) != predecessors.end()) {
        // ...add the current node's position to the path
        path.push_back(current_node.node_position);
        // Set the current node to the predecessor of it
        current_node = predecessors[to_string(current_node.node_position.x) + " " + to_string(current_node.node_position.y)];
      }
      // Adds the starting position to the end of the path
      path.push_back(starting_point);
      // Reverses the path so it is in the right order
      reverse(path.begin(), path.end());
      // Returns the final path
      return path;
    }
    // For each of the 8 directions...
    for (int i = 0; i < 8; i++) {
      position neighbouring_position;
      // If the direction is diagonal...
      if (directions_x[i] != 0 && directions_y[i] != 0) {
        // ...the neighbouring position is set with normalised with diagonal movement
        neighbouring_position = {current_node.node_position.x + (double)directions_x[i] * increments * DIAGONAL_MOVEMENT, current_node.node_position.y + (double)directions_y[i] * increments * DIAGONAL_MOVEMENT};
      }
      // If not...
      else {
        // The neighbouring position is set
        neighbouring_position = {current_node.node_position.x + (double)directions_x[i] * increments, current_node.node_position.y + (double)directions_y[i] * increments};
      }
      // If the neighbouring position is not on the traversable map, it will be skipped
      if (!bitmap_point_collision(map, 0, 0, neighbouring_position.x, SCREEN_SIZE[1] - neighbouring_position.y)) {
        continue;
      }
      // Sets a boolean to see if the neighbouring node has beenn explored yet
      bool is_already_explored = false;
      // For each of the explored nodes...
      for (int j = 0; j < explored_nodes.size(); j++) {
        // ...Check if there is a node that is close by...
        if (calculate_distance(explored_nodes[j].node_position, neighbouring_position) < 0.75 * increments) {
          // ...Sets the boolean to true if there is and stop since it's already known that is has been explored
          is_already_explored = true;
          break;
        }
      }
      // If the node has not been explored...
      if (!is_already_explored) {
        // Create a new node with the attributes
        node added_new_node = {neighbouring_position, calculate_distance(neighbouring_position, starting_point), calculate_distance(neighbouring_position, ending_point)};
        // Set the new node's predecessor to the current node using the position (as a string) for the key
        predecessors[to_string(added_new_node.node_position.x) + " " + to_string(added_new_node.node_position.y)] = current_node;
        // Adds the new node to the priority queue
        nodes.push(added_new_node);
      }
    }
  }
  return vector<position>{starting_point};
}

// Begins thew new stage by changing the BPM and starting the music
void begin_stage(stage_data stage) {
  // Sets the BPM to the BPM that was set for the stage
  bpm = stage.bpm;
  // A while loop is used as sometimes the music would randomly not play
  while (!music_playing()) {
    // Plays the music for the stage
    play_music(stage.music);
  }
}

// Prepares the stage by setting the time to display the announcement text
void prepare_stage(double &stage_display_time) {
  stage_display_time = 180;
}

// Processes all the player movment including the dash
void process_player_movement(float angle, bitmap map) {
  double applied_movement[] = {0, 0};

  // If the game is not paused...
  if (!game_pause) {
    // ...For each of the movement key, the corresponding applied movement and direction is set
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

  // If there is any movement, the animation prefix is set to run
  if (applied_movement[0] != 0 || applied_movement[1] != 0 || action_amount > 0)  {
    player_animation = "PlayerRun";
  }
  // If not, the animation prefix is set to idle
  else {
    player_animation = "PlayerIdle";
  }
  // Converts the angle to an integer to be used in a switch statement
  int action_direction = (int)round((angle_in_360(angle) + 90) / 90);
  // If the action ammount is more than 0 (which means an action is occuring)...
  if (action_amount > 0) {
    // ...Set the player's direction based on the angle (that was converted to an integer)
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
  // Depending on the direction, the associated suffix is added for the animation name
  switch (player_direction) {
    case UP: player_animation = player_animation + "Up"; break;
    case DOWN: player_animation = player_animation + "Down"; break;
    case RIGHT: player_animation = player_animation + "Right"; break;
    case LEFT: player_animation = player_animation + "Left"; break;
  }

  // If the game is paused, stop here
  if (game_pause) {
    return;
  }
  // If both of the axis of the applied movement is not 0 (meaning it is diagonal)...
  if (applied_movement[0] != 0 && applied_movement[1] != 0) {
    // ...Multiply the values by diagonal movement to normalise the movement
    applied_movement[0] = applied_movement[0] * DIAGONAL_MOVEMENT;
    applied_movement[1] = applied_movement[1] * DIAGONAL_MOVEMENT;
  }
  // If the player is not doing any special action and is just moving regularly...
  if (player_current_action == NOTHING) {
    // Position points are made to check if the next location the player is heading is on the map
    position player_point_x = {player_position.x + 32 + applied_movement[0] * 8, SCREEN_SIZE[1] - player_position.y + 104};
    position player_point_y = {player_position.x + 32, SCREEN_SIZE[1] - player_position.y + 104 - applied_movement[1] * 8};

    // Peforms the checks and if it is fine, the position is allowed
    if (bitmap_point_collision(map, 0, 0, player_point_x.x, player_point_x.y)) {
      player_position.x = player_position.x + player_speed * applied_movement[0] * delta_time;
    }
    if (bitmap_point_collision(map, 0, 0, player_point_y.x, player_point_y.y)) {
      player_position.y = player_position.y + player_speed * applied_movement[1] * delta_time;
    }
  }
}

void process_player_dash(double elasped_time, float angle, bool in_time, int duration, float strength) {
  // If the game is paused, stop here
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
  // If the game is paused, stop here
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
    projectile player_projectile(PLAYER, player_position.x + 16, player_position.y - 32, "PlayerProjectile", angle + rnd(-10, 10), 12.5, 2, 25, elasped_time);
    projectiles.push_back(player_projectile);
    play_sound_effect("Shoot", settings.sound_volume);
    player_power = player_power - 12;
    shake_amount = 15;
  }
  if (mouse_clicked(RIGHT_BUTTON) && player_power >= 55) {
    player_current_action = CAST;
    action_amount = 8;
    player_health = player_health + 5;
    particle melee_particle("PlayerSigil", player_position.x - 32, player_position.y - 16, 60);
    back_particles.push_back(melee_particle);
    play_sound_effect("Sigil", settings.sound_volume);
    position player_point = {player_position.x + 32, SCREEN_SIZE[1] - player_position.y + 104};
    for (int i = 0; i < enemies.size(); i++) {
      if (calculate_distance((SCREEN_SIZE[1] - enemies[i]->current_position.y) - player_point.y, enemies[i]->current_position.x  - player_point.x) < 128) {
        for (int j = 0; j < 3; j++) {
          particle hit_particle("Hit", enemies[i]->current_position.x, enemies[i]->current_position.y, 13);
          front_particles.push_back(hit_particle);
        }
        enemies[i]->damage(50);
        player_health = player_health + 10;
      }
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
      projectile player_projectile(PLAYER, player_position.x + 16, player_position.y - 32, "PlayerProjectile", angle - (30 * i - 30) + rnd(-10, 10), 12.5, 2, 25, elasped_time);
      projectiles.push_back(player_projectile);
    }
    play_sound_effect("Blast", settings.sound_volume);
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
  // If the game is paused, stop here
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

  // If fullscreen is not available, stop here
  if (!fullscreen_available) {
    return;
  }
  if (window_is_fullscreen(main_window)) {
    draw_bitmap(windowed, stick_to_screen_x(SCREEN_SIZE[0] - 64 - 1), stick_to_screen_y(1));
    if (bitmap_collision(mouse_cursor, stick_to_screen_x(adjusted_mouse_x()), stick_to_screen_y(adjusted_mouse_y()), windowed, stick_to_screen_x(SCREEN_SIZE[0] - 64), stick_to_screen_y(1)) && mouse_clicked(LEFT_BUTTON)) {
      window_toggle_fullscreen(main_window);
      play_sound_effect("Click", settings.sound_volume);
    }
  }
  else {
    draw_bitmap(fullscreen, stick_to_screen_x(SCREEN_SIZE[0] - 64 - 1), stick_to_screen_y(1));
    if (bitmap_collision(mouse_cursor, stick_to_screen_x(adjusted_mouse_x()), stick_to_screen_y(adjusted_mouse_y()), fullscreen, stick_to_screen_x(SCREEN_SIZE[0] - 64), stick_to_screen_y(1)) && mouse_clicked(LEFT_BUTTON)) {
      window_toggle_fullscreen(main_window);
      play_sound_effect("Click", settings.sound_volume);
    }
  }
}

// Draws the cursor as a bitmap
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
        play_sound_effect("Click", settings.sound_volume);
        return;
      }
      if (key_typed(F11_KEY) && fullscreen_available) {
        window_toggle_fullscreen(main_window);
        play_sound_effect("Click", settings.sound_volume);
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

      // Refreshes the screen using the target framerate and processes the event
      refresh_screen(TARGET_FRAMERATE);
      process_events();
    }
    int counter = 0;
    while (counter < 90) {
      if (quit_requested()) exit(0);
      if (key_typed(ESCAPE_KEY)) {
        play_sound_effect("Click", settings.sound_volume);
        return;
      }
      if (key_typed(F11_KEY) && fullscreen_available) {
        window_toggle_fullscreen(main_window);
        play_sound_effect("Click", settings.sound_volume);
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

      // Refreshes the screen using the target framerate and processes the event
      refresh_screen(TARGET_FRAMERATE);
      process_events();
    }
    for (int a = 0; a < 85; a++) {
      if (quit_requested()) exit(0);
      if (key_typed(ESCAPE_KEY)) {
        play_sound_effect("Click", settings.sound_volume);
        return;
      }
      if (key_typed(F11_KEY) && fullscreen_available) {
        window_toggle_fullscreen(main_window);
        play_sound_effect("Click", settings.sound_volume);
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

      // Refreshes the screen using the target framerate and processes the event
      refresh_screen(TARGET_FRAMERATE);
      process_events();
    }
  }
  int counter = 0;
  while (!quit_requested() && counter < 60) {
    if (key_typed(ESCAPE_KEY)) {
      play_sound_effect("Click", settings.sound_volume);
      return;
    }
    if (key_typed(F11_KEY) && fullscreen_available) {
      window_toggle_fullscreen(main_window);
      play_sound_effect("Click", settings.sound_volume);
    }
    counter = counter + 1;
    clear_screen(color_black());
    draw_cursor(true);
    // Refreshes the screen using the target framerate and processes the event
    refresh_screen(TARGET_FRAMERATE);
    process_events();
  }
}

void take_damage_player(int health, bool ignore_invincibility = false) {
  if (invincibility_frames > 0 && !ignore_invincibility) {
    return;
  }
  player_health = player_health - health;
  invincibility_frames = 60;
  play_sound_effect("Damage", settings.sound_volume);
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
  write_line("Loading settings...");
  json settings_json;
  // If the JSON settings file exists...
  if (filesystem::exists("JSON/settings.json")) {
    // Attempt to read the file and set the settings struct to the values from it
    try {
      settings_json = json_from_file("settings.json");
      settings.music_volume = json_read_number(settings_json, "music_volume");
      settings.sound_volume = json_read_number(settings_json, "sound_volume");
    }
    // If an exception occurs, a message is shown regarding it and the JSON settings file is reset
    catch (...) {
      write_line("Problem with settings from JSON... Reverting to and using default settings...");
      settings_json = create_json();
      json_set_number(settings_json, "music_volume", settings.music_volume);
      json_set_number(settings_json, "sound_volume", settings.sound_volume);
      json_to_file(settings_json, "settings.json");
    }
  }
  // If not... (If the JSON settings file does not exist)
  else {
    write_line("No settings stored. Generating JSON file for settings...");
    // Create a new output file stream
    ofstream file("JSON/settings.json");
    // If the file is opened correctly...
    if (file.is_open()) {
      // ...The file will close as the file only needs to be created
      write_line("New JSON file generated.");
      file.close();
      // The JSON settings file is written in, with the default settings
      settings_json = create_json();
      json_set_number(settings_json, "music_volume", settings.music_volume);
      json_set_number(settings_json, "sound_volume", settings.sound_volume);
      json_to_file(settings_json, "settings.json");
    }
    // If the file did not correctly open for some reason, a message will be outputted
    else {
      write_line("Problem with generating a new file... Using default settings...");
    }
  }

  // If the settings has the music volume to 0, a message will appear talking about it
  if (settings.music_volume == 0) {
    write_line("Warning: Music is essential part of the game and without it, may hinder your experience.");
  }
  // Sets the volume of the music
  set_music_volume(settings.music_volume);

  // Writes to the console what is happening
  write_line("Setting up Alterheart...");
  // Setting up the main window
  main_window = open_window("Alterheart", SCREEN_SIZE[0], SCREEN_SIZE[1]);
  window_toggle_border(main_window);
  hide_mouse();

  // Creates a queue for the stages
  queue<stage_data> stages;
  // Enqueues the different stages
  stages.push({120, "Alterheart OST 02 - Heartbeat Horizon", "Stage 1", 25});
  stages.push({140, "Alterheart OST 03 - Pulsing Pursuit", "Stage 2", 30});
  stages.push({160, "Alterheart OST 04 - Calls from the Core", "Boss!!!", 35, "Fear"});

  // Loads the traversable map and visual map to display in game
  bitmap traversable_map = load_bitmap("Map1", "Graphics/Island.png");
  bitmap visual_map = load_bitmap("Map2", "Graphics/Under Island.png");

  // Writes to the console what is happening
  write_line("Opened Alterheart!");
  // Checks if the display the user is using is at 16:9 display ratio
  if ((float)display_width(DISPLAY_DETAILS) / (float)display_height(DISPLAY_DETAILS) != (float)16 / (float)9) {
    // Disables the ability to fullscren the application
    fullscreen_available = false;
    // Run indefinitely until a break
    while (true) {
      if (quit_requested()) {
        return 0;
      }
      // If any key is pressed or left click is clicked, play the click sound and break
      if (any_key_pressed() || mouse_clicked(LEFT_BUTTON)) {
        play_sound_effect("Click", settings.sound_volume);
        break;
      }
      // Clear the screen black
      clear_screen(color_black());
      // Write the different text to explain
      draw_text_on_window(main_window, "FULLSCREEN IS DISABLED AS YOUR SCREEN", color_white(), main_font, 50, SCREEN_SIZE[0] / 2 - 425, SCREEN_SIZE[1] / 2 - 60);
      draw_text_on_window(main_window, "RESOLUTION IS NOT IN THE 16:9 RATIO!", color_white(), main_font, 50, SCREEN_SIZE[0] / 2 - 400, SCREEN_SIZE[1] / 2 - 20);
      draw_text_on_window(main_window, "Press any key or left click if understood.", color_white(), main_font, 20, SCREEN_SIZE[0] / 2 - 150, SCREEN_SIZE[1] / 2 + 25);

      // Draw the cursor and force it to be the mouse pointer
      draw_cursor(true);

      // Refreshes the screen using the target framerate and processes the event
      refresh_screen(TARGET_FRAMERATE);
      process_events();
    }
  }

  // Loads the splash text bitmap
  bitmap splash_text = load_bitmap("Initial Logo", "Graphics/Developer Logo.png");
  // Runs the splash screen
  run_splash_screen(splash_text);
  // Frees the splash text bitmap
  free_bitmap(splash_text);

  // Loads the game logo bitmap
  bitmap game_logo = load_bitmap("Game Logo", "Graphics/Game Logo.png");
  // Sets a variable to make a bouncing effect
  float bounce_effect = 0;

  // Plays the impact sound at a reduced volume
  play_sound_effect("Impact", settings.sound_volume / 2);
  while (!quit_requested()) {
    set_music_volume(settings.music_volume);
    // Fullscreens if F11 is pressed and is available
    if (key_typed(F11_KEY) && fullscreen_available) {
      window_toggle_fullscreen(main_window);
      play_sound_effect("Click", settings.sound_volume);
    }
    // If music is not playing, then play music
    if (!music_playing()) {
      play_music("Alterheart OST 01 - Alterheart Ambience");
    }
    // Clear the screen black
    clear_screen(color_black());
    // Fills rectangle with a purple colour
    fill_rectangle(rgb_color(72, 55, 92), 32, 32, SCREEN_SIZE[0] - 64, SCREEN_SIZE[1] - 64);

    // Draws the logo with a bounce effect using a sine wave
    draw_bitmap(game_logo, 325, 20 + sin(bounce_effect / 10) * 3);

    draw_bitmap(text_button1, 580, 235);
    draw_text_on_window(main_window, "Play", color_white(), main_font, 35, 615, 255);

    draw_bitmap(separator, 580, 300);

    draw_bitmap(text_button2, 580, 430);
    draw_text_on_window(main_window, "Quit", color_white(), main_font, 35, 610, 450);

    if (bitmap_collision(mouse_cursor, adjusted_mouse_x(), adjusted_mouse_y(), text_button1, 580, 235)) {
      draw_bitmap(selected, 580 - 16, 235 - 16);
      if (mouse_clicked(LEFT_BUTTON)) {
        play_sound_effect("Play", settings.sound_volume);
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

    // Refreshes the screen using the target framerate and processes the event
    refresh_screen(TARGET_FRAMERATE);
    process_events();
  }
  // Frees the game logo bitmap
  free_bitmap(game_logo);

  // Create a position that is the spawn point for the player
  position spawn_point = {2910, -630};
  player_position = spawn_point;
  // Sets the previous player position
  position previous_player_position = player_position;
  // Initalises stage data for the current stage
  stage_data current_stage = {bpm, "", "", -1};

  // Creates a counter to count how enemies that have been killed
  int enemies_killed = 0;
  // Creates a vector to cache different valid positions for non-floating enemies
  vector<position> on_map_positions;
  // While the vector of positions is less than 20
  while (on_map_positions.size() < 20) {
    // Get a random position that is on the length of the water
    position random_position = {(double)rnd(-200, 6000), (double)rnd(-2200, 1000)};
    // If there is no collision, get another random position until it has a valid position
    while (!bitmap_point_collision(traversable_map, 0, 0, random_position.x, SCREEN_SIZE[1] - random_position.y)) {
      random_position = {(double)rnd(-200, 6000), (double)rnd(-2200, 1000)};
    }
    // Add the position to the vector
    on_map_positions.push_back(random_position);
  }

  // Sets different variables that are to be used for the game
  double elasped_time = 0;
  double beat_offset = 0;
  double no_music_playing = 0;

  // Sets different variables for stage changing
  bool new_stage = true;
  double preparing_stage;
  prepare_stage(preparing_stage);

  // Create a pointer for the current boss
  unique_ptr<boss> current_boss;
  float spawn_rate = 300;
  float spawn_enemy = 0;

  // Create a point in time called previous time
  chrono::steady_clock::time_point previous_time = chrono::steady_clock::now();
  while (game_running && !quit_requested()) {

    // Create a point in time called current time
    chrono::steady_clock::time_point current_time = chrono::steady_clock::now();
    // Get the difference in time and cast it to a double
    delta_time = chrono::duration_cast<chrono::duration<double>>(current_time - previous_time).count();
    // Modify the double so that it is
    delta_time = delta_time * TARGET_FRAMERATE * (60.0 / TARGET_FRAMERATE);
    // Change the previous point in time to now
    previous_time = chrono::steady_clock::now();

    set_music_volume(settings.music_volume);

    if (key_typed(ESCAPE_KEY)) {
      game_pause = !game_pause;
      if (game_pause) {
        pause_music();
        play_sound_effect("Pause", settings.sound_volume);
      }
      else {
        resume_music();
      }
    }
    if (key_typed(F11_KEY) && fullscreen_available) {
      if (game_pause) {
        window_toggle_fullscreen(main_window);
        play_sound_effect("Click", settings.sound_volume);
      }
    }

    if (!window_has_focus(main_window) && !game_pause) {
      game_pause = true;
      pause_music();
      play_sound_effect("Pause", settings.sound_volume);
    }

    int y_difference = stick_to_screen_y(SCREEN_SIZE[1] - adjusted_mouse_y()) - (SCREEN_SIZE[1] - player_position.y + 64);
    int x_difference = stick_to_screen_x(adjusted_mouse_x()) - (player_position.x + 32);
    float pointing_angle = get_angle(y_difference, x_difference);
    float distance_difference = calculate_distance(x_difference, y_difference);

    if (!game_pause) {
      set_camera_x(player_position.x - SCREEN_SIZE[0] / 2 + 32 + cos(pointing_angle * DEGREES_TO_RADIANS) * abs((float)distance_difference / 16 * 2) + shake(shake_amount));
      set_camera_y((SCREEN_SIZE[1] - player_position.y) - SCREEN_SIZE[1] / 2 + 64 - sin(pointing_angle * DEGREES_TO_RADIANS) * abs((float)distance_difference / 9 * 2) + shake(shake_amount));
    }

    process_player_movement(pointing_angle, traversable_map);
    if (!new_stage) {
      process_player_dash(elasped_time, pointing_angle, bounce(elasped_time - beat_offset, 1, 1) > 0.5, 10, 17.5);
      process_player_attack(elasped_time, pointing_angle, traversable_map);
    }

    clear_screen(rgb_color(219, 157, 225));

    draw_bitmap_with_animation(elasped_time, "Water", 0, 0, 3);
    draw_bitmap(visual_map, 0, 0);
    draw_bitmap(traversable_map, 0, 0);

    position player_point = {player_position.x + 32, SCREEN_SIZE[1] - player_position.y + 104};
    if (!bitmap_point_collision(traversable_map, 0, 0, player_point.x, player_point.y) && player_current_action != DASH && !game_pause && !new_stage) {
      falling_frames = falling_frames + delta_time;
      if (falling_frames > 9) {
        play_sound_effect("Fall", settings.sound_volume);
        falling_frames = 0;
        player_position = last_dash_position;
        take_damage_player(10, true);
      }
    }

    if ((int)elasped_time % 15 == 0) {
      if (calculate_distance(previous_player_position, player_position) > 32 && bitmap_point_collision(traversable_map, 0, 0, player_point.x, player_point.y) && player_current_action != DASH) {
        particle dust_particle("Dust", player_position.x + (rnd() * 2 - 1) * 4, player_position.y - 48 + (rnd() * 2 - 1) * 4, 14);
        back_particles.push_back(dust_particle);
      }
      previous_player_position = player_position;
    }

    if (!game_pause && !new_stage) {
      if (spawn_enemy > spawn_rate) {
        spawn_enemy = 0;
        int spawn_amount = rnd(1, 3);
        for (int i = 0; i < spawn_amount; i++) {
          position random_position = {(double)rnd(-200, 6000), (double)rnd(-2200, 1000)};
          switch (rnd(1, 4)) {
            case 1:
              random_position = on_map_positions[rnd(0, on_map_positions.size())];
              enemies.push_back(make_unique<wraith>(random_position.x, random_position.y));
              break;
            case 2:
              enemies.push_back(make_unique<mage>(random_position.x, random_position.y, rnd(1, 3)));
              break;
            case 3:
              enemies.push_back(make_unique<drone>(random_position.x, random_position.y));
              break;
          }
        }
      }
      else {
        spawn_enemy = spawn_enemy + delta_time;
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

    if (invincibility_frames > 0) {
      invincibility_frames = invincibility_frames - 1;
      if ((int)elasped_time % 6 == 0 || (int)elasped_time % 6 == 1 || (int)elasped_time % 6 == 2) {
        draw_bitmap_with_animation(elasped_time, player_animation, player_position.x, SCREEN_SIZE[1] - player_position.y, 6);
      }
    }
    else {
      draw_bitmap_with_animation(elasped_time, player_animation, player_position.x, SCREEN_SIZE[1] - player_position.y, 6);
    }

    // For each of the enemies...
    for (int i = 0; i < enemies.size(); i++) {
      // Get the player point that will be used to traversal
      position player_point_traversal = {player_position.x + 32, player_position.y - 104};
      // If the enemy's HP is 0...
      if (enemies[i]->hp == 0) {
        // ...Play the kill sound effect
        play_sound_effect("Kill", settings.sound_volume);
        // Generate particles, which are smoke, to indicate a kill
        for (int j = 0; j < 5; j++) {
          particle kill_particle("Kill", enemies[i]->current_position.x + (rnd() * 2 - 1) * 128, enemies[i]->current_position.y + (rnd() * 2 - 1) * 128, 14);
          front_particles.push_back(kill_particle);
        }
        // Increaes the count of enemies killed
        enemies_killed = enemies_killed + 1;
        // Remove the enemy from the vector
        enemies.erase(enemies.begin() + i);
      }

      // Casts the enemy pointer to a wraith pointer
      wraith* wraith_ptr = dynamic_cast<wraith*>(enemies[i].get());
      if (wraith_ptr) {
        if (!bitmap_point_collision(traversable_map, 0, 0, enemies[i]->current_position.x, SCREEN_SIZE[1] - enemies[i]->current_position.y)) {
          enemies.erase(enemies.begin() + i);
        }
        draw_bitmap_with_animation(elasped_time, enemies[i]->animation, enemies[i]->current_position.x - 80, SCREEN_SIZE[1] - enemies[i]->current_position.y - 128, 4);
        if (!game_pause) {
          if (wraith_ptr->pattern() == 1) {
            enemies[i]->animation = "WraithRun";
            if (bitmap_point_collision(traversable_map, 0, 0, player_point.x, player_point.y)) {
              vector<position> path = perform_modified_astar(traversable_map, enemies[i]->current_position, player_point_traversal, 32);
              path.erase(path.begin());
              float enemy_angle = 180 - get_angle(SCREEN_SIZE[1] - enemies[i]->current_position.y - (SCREEN_SIZE[1] - path[0].y), enemies[i]->current_position.x - path[0].x);
              enemies[i]->move(traversable_map, enemy_angle);
              if (calculate_distance(enemies[i]->current_position, player_point_traversal) < 64) {
                wraith_ptr->next_pattern();
                wraith_ptr->attack_action = 132;
              }
            }
          }
          else if (wraith_ptr->pattern() == 2) {
            enemies[i]->animation = "WraithAttack";
            wraith_ptr->attack_action = wraith_ptr->attack_action - 1;
            particle wraith_melee_particle("WraithSigil", enemies[i]->current_position.x - 64, enemies[i]->current_position.y + 96, 30);
            back_particles.push_back(wraith_melee_particle);
            if (calculate_distance((SCREEN_SIZE[1] - enemies[i]->current_position.y) - player_point.y, enemies[i]->current_position.x  - player_point.x) < 96) {
              take_damage_player(15);
            }
            if (wraith_ptr->attack_action <= 0) {
              wraith_ptr->next_pattern();
            }
          }
          int enemy_direction = (int)round((angle_in_360(180 - get_angle(SCREEN_SIZE[1] - enemies[i]->current_position.y - player_point.y, enemies[i]->current_position.x - player_point.x)) + 180) / 180);
          switch (enemy_direction) {
            case 1: case 3: enemies[i]->animation = enemies[i]->animation + "Right"; break;
            case 2: enemies[i]->animation = enemies[i]->animation + "Left"; break;
          }
        }
        continue;
      }

      // Casts the enemy pointer to a mage pointer
      mage* mage_ptr = dynamic_cast<mage*>(enemies[i].get());
      if (mage_ptr) {
        draw_bitmap_with_animation(elasped_time, enemies[i]->animation, enemies[i]->current_position.x - 64, SCREEN_SIZE[1] - enemies[i]->current_position.y - 96, 4);
        if (!game_pause) {
          if (calculate_distance(enemies[i]->current_position, player_point_traversal) > 256) {
            enemies[i]->move(traversable_map, 180 - get_angle(SCREEN_SIZE[1] - enemies[i]->current_position.y - player_point.y, enemies[i]->current_position.x - player_point.x));
          }
          if (mage_ptr->ready_to_attack()) {
            if (rnd(1, 5) == 1) {
              for (int j = 0; j < 8; j++) {
                projectile fireball(ENEMY, enemies[i]->current_position.x, enemies[i]->current_position.y, "MageProjectile", j * 45 + (rnd() * 2 - 1) * 8, 10, 3, 10, elasped_time);
                projectiles.push_back(fireball);
              }
            }
            else {
              for (int j = 0; j < 4; j++) {
                if (mage_ptr->type == 1) {
                  projectile fireball(ENEMY, enemies[i]->current_position.x, enemies[i]->current_position.y, "MageProjectile", j * 90 + (rnd() * 2 - 1) * 8, 10, 3, 10, elasped_time);
                  projectiles.push_back(fireball);
                }
                else if (mage_ptr->type == 2) {
                  projectile fireball(ENEMY, enemies[i]->current_position.x, enemies[i]->current_position.y, "MageProjectile", j * 90 + 45 + (rnd() * 2 - 1) * 8, 10, 3, 10, elasped_time);
                  projectiles.push_back(fireball);
                }
              }
            }

          }
        }
        continue;
      }

      // Casts the enemy pointer to a drone pointer
      drone* drone_ptr = dynamic_cast<drone*>(enemies[i].get());
      if (drone_ptr) {
        draw_bitmap_with_animation(elasped_time, enemies[i]->animation, enemies[i]->current_position.x - 64, SCREEN_SIZE[1] - enemies[i]->current_position.y - 96, 4);
        if (!game_pause) {
          enemies[i]->move(traversable_map, 180 - get_angle(SCREEN_SIZE[1] - enemies[i]->current_position.y - (player_point.y + (rnd() * 2 - 1) * 32), enemies[i]->current_position.x - (player_point.x + (rnd() * 2 - 1) * 32)));
          if (calculate_distance(enemies[i]->current_position, player_point_traversal) < 32) {
            take_damage_player(30);
            for (int j = 0; j < 10; j++) {
              particle kill_particle("Kill", enemies[i]->current_position.x + (rnd() * 2 - 1) * 256, enemies[i]->current_position.y + (rnd() * 2 - 1) * 256, 14);
              front_particles.push_back(kill_particle);
            }
            enemies.erase(enemies.begin() + i);
          }
        }
        continue;
      }
    }

    // For each of the projectiles...
    for (int i = 0; i < projectiles.size(); i++) {
      // ...Draw the projectile
      draw_bitmap_with_animation(elasped_time, projectiles[i].animation, projectiles[i].position.x, SCREEN_SIZE[1] - projectiles[i].position.y, projectiles[i].animation_speed, projectiles[i].offset());
      // If the game is not paused....
      if (!game_pause) {
        // ...Move the projectile
        projectiles[i].move();
        // If the projectile is out of range, delete the projectile
        if (projectiles[i].out_of_range()) {
          projectiles.erase(projectiles.begin() + i);
        }
      }
    }

    // For each of the projectiles and projectiles...
    for (int i = 0; i < enemies.size(); i++) {
      for (int j = 0; j < projectiles.size(); j++) {
        // If the projectile is a player projectile and it is within range of an enemy...
        if (projectiles[j].type == PLAYER && calculate_distance((SCREEN_SIZE[1] - (projectiles[j].position.y - 32)) - (SCREEN_SIZE[1] - enemies[i]->current_position.y), (projectiles[j].position.x + 32) - enemies[i]->current_position.x) < 96) {
          // Damage that enemy
          enemies[i]->damage(projectiles[j].damage);
          // Create a particle for the hit
          particle hit_particle("Hit", projectiles[j].position.x, projectiles[j].position.y, 13);
          front_particles.push_back(hit_particle);
          // Delete the projectile
          projectiles.erase(projectiles.begin() + j);
        }
        // If the projectile is a enemy projectile and it is within range of the player...
        if (projectiles[j].type == ENEMY && calculate_distance((SCREEN_SIZE[1] - (projectiles[j].position.y - 16)) - player_point.y + 32, (projectiles[j].position.x + 16) - player_point.x) < 64 && invincibility_frames == 0) {
          // Damage the player
          take_damage_player(projectiles[j].damage);
          // Delete the projectile
          projectiles.erase(projectiles.begin() + j);
        }
      }
    }

    for (int i = 0; i < front_particles.size(); i++) {
      draw_bitmap_with_animation(elasped_time, front_particles[i].animation, front_particles[i].position.x, SCREEN_SIZE[1] - front_particles[i].position.y, 1);
      if (!game_pause) {
        if (front_particles[i].finished()) {
          front_particles.erase(front_particles.begin() + i);
        }
      }
    }

    // The player's power increase in beat as long as the game is not pasued or the player is not shooting or casting
    if (bounce(elasped_time, 1, 150) > 0.5 && player_current_action != SHOOT && player_current_action != CAST && !game_pause) {
      player_power = player_power + 8;
      if (player_power > 100) {
        player_power = 100;
      }
    }

    // If the amount of enemies that has been killed is equal or greater than the number of enemies to pass the stage and the player is currently not doing anything...
    if (enemies_killed >= current_stage.enemies_to_pass && player_current_action == NOTHING && current_stage.enemies_to_pass != -1) {
      // Reset the enemies killed counter
      enemies_killed = 0;
      // Get rid of all enemies that are currently still present
      enemies.clear();
      play_sound_effect("Impact", settings.sound_volume);
      // Shake screen and trigger new stage
      shake_amount = 1000;
      new_stage = true;
      // Stops the music
      stop_music();
    }

    // If a new stage is not happening, the HP bar, power bar and heart icon is to change based on the beat
    if (!new_stage) {
      color hp_bar = rgba_color(255, 195, 210, 180 + (int)bounce(elasped_time - beat_offset, 25, 2));
      fill_rectangle(hp_bar, stick_to_screen_x(25), stick_to_screen_y(25), player_health * 4.5, 32);
      draw_bitmap(health_icon, stick_to_screen_x(25), stick_to_screen_y(25));
      color power_bar = rgba_color(192, 178, 209, 180 + (int)bounce(elasped_time - beat_offset, 25, 2));
      fill_rectangle(power_bar, stick_to_screen_x(25), stick_to_screen_y(65), player_power * 4.5, 16);
      draw_bitmap(power_icon, stick_to_screen_x(25), stick_to_screen_y(65));
      draw_bitmap(music_heart, stick_to_screen_x(5), stick_to_screen_y(75 - bounce(elasped_time - beat_offset, 4, 300)));
    }
    // If a new stage is being announced, the HP bar, power bar and heart icon is to display, but not change
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
      // If there are no more stages to load, end the game
      if (stages.size() == 0) {
        game_running = false;
        return 0;
      }
      // Gets the top of the queue for the next stage
      stage_data next_stage = stages.front();
      // If it is currently preparing a stage, display the text to announce a new stage
      if (preparing_stage > 0) {
        draw_text_on_window(main_window, next_stage.name, color_white(), main_font, 64, stick_to_screen_x(SCREEN_SIZE[0] / 2 - 96), stick_to_screen_y(SCREEN_SIZE[1] / 2 - 24));
        preparing_stage = preparing_stage - delta_time;
      }
      // When it is time to start the stage...
      else {
        // Starts preparing the stage
        prepare_stage(preparing_stage);
        current_stage = next_stage;
        // Dequeue a stage
        stages.pop();
        begin_stage(current_stage);
        beat_offset = elasped_time;
        new_stage = false;
        // The spawn rate is increased (by decreasing the value)
        spawn_rate = spawn_rate - 60;
        /*
        NOTE: This is where the boss would be spawned but as of submission, there was not enough time
        if (current_stage.boss != "") {
          if (current_stage.boss == "Fear") {
            current_boss = make_unique<fear>(spawn_point.x, spawn_point.y);
          }
        }
        */
      }
    }
    // Processes the pause menu and draws the cursor
    process_pause_menu();
    draw_cursor();

    // While the game is not paused, the elasped time is to go up
    if (!game_pause) {
      elasped_time = elasped_time + delta_time;
    }

    // If the player's health is less than 0, the player's health is set to 0
    if (player_health < 0) {
      player_health = 0;
    }
    // If the player's health is equal to 0, sets booleans to cause game lose condition
    if (player_health == 0) {
      game_running = false;
      game_lose = true;
    }

    // If the music is not longer playing and it is not a new stage, whilst the game is also not paused...
    if (!music_playing() && !new_stage && !game_pause) {
      // Counts the time of how long there has been no music
      no_music_playing = no_music_playing + delta_time;
      // If the music has not been played for around 1.5 seconds...
      if (no_music_playing > 90) {
        // ...Sets booleans to cause a game lose condition
        game_running = false;
        game_lose = true;
      }
    }
    // If not...
    else {
      // ...The counter is reset
      no_music_playing = 0;
    }

    // Gets the current FPS as a integer
    int current_fps = (int)round(60 / delta_time);
    // Draws it as text on in the bottom left of the screen
    draw_text_on_window(main_window, to_string(current_fps), color_white(), main_font, 16, stick_to_screen_x(8), stick_to_screen_y(SCREEN_SIZE[1] - 20));

    // Refreshes the screen using the target framerate and processes the event
    refresh_screen(TARGET_FRAMERATE);
    process_events();
  }

  while (game_lose) {
    // NOTE: A lose screen is to be added here, unfortunately, due to time constraints, it was not added.
    write_line("You lose!");
    break;
  }

  return 0;
}
