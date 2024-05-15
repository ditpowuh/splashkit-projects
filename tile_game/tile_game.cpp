// Using SplashKit and to_string from the standard library
#include "splashkit.h"
using std::to_string;

// Sets the screen size as a constant array of integers
const int SCREEN_SIZE[] = {1280, 720};
// Sets the target framerate as a constant integer
const int TARGET_FRAMERATE = 60;

// Sets the size of the tiles as an int (this is in pixels)
const int TILE_SIZE = 40;
// Creates an enumeration for the different types of tiles
enum tile {
  GRASS,
  WATER,
  DESERT
};
// Gets the map width and height by seeing how many tiles can be fit into the window
const int MAP_WIDTH = SCREEN_SIZE[0] / TILE_SIZE;
const int MAP_HEIGHT = SCREEN_SIZE[1] / TILE_SIZE;

// Creates a struct to represent positions
struct position {
  int x;
  int y;
};

// Creates a struct for coins that are placed
struct coin {
  position position;
  bool collected;
};

// Creates the tilemap, which is an array of arrays of tiles
tile tilemap[MAP_HEIGHT][MAP_WIDTH];
// Initialises the player position
position player_position = {0, 0};
// Initialises the coins that are placed
coin placed_coins[20];

// Initialises the count of coins that have been collected
int collected_coins = 0;

// Sets the parsed position to a valid random place on the map - uses a reference to directly change the position
void place_randomly(position &position_to_place) {
  // Sets the position to a random location on the map
  position_to_place = {rnd(0, MAP_WIDTH), rnd(0, MAP_HEIGHT)};
  // Continues to do so until the tile that has been randomly selected is not water
  while (tilemap[position_to_place.y][position_to_place.x] == WATER) {
    position_to_place = {rnd(0, MAP_WIDTH), rnd(0, MAP_HEIGHT)};
  }
}

// Generates the map by setting a type of tile for each of the tiles - uses a reference to directly change the parsed array of arrays
void generate_map(tile (&tm)[MAP_HEIGHT][MAP_WIDTH]) {
  // Repeats for each of the rows of the tilemap
  for (int h = 0; h < MAP_HEIGHT; h++) {
    // Repeats for each of the columns within the rows of the tilemap
    for (int w = 0; w < MAP_WIDTH; w++) {
      // Gets a random integer that is from 1 to 5 (inclusive)
      int random_number = rnd(1, 6);
      // Sets the tile based on the random integer
      switch (random_number) {
        // If the integer is 1 or 2, grass is selected
        case 1:
        case 2:
          tm[h][w] = GRASS;
          break;
        // If the integer is 3, water is selected
        case 3:
          tm[h][w] = WATER;
          break;
        // If the integer is 4 or 5, desert is selected
        case 4:
        case 5:
          tm[h][w] = DESERT;
          break;
      }
    }
  }
}

// Sets a random position for all the placed coins
void generate_placed_coins() {
  for (int i = 0; i < 20; i++) {
    place_randomly(placed_coins[i].position);
  }
}

// Peforms checking the tile after being traversed
void tile_chance(position current_position) {
  // Checks the tile of the parsed position
  switch (tilemap[current_position.y][current_position.x]) {
    // If the tile is grass, it has 10% of a chance to gain a coin
    case GRASS:
      if (rnd() <= 0.1) {
        collected_coins++;
      }
      break;
    // If the tile is water, which shouldn't be possible anyways, nothing will happen
    case WATER:
      break;
    // If the tile is desert, it has 5% of a chance to lose a coin (if the player actually has at least a coin)
    case DESERT:
      if (rnd() <= 0.05) {
        if (collected_coins > 0) {
          collected_coins--;
        }
      }
      break;
  }
}

// Processes the player's movement controls
void process_player_movement() {
  // If the W key is pressed...
  if (key_typed(W_KEY)) {
    // If the tile above is water or out of range, the player will be stopped and won't move.
    if (tilemap[player_position.y - 1][player_position.x] == WATER || player_position.y == 0) {
      return;
    }
    // The player is moved up
    player_position.y = player_position.y - 1;
    // The player's position is checked and processed for the random chance of earning or losing a coin
    tile_chance(player_position);
  }
  // If the S key is pressed...
  if (key_typed(S_KEY)) {
    // If the tile below is water or out of range, the player will be stopped and won't move.
    if (tilemap[player_position.y + 1][player_position.x] == WATER || player_position.y == MAP_HEIGHT - 1) {
      return;
    }
    // The player is moved down
    player_position.y = player_position.y + 1;
    // The player's position is checked and processed for the random chance of earning or losing a coin
    tile_chance(player_position);
  }
  // If the D key is pressed...
  if (key_typed(D_KEY)) {
    // If the tile to the right is water or out of range, the player will be stopped and won't move.
    if (tilemap[player_position.y][player_position.x + 1] == WATER || player_position.x == MAP_WIDTH - 1) {
      return;
    }
    // The player is moved right
    player_position.x = player_position.x + 1;
    // The player's position is checked and processed for the random chance of earning or losing a coin
    tile_chance(player_position);
  }
  // If the A key is pressed...
  if (key_typed(A_KEY)) {
    // If the tile to the left is water or out of range, the player will be stopped and won't move.
    if (tilemap[player_position.y][player_position.x - 1] == WATER || player_position.x == 0) {
      return;
    }
    // The player is moved left
    player_position.x = player_position.x - 1;
    // The player's position is checked and processed for the random chance of earning or losing a coin
    tile_chance(player_position);
  }
}

int main() {
  // A new window is made with the title and window size
  window main_window = open_window("Tile RPG", SCREEN_SIZE[0], SCREEN_SIZE[1]);
  // The border is toggled off for the window
  window_toggle_border(main_window);

  // A font is loaded
  font main_font = load_font("Monogram", "monogram.ttf");

  // Sets a bool for the main game loop
  bool game_running = true;

  // Runs the necessary procedures to start the game, such as the tilemap, the coins and the player's intial position
  generate_map(tilemap);
  generate_placed_coins();
  place_randomly(player_position);

  while (game_running && !quit_requested()) {
    // If the escape key is pressed, the game will be stopped
    if (key_typed(ESCAPE_KEY)) {
      game_running = false;
    }
    // If F11 is pressed, the fullscreen will be toggled for the window
    if (key_typed(F11_KEY)) {
      window_toggle_fullscreen(main_window);
    }
    // Processes the player's movement before anything else
    process_player_movement();
    // For each of the placed coins, if the player's position is equal to the coin's position, and the coin has not been collected yet...
    for (int i = 0; i < 20; i++) {
      if (placed_coins[i].position.x == player_position.x && placed_coins[i].position.y == player_position.y) {
        if (!placed_coins[i].collected) {
          // ...the coin will be marked as collected and the number of collected coins will be incremented
          placed_coins[i].collected = true;
          collected_coins++;
        }
      }
    }

    // For each of the tiles in the tilemap...
    for (int h = 0; h < MAP_HEIGHT; h++) {
      for (int w = 0; w < MAP_WIDTH; w++) {
        // Sets the tile colour based on what the tile is
        color tile_colour;
        switch (tilemap[h][w]) {
          // If the tile is grass, the tile will be green
          case GRASS:
            tile_colour = color_green();
            break;
          // If the tile is water, the tile will be blue
          case WATER:
            tile_colour = color_blue();
            break;
          // If the tile is desert, the tile will be yellow
          case DESERT:
            tile_colour = color_yellow();
            break;
        }
        // Fills the rectangle with the tile position based on the size of the tile in pixels
        fill_rectangle(tile_colour, w * TILE_SIZE, h * TILE_SIZE, TILE_SIZE, TILE_SIZE);
      }
    }

    // For each of the coins...
    for (int i = 0; i < 20; i++) {
      // ...if the coin has not been collected...
      if (!placed_coins[i].collected) {
        // ...fills a circle for it with the colour orange, at the centre of the tile of its designated position
        fill_circle(color_orange(), placed_coins[i].position.x * TILE_SIZE + TILE_SIZE / 2, placed_coins[i].position.y * TILE_SIZE + TILE_SIZE / 2, 8);
      }
    }

    // If 15 or more coins have been collected, the game is stopped as the player has won
    if (collected_coins >= 15) {
      write_line("You won!");
      game_running = false;
    }

    // Draws the player as a red dot, placed at the centre of the tile of its position
    fill_circle(color_red(), player_position.x * TILE_SIZE + TILE_SIZE / 2, player_position.y * TILE_SIZE + TILE_SIZE / 2, 16);
    // Draws text on the window at the top left, with the number of coins that have been collected
    draw_text_on_window(main_window, to_string(collected_coins), color_red(), main_font, 64, 16, 8);

    // Refreshes with the target framrate and processes the events
    refresh_screen(TARGET_FRAMERATE);
    process_events();
  }

  return 0;
}
