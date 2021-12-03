#define VERSION "0.1.1999"
#define PROGRAM_NAME "aawordsearch"
// n * n grid
const int GRID_SIZE = 20;       // n
const int MAX_LEN = GRID_SIZE - 2;

const char HOST[] = "random-word-api.herokuapp.com";
const char PAGE[] = "word";
const char PROTOCOL[] = "http";

const char fill_char = '-';

typedef struct dir_op
{
  int begin_row;
  int begin_col;
  const int row;
  const int col;
  void (*direction)(struct dir_op *dir_op, const int len);
} dir_op;
