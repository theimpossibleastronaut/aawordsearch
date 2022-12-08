/*
 * wordsearch.c
 *
 * Copyright 2021-2022 Andy Alt <andy400-dev@yahoo.com>
 * https://github.com/theimpossibleastronaut/wordsearch
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
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <getopt.h>
#include <stdbool.h>
#include <limits.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>
#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

#ifndef VERSION
#define VERSION "_unversioned"
#endif

#ifndef PROGRAM_NAME
#define PROGRAM_NAME "aawordsearch"
#endif

struct lang_vars
{
  const char *lang;
  const char *locale;
  const wchar_t *alphabet;
  const size_t length;
};

#ifdef HAVE_CURL
struct memory {
 char *response;
 size_t size;
};

static size_t cb(void *data, size_t size, size_t nmemb, void *userp)
{
 size_t realsize = size * nmemb;
 struct memory *mem = (struct memory *)userp;

 char *ptr = realloc(mem->response, mem->size + realsize + 1);
 if(ptr == NULL)
 {
   fputs("Error allocating memory\n", stderr);
   exit(EXIT_FAILURE);
 }

 mem->response = ptr;
 memcpy(&(mem->response[mem->size]), data, realsize);
 mem->size += realsize;
 mem->response[mem->size] = 0;

 return realsize;
}
#endif

// n * n grid
const int GRID_SIZE = 20;       // n
#define MAX_LEN (GRID_SIZE - 2)
const int N_DIRECTIONS = 8;

const char *HOST[] = {
  "random-word-api.herokuapp.com",
  NULL
};

#ifdef HAVE_CURL
const char SERVICE[] = "https";
#else
const char SERVICE[] = "http";
#endif

const wchar_t fill_char = '-';

const wchar_t es_alphabet[] = L"ABCDEÉFGHIÍJKLMNÑOÓPQRSTUÜVWXYZ";
const wchar_t it_alphabet[] = L"ABCDEFGHILMNOPQRSTUVZ";
const wchar_t de_alphabet[] = L"AÄBCDEFGHIJKLMNOÖPQRSTUÜVWXYZß";
const wchar_t en_alphabet[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

typedef struct dir_op
{
  int begin_row;
  int begin_col;
  const int row;
  const int col;
} dir_op;

enum
{
  HORIZONTAL_NOOP,
  HORIZONTAL_INC
};
enum
{
  HORIZONTAL_BACKWARD_DEC = -1,
  HORIZONTAL_BACKWARD_NOOP
};
enum
{
  VERTICAL_NOOP,
  VERTICAL_INC
};
enum
{
  VERTICAL_UP_DEC = -1,
  VERTICAL_UP_NOOP
};
enum
{
  DIAGONAL_DOWN_RIGHT_INC = 1,
};
enum
{
  DIAGONAL_DOWN_LEFT_DEC = -1,
  DIAGONAL_DOWN_LEFT_INC = 1,
};
enum
{
  DIAGONAL_UP_RIGHT_DEC = -1,
  DIAGONAL_UP_RIGHT_INC = 1,
};
enum
{
  DIAGONAL_UP_LEFT_DEC = -1,
};


static int
dec (const int len)
{
  return (rand () % (GRID_SIZE - len)) + len;
}


static int
noop (const int len)
{
  // poor person's way to prevent the compiler warning about an unused function parameter
  if (len < 0)
    return len;

  return rand () % GRID_SIZE;
}


static int
inc (const int len)
{
  return rand () % (GRID_SIZE - len);
}


// Create an array of function pointers
static int (*op[]) (const int) = {
  dec,
  noop,
  inc
};


static int
get_row_op (const int op)
{
  // Adding 1 so accessing op[-1]() never happens
  return op + 1;
}


static int
get_col_op (const int op)
{
  return op + 1;
}


void
init_puzzle (wchar_t puzzle[][GRID_SIZE])
{
  int i, j;

  for (i = 0; i < GRID_SIZE; i++)
  {
    for (j = 0; j < GRID_SIZE; j++)
    {
      puzzle[i][j] = fill_char;
    }
  }
}

// Most of the network code and fail function was pinched and adapted from
// https://www.lemoda.net/c/fetch-web-page/

/* Quickie function to test for failures. It is actually better to use
   a macro here, since a function like this results in unnecessary
   function calls to things like "strerror". However, not every
   version of C has variadic macros. */

static void
fail (int test, const char *format, ...)
{
  if (test)
  {
    va_list args;
    va_start (args, format);
    vfprintf (stderr, format, args);
    va_end (args);
    // exit ok
    exit (EXIT_FAILURE);
  }
}


static inline int
get_words (wchar_t str[][BUFSIZ], const int fetch_count, const char *lang, const char *host_ptr)
{
  printf ("Attempting to fetch %d words from %s://%s...\n", fetch_count, SERVICE, host_ptr);
  char *buf_ptr = NULL;
#ifdef HAVE_CURL

  CURL *curl;
  CURLcode res;
  struct memory chunk = {0};

  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();

  if(curl) {
    const char *url_format = "https://%s/word?number=%d&lang=%s";
    char url[BUFSIZ];
    if ((size_t)snprintf(url, sizeof url, url_format, host_ptr, fetch_count, lang) >= sizeof url)
    {
      fputs("url truncated\n", stderr);
      exit(EXIT_FAILURE);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);

    // By default, the data will be sent to stdout when curl_easy_perform() is called. So we set these 3 options instead.
    // After curl_easy_perform is called, the data will be in chunk.response.

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);


#ifdef SKIP_PEER_VERIFICATION
    /*
     * If you want to connect to a site who is not using a certificate that is
     * signed by one of the certs in the CA bundle you have, you can skip the
     * verification of the server's certificate. This makes the connection
     * A LOT LESS SECURE.
     *
     * If you have a CA cert for the server stored someplace else than in the
     * default bundle, then the CURLOPT_CAPATH option might come handy for
     * you.
     */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

#ifdef SKIP_HOSTNAME_VERIFICATION
    /*
     * If the site you are connecting to uses a different host name that what
     * they have mentioned in their server certificate's commonName (or
     * subjectAltName) fields, libcurl will refuse to connect. You can skip
     * this check, but this will make the connection less secure.
     */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    fail (res != CURLE_OK, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

    buf_ptr = chunk.response;

    /* always cleanup */
    curl_easy_cleanup(curl);
  }
  else
    fputs ("Unable to initialize curl\n", stderr);

  curl_global_cleanup();

  if (buf_ptr == NULL)
    exit(EXIT_FAILURE);

#else

  struct addrinfo hints, *rp, *result;
  int error;
  /* "s" is the file descriptor of the socket. */
  int s;

  memset (&hints, 0, sizeof (hints));
  /* Don't specify what type of internet connection. */
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  error = getaddrinfo (host_ptr, SERVICE, &hints, &result);
  fail (error, gai_strerror (error));
  s = -1;
  for (rp = result; rp !=NULL; rp = rp->ai_next)
  {
    s = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    fail (s < 0, "socket: %s\n", strerror (errno));
    if (connect (s, rp->ai_addr, rp->ai_addrlen) < 0)
    {
      fprintf (stderr, "connect: %s\n", strerror (errno));
      close (s);
      exit (EXIT_FAILURE);
    }
    break;
  }
  freeaddrinfo (result);

  /* "format" is the format of the HTTP request we send to the web
     server. */

  const char *format = "\
GET /word?number=%d&lang=%s HTTP/1.1\r\n\
Host: %s\r\n\
User-Agent: github.com/theimpossibleastronaut/aawordsearch (v%s)\r\n\
\r\n";

  /* "msg" is the request message that we will send to the
     server. */

  char msg[BUFSIZ];
  int status =
    snprintf (msg, BUFSIZ, format, fetch_count, lang, host_ptr, VERSION);
  if (status >= BUFSIZ)
  {
    fputs ("snprintf failed.", stderr);
    return -1;
  }

  /* Send the request. */
  status = send (s, msg, strlen (msg), 0);

  /* Check it succeeded. The FreeBSD manual page doesn't mention
     whether "send" sets errno, but
     "http://pubs.opengroup.org/onlinepubs/009695399/functions/send.html"
     claims it does. */

  fail (status == -1, "send failed: %s\n", strerror (errno));

  /* Our receiving buffer. */
  char srv_str[BUFSIZ + 10];
  *srv_str = '\0';
  char buf[BUFSIZ + 10];
  *buf = '\0';
  int bytes;
  int bytes_total = 0;

  /* Loop until there is no data left to be read
     (see the recv man page for return codes) */
  while ((bytes = recv (s, srv_str, BUFSIZ, 0)) > 0)
  {
    int max_len = BUFSIZ - bytes_total;
    // concatenate the string each iteration of the loop
    status = snprintf (buf + bytes_total, max_len, "%s", srv_str);
    if (status >= max_len)
    {
      fputs ("snprintf failed.", stderr);
      return -1;
    }
    bytes_total += bytes;
  }

  if (close (s) != 0)
  {
    fputs ("Error closing socket\n", stderr);
    return -1;
  }

  fail (bytes == -1, "%s\n", strerror (errno));

  if (bytes_total == 0)
    return -1;

  buf_ptr = buf;

#endif

  // convert buf from char* to wchar_t*
  size_t buf_size = strlen(buf_ptr) + 1;
  wchar_t wbuf[buf_size];
  mbstowcs (wbuf, buf_ptr, buf_size);

  const wchar_t open_bracket[] = L"[\"";
  const wchar_t closed_bracket[] = L"\"]";
  const char *str_not_found = "Expected '%s' not found in string\n";
  wchar_t *buf_start = wcsstr (wbuf, open_bracket);

  if (buf_start != NULL)
    buf_start++;
  else
  {
    fprintf (stderr, str_not_found, open_bracket);
    return -1;
  }

  wchar_t *buf_end = wcsstr (buf_start, closed_bracket);
  if (buf_end != NULL)
    *buf_end = '\0';            // replaces the ']' so the string should end with '"'
  else
  {
    fprintf (stderr, str_not_found, closed_bracket);
    return -1;
  }

  wchar_t *wptr;
  const wchar_t delimiter[] = L"\",\"";
  wchar_t *token = wcstok (buf_start, delimiter, &wptr);
  int n_word = 0;

  while (token != NULL)
  {
    // printf("[%s]\n", token);
    wcscpy (str[n_word], token);
    token = wcstok (NULL, delimiter, &wptr);
    n_word++;
  }

  return 0;
}


static inline int
placer (dir_op * dir_op, const wchar_t *str, wchar_t puzzle[][GRID_SIZE])
{
  int row = dir_op->begin_row;
  int col = dir_op->begin_col;
  wchar_t *ptr = (wchar_t *) str;
  while (*ptr)
  {
    const wchar_t u = towupper (*ptr);
    if (!(u == puzzle[row][col] || puzzle[row][col] == fill_char))
      return -1;
    ptr++;
    row += dir_op->row;
    col += dir_op->col;
  }

  row = dir_op->begin_row;
  col = dir_op->begin_col;
  ptr = (wchar_t *) str;
  while (*ptr != '\0')
  {
    const wchar_t u = towupper (*ptr);
    puzzle[row][col] = u;
    ptr++;
    col += dir_op->col;
    row += dir_op->row;
  }
  return 0;
}


void
print_answer_key (FILE * restrict stream, wchar_t puzzle[][GRID_SIZE])
{
  int i, j;
  fputs (" ==] Answer key [==\n", stream);
  for (i = 0; i < GRID_SIZE; i++)
  {
    for (j = 0; j < GRID_SIZE; j++)
    {
      fprintf (stream, "%lc ", puzzle[i][j]);
    }
    fputs ("\n", stream);
  }
  fputs ("\n\n\n", stream);
  return;
}


void
print_puzzle (FILE * restrict stream, wchar_t puzzle[][GRID_SIZE], struct lang_vars *st_lang_ptr)
{
  int i, j;
  for (i = 0; i < GRID_SIZE; i++)
  {
    for (j = 0; j < GRID_SIZE; j++)
    {
      if (puzzle[i][j] == fill_char)
        fprintf (stream, "%lc ",
                 st_lang_ptr->alphabet[rand () % st_lang_ptr->length]);
      else
        fprintf (stream, "%lc ", puzzle[i][j]);
    }
    fputs ("\n", stream);
  }
  fputs ("\n", stream);
  return;
}


static void
print_words (FILE * restrict stream, wchar_t words[][BUFSIZ], const int n_string)
{
  int i = 0;
  while (i < n_string)
  {
    if (*words[i] != '\0')
      fprintf (stream, "%*ls", MAX_LEN + 1, words[i]);
    i++;

    // start a new row after every 3 words
    if (i % 3 == 0)
      fputs ("\n", stream);
  }
  fputs ("\n", stream);
  return;
}


dir_op *
create_dir_op ()
{
  static dir_op dir_ops[] = {
    {0, 0, HORIZONTAL_NOOP, HORIZONTAL_INC},
    {0, 0, HORIZONTAL_BACKWARD_NOOP, HORIZONTAL_BACKWARD_DEC},
    {0, 0, VERTICAL_INC, VERTICAL_NOOP},
    {0, 0, VERTICAL_UP_DEC, VERTICAL_UP_NOOP},
    {0, 0, DIAGONAL_DOWN_RIGHT_INC, DIAGONAL_DOWN_RIGHT_INC},
    {0, 0, DIAGONAL_DOWN_LEFT_INC, DIAGONAL_DOWN_LEFT_DEC},
    {0, 0, DIAGONAL_UP_RIGHT_DEC, DIAGONAL_UP_RIGHT_INC},
    {0, 0, DIAGONAL_UP_LEFT_DEC, DIAGONAL_UP_LEFT_DEC}
  };
  return dir_ops;
}


/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  INPUT_FILE = CHAR_MAX + 1,
  LANG
};


void
print_usage ()
{
  puts ("\n\
  -h, --help                  show help for command line options\n\
  -V, --version               show the program version number\n\
      --lang=LANG             language (optional; defaults to 'en')\n\
                              available: 'en','de','it','es'\n\
  -l, --log                   log the output to a file (in addition to stdout)\n\
      --input-file=FILE       Reads words from plain text file");
}

static inline int
write_log (wchar_t words[][BUFSIZ], wchar_t puzzle[][GRID_SIZE],
           const long unsigned seed, const int n_string, struct lang_vars *st_lang_ptr)
{
  {
    char log_file[BUFSIZ];
    snprintf (log_file, BUFSIZ, "aawordsearch_%lu.log", seed);
    FILE *fp = fopen (log_file, "w");
    if (fp != NULL)
    {
      fprintf (fp, "seed = %lu\n\n", seed);
      print_answer_key (fp, puzzle);
      print_puzzle (fp, puzzle, st_lang_ptr);
      print_words (fp, words, n_string);
    }
    else
    {
      fputs ("Error while opening ", stderr);
      perror (log_file);
      return -1;
    }

    if (fclose (fp) != 0)
      fprintf (stderr, "Error closing %s\n", log_file);

    char word_log_file[BUFSIZ];
    snprintf (word_log_file, BUFSIZ, "aawordsearch_words_%lu.log", seed);
    fp = fopen (word_log_file, "w");
    if (fp != NULL)
    {
      int l = 0;
      while (l < n_string)
      {
        fprintf (fp, "%ls\n", words[l]);
        l++;
      }
    }
    if (fclose (fp) != 0)
      fprintf (stderr, "Error closing %s\n", log_file);
  }
  return 0;
}


#ifndef TEST
/*!
 * Removes trailing white space from a string (including newlines, formfeeds,
 * tabs, etc
 * @param[out] str The string to be altered
 * @return void
 */
static void
trim_whitespace (wchar_t *str)
{
  if (str == NULL)
    return;

  wchar_t *pos_0 = str;
  /* Advance pointer until NULL terminator is found */
  while (*str != '\0')
    str++;

  /* set pointer to segment preceding NULL terminator */
  if (str != pos_0)
    str--;
  else
    return;

  while (isspace (*str))
  {
    *str = '\0';
    if (str != pos_0)
      str--;
    else
      break;
  }

  return;
}


int
main (int argc, char **argv)
{
  setlocale(LC_ALL, "");
  const int max_words_target = GRID_SIZE;
  const int fetch_count = max_words_target * 1.2;
  // this probably means the word server is having issues. If this number is exceeded,
  // we'll quit completely
  const int max_tot_err_allowed = 10;
  const int max_tries_per_direction = GRID_SIZE * 5;
  wchar_t puzzle[GRID_SIZE][GRID_SIZE];

  bool want_log = false;
  char *word_file_path = NULL;
  char *lang = NULL;
  char *lang_en = "en";

  const struct option long_options[] = {
    {"help", no_argument, NULL, 'h'},
    {"log", no_argument, NULL, 'l'},
    {"version", no_argument, NULL, 'V'},
    {"input-file", required_argument, NULL, INPUT_FILE},
    {"lang", required_argument, NULL, LANG},
    {0, 0, 0, 0}
  };

  const char *short_options = "hlV";
  int c;
  while ((c = getopt_long (argc, argv, short_options, long_options, NULL)) != -1)
  {
    switch (c)
    {
    case 'h':
      print_usage ();
      return 0;
    case 'l':
      want_log = true;
      puts ("The log will be activated! Hooray!");
      break;
    case INPUT_FILE:
      word_file_path = optarg;
      break;
    case LANG:
      lang = optarg;
      break;
    case 'V':
      // printf ("%s v%s\n\n", PROGRAM_NAME, VERSION);
      puts (PROGRAM_NAME " " VERSION "\n");
      break;
    case '?':
      printf ("Try '%s --help' for more information.\n", argv[0]);
      return -1;
    default:
      printf ("?? getopt returned character code 0%o ??\n", c);
    }
  }

  /* Print any remaining command line arguments (not options). */
  if (optind < argc)
  {
    printf ("non-option ARGV-elements: ");
    while (optind < argc)
      printf ("%s ", argv[optind++]);
    putchar ('\n');
  }

  wchar_t fetched_words[GRID_SIZE * 2][BUFSIZ];
  int max_list_size = sizeof fetched_words / sizeof fetched_words[0];

  if (word_file_path != NULL)
  {
    FILE* fp = fopen(word_file_path, "r");
    if (fp == NULL) {
      fputs ("error opening word file: ", stderr);
      perror (word_file_path);
      return -1;
    }

    int cur_word = 0;
    while (cur_word < max_list_size && fgetws (fetched_words[cur_word], sizeof fetched_words[0], fp) != NULL )
    {
      trim_whitespace (fetched_words[cur_word]);
      wchar_t *ptr = wcschr(fetched_words[cur_word], ' ');
      wchar_t *ptr2 = wcschr(fetched_words[cur_word], '.');
      if (*fetched_words[cur_word] == '\0' || ptr != NULL || ptr2 != NULL)
        continue;
      cur_word++;
    }

    if (cur_word < max_words_target)
    {
      fprintf(stderr, "Your word list must contain at least %d words.\n", max_words_target);
      return -1;
    }

    if (fclose(fp) != 0)
    {
      fputs ("Error closing file:", stderr);
      perror (word_file_path);
      // errno is a global that's included with errno.h. Many common std
      // functions set it after they're called.
      return errno;
    }
  }

  init_puzzle (puzzle);

  if (lang == NULL)
    lang = lang_en;

  struct lang_vars st_langvars[] = {
    {"en", "en_US", en_alphabet, wcslen(en_alphabet)},
    {"de", "de_DE.UTF-8", de_alphabet, wcslen(de_alphabet)},
    {"it", "it_IT.UTF-8", it_alphabet, wcslen(it_alphabet)},
    {"es", "es_ES.UTF-8", es_alphabet, wcslen(es_alphabet)},
    {NULL, NULL, NULL, 0}
  };

  struct lang_vars *st_lang_ptr = st_langvars;
  while (st_lang_ptr->lang != NULL)
  {
    if (strcmp (lang, st_lang_ptr->lang) == 0)
      break;
    st_lang_ptr++;
  }

  if (st_lang_ptr->lang == NULL)
  {
    fputs("Invalid lang provided", stderr);
    return -1;
  }

  setlocale(LC_ALL, st_lang_ptr->locale);

  /* seed the random number generator */
  const time_t seed = time (NULL);
  srand (seed);
  int n_tot_err = 0;

  if (word_file_path == NULL)
  {
    const char **host_ptr = HOST;
    int r = -1;
    while (*host_ptr != NULL && r != 0)
    {
      int strikes = 0;
      do
      {
        r = get_words (fetched_words, fetch_count, st_lang_ptr->lang, *host_ptr);
        if (r != 0)
          n_tot_err++;
      }
      while (++strikes < 3 && r == -1);

      if (r != 0)
        fputs ("Failed to get words from server\n", stderr);

      host_ptr++;
    }
    if (r != 0)
      return -1;
  }

  int i;
  wchar_t words[GRID_SIZE * 2][BUFSIZ];
  for (i = 0; i < max_words_target; i++)
  {
    *words[i] = '\0';
  }

  dir_op *dir_op = create_dir_op ();
  int n_string = 0, f_string = 0;
  int cur_dir = 0;
  while ((n_string < max_words_target) && n_tot_err < max_tot_err_allowed)
  {
    size_t len = wcslen (fetched_words[f_string]);
    if (len > (size_t)MAX_LEN)          // skip the word if it exceeds this value
    {
      printf ("word '%ls' exceeded max length\n", fetched_words[f_string]);
      f_string++;
      continue;
    }

    wchar_t *ptr = wcschr(fetched_words[f_string], ' ');
    wchar_t *ptr2 = wcschr(fetched_words[f_string], '.');
    if (ptr != NULL || ptr2 != NULL)
    {
      printf ("Skipping '%ls'\n", fetched_words[f_string]);
      f_string++;
      continue;
    }

    wcscpy (words[n_string], fetched_words[f_string]);
    printf ("%d.) %ls\n", n_string + 1, words[n_string]);

    // Try placing the word in all 8 directions, each direction at most
    // max_tries_per_direction. If successful, break from both loops and get
    // the next word.
    int d, r;
    for (d = 0; d < N_DIRECTIONS; d++)
    {
      int ctr;
      for (ctr = 0; ctr < max_tries_per_direction; ctr++)
      {
        dir_op[cur_dir].begin_row =
          op[get_row_op (dir_op[cur_dir].row)] (len);
        dir_op[cur_dir].begin_col =
          op[get_col_op (dir_op[cur_dir].col)] (len);
        r = placer (&dir_op[cur_dir], words[n_string], puzzle);
        if (!r)
          break;
      }
      cur_dir == N_DIRECTIONS - 1 ? cur_dir = 0 : cur_dir++;
      if (!r)
      {
        n_string++;
        f_string++;
        break;
      }
    }
    if (r)
    {
      n_tot_err++;
      printf ("Unable to find a place for '%ls'\n", words[n_string]);
      f_string++;
    }

    if (n_tot_err >= max_tot_err_allowed)
    {
      fprintf (stderr, "Too many errors (%d); giving up\n", n_tot_err);
      return -1;
    }
  }

  print_answer_key (stdout, puzzle);
  print_puzzle (stdout, puzzle, st_lang_ptr);
  print_words (stdout, words, n_string);

  // write the seed, answer key, and puzzle to a file
  if (want_log)
    if (write_log (words, puzzle, seed, n_string, st_lang_ptr) != 0)
      return -1;

  return 0;
}
#else

/* assert() doesn't nothing if NDEBUG is defined, so let's make sure it's undefined
before including the header */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

enum
{
  HORIZONTAL,
  HORIZONTAL_BACKWARD,
  VERTICAL,
  VERTICAL_UP,
  DIAGONAL_DOWN_RIGHT,
  DIAGONAL_DOWN_LEFT,
  DIAGONAL_UP_RIGHT,
  DIAGONAL_UP_LEFT
};


void
test_dir_ops (dir_op * dir_op)
{
  /* loop through each direction once to make sure the corresponding
     constants (3rd and 4th fields) are correct */
  int i;
  for (i = 0; i < N_DIRECTIONS; i++)
  {
    switch (i)
    {
    case HORIZONTAL:
      assert (dir_op[i].row == HORIZONTAL_NOOP);
      assert (dir_op[i].col == HORIZONTAL_INC);
      break;
    case HORIZONTAL_BACKWARD:
      assert (dir_op[i].row == HORIZONTAL_BACKWARD_NOOP);
      assert (dir_op[i].col == HORIZONTAL_BACKWARD_DEC);
      break;
    case VERTICAL:
      assert (dir_op[i].row == VERTICAL_INC);
      assert (dir_op[i].col == VERTICAL_NOOP);
      break;
    case VERTICAL_UP:
      assert (dir_op[i].row == VERTICAL_UP_DEC);
      assert (dir_op[i].col == VERTICAL_UP_NOOP);
      break;
    case DIAGONAL_DOWN_RIGHT:
      assert (dir_op[i].row == DIAGONAL_DOWN_RIGHT_INC);
      assert (dir_op[i].col == DIAGONAL_DOWN_RIGHT_INC);
      break;
    case DIAGONAL_DOWN_LEFT:
      assert (dir_op[i].row == DIAGONAL_DOWN_LEFT_INC);
      assert (dir_op[i].col == DIAGONAL_DOWN_LEFT_DEC);
      break;
    case DIAGONAL_UP_RIGHT:
      assert (dir_op[i].row == DIAGONAL_UP_RIGHT_DEC);
      assert (dir_op[i].col == DIAGONAL_UP_RIGHT_INC);
      break;
    case DIAGONAL_UP_LEFT:
      assert (dir_op[i].row == DIAGONAL_UP_LEFT_DEC);
      assert (dir_op[i].col == DIAGONAL_UP_LEFT_DEC);
      break;
    }
  }
  return;
}


/* loop through each direction 50? times to make sure that the random numbers
generated don't exceed the desired values */
void
test_starting_points (dir_op * dir_op, const int len)
{
  int i, j;
  int row, col;
  for (i = 0; i < N_DIRECTIONS; i++)
  {
    fprintf (stderr, "i:%d\n", i);
    for (j = 0; j < GRID_SIZE * 5; j++)
    {
      row = op[get_row_op (dir_op[i].row)] (len);
      col = op[get_col_op (dir_op[i].col)] (len);
      // fprintf (stderr, "%d", row);
      // fprintf (stderr, "%d", col);
      switch (i)
      {
      case HORIZONTAL:
        assert (row >= 0 && row < GRID_SIZE);
        assert (col >= 0 && col < GRID_SIZE - len);
        break;
      case HORIZONTAL_BACKWARD:
        assert (row >= 0 && row < GRID_SIZE);
        assert (col >= len && col < GRID_SIZE);
        break;
      case VERTICAL:
        assert (row >= 0 && row < GRID_SIZE - len);
        assert (col >= 0 && col < GRID_SIZE);
        break;
      case VERTICAL_UP:
        assert (row >= 0 && row < GRID_SIZE);
        assert (row >= len && row < GRID_SIZE);
        break;
      case DIAGONAL_DOWN_RIGHT:
        assert (row >= 0 && row < GRID_SIZE - len);
        assert (col >= 0 && col < GRID_SIZE - len);
        break;
      case DIAGONAL_DOWN_LEFT:
        assert (row >= 0 && row < GRID_SIZE - len);
        assert (col >= len && col < GRID_SIZE);
        break;
      case DIAGONAL_UP_RIGHT:
        assert (row >= 0 && row < GRID_SIZE);
        assert (col >= 0 && col < GRID_SIZE - len);
        break;
      case DIAGONAL_UP_LEFT:
        assert (row >= 0 && row < GRID_SIZE);
        assert (col >= len && col < GRID_SIZE);
        break;
      }
    }
  }
  return;
}


int
main (void)
{
  dir_op *dir_op = create_dir_op ();

  test_dir_ops (dir_op);
  test_starting_points (dir_op, 5);
  test_starting_points (dir_op, GRID_SIZE - 2);

  return 0;
}
#endif
