/*
 * wordsearch.c
 * 
 * Copyright 2021 Andy Alt <andy400-dev@yahoo.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// n * n grid
const int GRID_SIZE = 20; // n


bool check (const int row, const int col, const char puzzle[][GRID_SIZE], const char c)
{
  const int u = toupper (c);
  if (u == puzzle[row][col] || puzzle[row][col] == '_')
    return true;

  return false;
}

void place (const int row, const int col, char puzzle[][GRID_SIZE], const char c)
{
  puzzle[row][col] = toupper(c);
  return;
}

int horizontal (const int len, const char *str, char puzzle[][GRID_SIZE])
{
  int col = rand() % (GRID_SIZE - len);
  const int row = rand() % GRID_SIZE;
  const int col_orig = col;
  char *ptr = (char *)str;
  while (*ptr != '\0')
  {
    if (check (row, col, puzzle, *ptr) == false)
      return -1;
    
    ptr++;
    col++;
  }
  col = col_orig;
  ptr = (char *)str;
  while (*ptr != '\0')
  {
    place (row, col, puzzle, *ptr);
    ptr++;
    col++;
  }

  return 0;
}

int horizontal_backward (const int len, const char *str, char puzzle[][GRID_SIZE])
{
  int col = (rand() % (GRID_SIZE - len)) + len ;
  const int row = rand () % GRID_SIZE;
  const int col_orig = col;
  char *ptr = (char *)str;
  while (*ptr != '\0')
  {
    if (check (row, col, puzzle, *ptr) == false)
      return -1;
    ptr++;
    col--;
  }

  col = col_orig;
  ptr = (char *)str;
  while (*ptr != '\0')
  {
    place (row, col, puzzle, *ptr);
    ptr++;
    col--;
  }
  return 0;
}

int vertical (const int len,  const char *str, char puzzle[][GRID_SIZE])
{
  int row = rand() % (GRID_SIZE - len);
  const int col = rand () % GRID_SIZE;
  const int row_orig = row;
  char *ptr = (char *)str;
  while (*ptr != '\0')
  {
    if (check (row, col, puzzle, *ptr) == false)
      return -1;
    ptr++;
    row++;
  }

  row = row_orig;
  ptr = (char *)str;
  while (*ptr != '\0')
  {
    place (row, col, puzzle, *ptr);
    ptr++;
    row++;
  }
  return 0;
}

int vertical_up (const int len, const char *str, char puzzle[][GRID_SIZE])
{
  int row = (rand() % (GRID_SIZE - len)) + len;
  const int col = rand () % GRID_SIZE;
  const int row_orig = row;
  char *ptr = (char *)str;
  while (*ptr != '\0')
  {
    if (check (row, col, puzzle, *ptr) == false)
      return -1;
    ptr++;
    row--;
  }

  row = row_orig;
  ptr = (char *)str;
  while (*ptr != '\0')
  {
    place (row, col, puzzle, *ptr);
    ptr++;
    row--;
  }
  return 0;
}


int diaganol_down_right (const int len, const char *str, char puzzle[][GRID_SIZE])
{
  int row = (rand() % (GRID_SIZE - len));
  int col = (rand() % (GRID_SIZE - len));
  const int row_orig = row;
  const int col_orig = col;
  char *ptr = (char *)str;
  while (*ptr != '\0')
  {
    if (check (row, col, puzzle, *ptr) == false)
      return -1;
    ptr++;
    col++;
    row++;
  }

  col = col_orig;
  row = row_orig;
  ptr = (char *)str;
  while (*ptr != '\0')
  {
    place (row, col, puzzle, *ptr);
    ptr++;
    col++;
    row++;
  }
  return 0;
}

int diaganol_down_left (const int len, const char *str, char puzzle[][GRID_SIZE])
{
  int col = (rand() % (GRID_SIZE - len)) + len;
  int row = (rand() % (GRID_SIZE - len));
  const int row_orig = row;
  const int col_orig = col;
  char *ptr = (char *)str;
  while (*ptr != '\0')
  {
    if (check (row, col, puzzle, *ptr) == false)
      return -1;
    ptr++;
    col--;
    row++;
  }

  col = col_orig;
  row = row_orig;
  ptr = (char *)str;
  while (*ptr != '\0')
  {
    place (row, col, puzzle, *ptr);
    ptr++;
    col--;
    row++;
  }
  return 0;
}

int diaganol_up_left (const int len, const char *str, char puzzle[][GRID_SIZE])
{
  //int col = (rand() % (GRID_SIZE - len)) + len;
  int col = (rand() % (GRID_SIZE - len)) + len ;
  int row = (rand() % (GRID_SIZE - len)) + len;
  const int row_orig = row;
  const int col_orig = col;
  char *ptr = (char *)str;
  while (*ptr != '\0')
  {
    if (check (row, col, puzzle, *ptr) == false)
      return -1;
    ptr++;
    col--;
    row--;
  }

  col = col_orig;
  row = row_orig;
  ptr = (char *)str;
  while (*ptr != '\0')
  {
    place (row, col, puzzle, *ptr);
    ptr++;
    col--;
    row--;
  }
  return 0;
}

int diaganol_up_right (const int len, const char *str, char puzzle[][GRID_SIZE])
{
  int col = rand() % (GRID_SIZE - len);
  int row = (rand() % (GRID_SIZE - len)) + len;
  const int row_orig = row;
  const int col_orig = col;
  char *ptr = (char *)str;
  while (*ptr != '\0')
  {
    if (check (row, col, puzzle, *ptr) == false)
      return -1;
    ptr++;
    col++;
    row--;
  }

  col = col_orig;
  row = row_orig;
  ptr = (char *)str;
  while (*ptr != '\0')
  {
    place (row, col, puzzle, *ptr);
    ptr++;
    col++;
    row--;
  }
  return 0;
}


const char *strings[] = {
  "received",
  "software",
  "purpose",
  "version",
  "program",
  "foundation",
  "General",
  "Public",
  "License",
  "distributed",
  "California",
  "Mainland",
  "Honeymoon",
  "simpson",
  "incredible",
  "MERCHANTABILITYMERCHANTABILITY",
  "warranty",
  "temperate",
  "london",
  "tremendous",
  "desperate",
  "paradoxical",
  "starship",
  "enterprise",
  NULL
};

const size_t n_strings = sizeof(strings)/sizeof(strings[0]);

enum {
  HORIZONTAL,
  HORIZONTAL_BACKWARD,
  VERTICAL,
  VERTICAL_UP,
  DIAGANOL_DOWN_RIGHT,
  DIAGANOL_DOWN_LEFT,
  DIAGANOL_UP_RIGHT,
  DIAGANOL_UP_LEFT
};

const int directions = 8;

int get_rnd (const int n)
{
  return rand () % n;
}

//int get_boundary (const char *str)
//{
  //return GRID_SIZE - strlen (str);
//}

int main(int argc, char **argv)
{
  char puzzle[GRID_SIZE][GRID_SIZE];
  int i = 0;
  int j = 0;

  /* initialize the puzzle with NULLs */
  for (i = 0; i < GRID_SIZE; i++)
  {
    for (j = 0; j < GRID_SIZE; j++)
    {
      puzzle[i][j] = '_';
    }
  }

  // Create an array of function pointers
  int (*direction_arr[])(const int, const char*, char(*)[GRID_SIZE]) = {
    horizontal,
    horizontal_backward,
    vertical,
    vertical_up,
    diaganol_down_left,
    diaganol_down_right,
    diaganol_up_left,
    diaganol_up_right
  };
  /* seed the random number generator */
  srand (time (NULL));

  int n_string = 0;
  int tries = 0;
  while (strings[n_string] != NULL)
  {
    const int len = strlen (strings[n_string]);
    if (len > (GRID_SIZE - 2)) // skip the word if it exceeds this value
    {
      n_string++;
      continue;
    }
    
    const int max_tries = GRID_SIZE * 4;
    // The word will be skipped if a place can't be found
    for (tries = 0; tries < max_tries; tries++)
    {
      int rnd;
      // After n number of tries, try different directions.
      if (tries == 0 || tries > max_tries/2)
      {
        rnd = rand () % directions;
        // rnd = 7;
      }

      int r = direction_arr[rnd] (len, strings[n_string], puzzle);
      
      if (r == 0)
        break;
    }
    n_string++;
  }

  /* Fill in any unused spots with random letters */
  for (i = 0; i < GRID_SIZE; i++)
  {
    for (j = 0; j < GRID_SIZE; j++)
    {
      if (puzzle[i][j] == '_')
        puzzle[i][j] = (rand () % (90 - 65 + 1)) + 65;
    }
  }

  for (i = 0; i < GRID_SIZE; i++)
  {
    for (j = 0; j < GRID_SIZE; j++)
    {
      printf ("%c ", puzzle[i][j]);
    }
    puts ("");
  }

  i = 0;
  while (strings[i] != NULL)
  {
    printf ("%s\t", strings[i]);
    i++;
  }
  
  return 0;
}

