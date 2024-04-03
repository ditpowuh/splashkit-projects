// Import the SplashKit SDK and System
using SplashKitSDK;
using System;

// Initialise a instance of the Random class
Random rnd = new Random();

// Sets the screen size as an array of integers
int[] screenSize = [1280, 720];

// Sets the speed of the player
double playerSpeed = 5;
//
double[] playerPosition = [screenSize[0] / 2, screenSize[1] / 2];

double[] appliedMovement = [0, 0];
// Gets the value of the square root of 2 divided by 2 (this is used to normalise the player movement)
double diagonalMovement = Math.Sqrt(2) / 2;

const string GitHubRaw = "https://raw.githubusercontent.com/ditpowuh/splashkit-projects/main/collection_game/";

// Gets all the sounds
SoundEffect collectGemSound = SetSoundEffect("collectGemSound", "Sounds/CollectGem.wav");
SoundEffect loseSound = SetSoundEffect("loseSound", "Sounds/Lose.wav");
SoundEffect moveGemsSound = SetSoundEffect("moveGemsSound", "Sounds/MoveGems.wav");
SoundEffect winSound = SetSoundEffect("winSound", "Sounds/WinTune.wav");

// Gets the main bitmap for player
Bitmap player = SetBitmap("Player", "Sprites/Computer.png");

// Gets the bitmap for player when won
Bitmap playerWin = SetBitmap("PlayerWin", "Sprites/ComputerWin.png");

// Gets bitmap for gems and puts in an array of Bitmaps
Bitmap[] gemSprites = [
  SetBitmap("RedGem", "Sprites/RedGem.png"),
  SetBitmap("GreenGem", "Sprites/GreenGem.png"),
  SetBitmap("BlueGem", "Sprites/BlueGem.png")
];

// Gets bitmap for the ghost and initalises the ghost's positon in an integer array that has two random integers that are between 0 and the window resolution
Bitmap ghost = SetBitmap("Ghost", "Sprites/Ghost.png");
int[] ghostPosition = [rnd.Next(0, screenSize[0]), rnd.Next(0, screenSize[1])];

// Gets bitmap for the text to show when you either win or lose
Bitmap winText = SetBitmap("winText", "Sprites/WinText.png");
Bitmap loseText = SetBitmap("loseText", "Sprites/LoseText.png");

// Sets a variable for sin values (uses a float for that little bit of peformance)
float sinValue = 0;

// Sets an array of arrays that initalises the positions of the gems
int[][] gemPositions = [
  [0, 0],
  [0, 0],
  [0, 0]
];

// Creates a list of integers for the collected gems
List<int> gemsCollected = new List<int>();
// Initalises the order of the gems, being their index
int[] order = [0, 1, 2];
// Shuffles the array 'order' so that it is random
Random.Shared.Shuffle(order);

// Sets the target framerate (a constant so that is read-only)
const int targetFramerate = 60;
// Opens the window with the name and screen size
SplashKit.OpenWindow("Collection Game", screenSize[0], screenSize[1]);

// Sets different booleans for each condition of the game
bool gameQuit = false;
bool gameLose = false;
bool gameWin = false;

// Functions

// Function that tries to get the bitmap as a local path, but if it fails to (i.e. no bitmap is found at local path), it'll try to download the bitmap from the GitHub version of my project
Bitmap SetBitmap(string name, string localPath = "") {
  Bitmap? bitmap = SplashKit.LoadBitmap(name, localPath);
  bitmap ??= SplashKit.DownloadBitmap(name, GitHubRaw + localPath, 433);
  return bitmap;
}

// Function that tries to get the sound effect as a local path, but if it fails to (i.e. no sound effect is found at local path), it'll try to download the sound effect from the GitHub version of my project
SoundEffect SetSoundEffect(string name, string localPath = "") {
  SoundEffect? soundEffect = SplashKit.LoadSoundEffect(name, localPath);
  soundEffect ??= SplashKit.DownloadSoundEffect(name, GitHubRaw + localPath, 433);
  return soundEffect;
}

// Function that randomises the positions of the gems - plays sound by default
void RegenerateGems(bool playSound = true) {
  // Iterates through each of the gems
  for (int i = 0; i < 3; i++) {
    gemPositions[i] = [rnd.Next(0, screenSize[0]), rnd.Next(0, screenSize[1])];
  }
  if (playSound) {
    moveGemsSound.Play();
  }
}

// Function that returns an integer for sinwave movement
int BounceEffect(int offset = 0) {
  return (int)(Math.Sin(sinValue / 10f + offset * 5f) * 2f);
}

// Function that renders the gems and checks the collision
void RenderGemsAndCheckCollision() {
  // If the player loses, the gems will not be rendered and their collision will not be checked
  if (gameLose) {
    return;
  }
  // Iterates through the 3 gems
  for (int i = 0; i < 3; i++) {
    // If the gem has not been collected, it will be rendered
    if (!gemsCollected.Contains(i)) {
      SplashKit.DrawBitmap(gemSprites[i], gemPositions[i][0] - 32, screenSize[1] - gemPositions[i][1] - 32 + BounceEffect(i));
    }
    // Checks if the player is colliding with the gem...
    if (player.BitmapCollision(playerPosition[0] - 32, screenSize[1] - playerPosition[1] - 32, gemSprites[i], gemPositions[i][0] - 32, screenSize[1] - gemPositions[i][1] - 32)) {
      // If the gem has not been collected...
      if (!gemsCollected.Contains(i)) {
        // ...and it is in the correct order...
        if (order[gemsCollected.Count] == i) {
          // The gem will be added to the list of collected gems
          gemsCollected.Add(i);
          // A sound will play when the gem is collected
          collectGemSound.Play();
        }
        // If the gem collected is not the correct next one, it will regenerate the gems
        else {
          RegenerateGems();
        }
      }
    }
  }
}

// Runs the regenerate gems to randomise the positions of the gems, whilst making no sound
RegenerateGems(false);

// Game loop, where if the gameQuit is true or if it is requested, the loop will end
while (!SplashKit.QuitRequested() && !gameQuit) {
  if (SplashKit.KeyTyped(KeyCode.EscapeKey)) {
    gameQuit = true;
  }
  appliedMovement = [0, 0];
  // Applying movement for both W, A, S and D, and arrow keys
  if (SplashKit.KeyDown(KeyCode.DKey) || SplashKit.KeyDown(KeyCode.RightKey)) {
    appliedMovement[0] = 1;
  }
  if (SplashKit.KeyDown(KeyCode.AKey) || SplashKit.KeyDown(KeyCode.LeftKey)) {
    appliedMovement[0] = -1;
  }
  if (SplashKit.KeyDown(KeyCode.WKey) || SplashKit.KeyDown(KeyCode.UpKey)) {
    appliedMovement[1] = 1;
  }
  if (SplashKit.KeyDown(KeyCode.SKey) || SplashKit.KeyDown(KeyCode.DownKey)) {
    appliedMovement[1] = -1;
  }
  // If the movement is diagonal, it is normalised so that the speed is constant (1 pixel multiplied by playerSpeed, per frame)
  if (appliedMovement[0] != 0 && appliedMovement[1] != 0) {
    appliedMovement = [appliedMovement[0] * diagonalMovement, appliedMovement[1] * diagonalMovement];
  }
  // Applies the movement by changing the player position
  playerPosition[0] = playerPosition[0] + playerSpeed * appliedMovement[0];
  playerPosition[1] = playerPosition[1] + playerSpeed * appliedMovement[1];

  // If the player moves out of bounds from the left or right, it will move the player to the other side
  if (playerPosition[0] < -16) {
    playerPosition[0] = screenSize[0] + 16;
  }
  if (playerPosition[0] > screenSize[0] + 16) {
    playerPosition[0] = -16;
  }

  // If the player moves out of bounds from the top or bottom, it will move the player to the other side
  if (playerPosition[1] < -16) {
    playerPosition[1] = screenSize[1] + 16;
  }
  if (playerPosition[1] > screenSize[1] + 16) {
    playerPosition[1] = -16;
  }

  // Clears the screen with the colour white
  SplashKit.ClearScreen(SplashKit.ColorWhite());
  // Draws the player, with different sprites whether the player has won or not yet
  SplashKit.DrawBitmap(gameWin ? playerWin : player, playerPosition[0] - 32, screenSize[1] - playerPosition[1] - 32 + BounceEffect(3));

  // Renders gem and checks collision by running that function
  RenderGemsAndCheckCollision();

  // Increases the sin value variable for sin movement
  sinValue = sinValue + 1f;

  // Draws the ghost
  SplashKit.DrawBitmap(ghost, ghostPosition[0] - 32, screenSize[1] - ghostPosition[1] - 32 + BounceEffect(4));
  // Calculates the smooth movement of the ghost using sin
  int ghostMovement = (int)Math.Abs((Math.Sin(sinValue / 25) * 3));

  // If the game has not been won or lost yet...
  if (!gameWin && !gameLose) {
    // ...move the ghost based on the player position (8 possible direction achieved with this)
    if (ghostPosition[0] > playerPosition[0]) {
      ghostPosition[0] = ghostPosition[0] - ghostMovement;
    }
    else if (ghostPosition[0] < playerPosition[0]) {
      ghostPosition[0] = ghostPosition[0] + ghostMovement;
    }
    if (ghostPosition[1] > playerPosition[1]) {
      ghostPosition[1] = ghostPosition[1] - ghostMovement;
    }
    else if (ghostPosition[1] < playerPosition[1]) {
      ghostPosition[1] = ghostPosition[1] + ghostMovement;
    }
  }

  // When the player collides with the ghost and the game has not been won nor lost, the game will be considered lost and the lose sound effect will play
  if (player.BitmapCollision(playerPosition[0] - 32, screenSize[1] - playerPosition[1] - 32, ghost, ghostPosition[0] - 32, screenSize[1] - ghostPosition[1] - 32) && !gameWin && !gameLose) {
    gameLose = true;
    loseSound.Play();
  }
  // When all 3 gems have been collected and the game has not been considered won, the game will be considered won and the win sound effect will play
  if (gemsCollected.Count == 3 && !gameWin) {
    gameWin = true;
    winSound.Play();
  }

  // When the player wins, draw the win text
  if (gameWin) {
    SplashKit.DrawBitmap(winText, 100, screenSize[1] - 680 + BounceEffect(5));
  }
  // When the player loses, draw the win text
  if (gameLose) {
    SplashKit.DrawBitmap(loseText, 100, screenSize[1] - 680 + BounceEffect(5));
  }

  // Refreshes the screen with the target framerate and processes events
  SplashKit.RefreshScreen(targetFramerate);
  SplashKit.ProcessEvents();
}
