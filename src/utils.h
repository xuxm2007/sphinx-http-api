// Copyright 2012 zhaigy hontlong@gmail.com
#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include <memory.h>
#include <string>

#define ISSPACE(x) \
  ((x) == ' ' || (x) == '\r' || (x) == '\n' || (x) == '\f' \
  || (x) == '\b' || (x) == '\t' )

inline std::string & trim_right(std::string & source) {
  return source.erase(source.find_last_not_of(" \t") + 1);
}

inline std::string & trim_left(std::string & source) {
  return source.erase(0, source.find_first_not_of(" "));
}

inline std::string & trim(std::string & source) {
  return trim_left(trim_right(source));
}

/* 
static char * trim(char *String) {
  char *Tail, *Head;
  for (Tail = String + strlen(String) - 1; Tail >= String; Tail--) {
    if (!ISSPACE(*Tail)) {
      break;
    }
  }
  Tail[1] = 0;
  for (Head = String; Head <= Tail; Head++) {
    if (!ISSPACE(*Head)) {
      break;
    }
  }
  if (Head != String) {
    memcpy(String, Head, (Tail - Head + 2) * sizeof(char)); // NOLINT
  }
  return String;
}
*/

#endif  // SRC_UTILS_H_

