#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

typedef struct def_t def_t;

struct def_t {
  char name[64];
  char value[256];
};

const char *inc_buf[16];
int inc_cnt = 0;

def_t *def_buf = NULL;
int def_cnt = 0;

FILE *output = NULL;
uint64_t cond = 0;

void parse(FILE *input);

void parse_line(const char *line_buffer, int line_length) {
  char buffer[2048];
  int length = 0;
  
  for (int i = 0; i < line_length; i++) {
    int replace = 0;
    
    for (int j = 0; j < def_cnt; j++) {
      if (i > line_length - strlen(def_buf[j].name)) continue;
      
      if (i < line_length - strlen(def_buf[j].name) && isalnum(def_buf[j].name[strlen(def_buf[j].name) - 1])) {
        if (isalnum(line_buffer[i + strlen(def_buf[j].name)])) continue;
      }
      
      if (i > 0 && isalnum(def_buf[j].name[0])) {
        if (isalnum(line_buffer[i - 1])) continue;
      }
      
      if (!memcmp(def_buf[j].name, line_buffer + i, strlen(def_buf[j].name))) {
        for (int k = 0; k < strlen(def_buf[j].value); k++) {
          buffer[length++] = def_buf[j].value[k];
        }
        
        i += strlen(def_buf[j].name) - 1;
        
        replace = 1;
        break;
      }
    }
    
    if (!replace) {
      buffer[length++] = line_buffer[i];
    }
  }
  
  buffer[length] = '\0';
  
  if (length) {
    if (buffer[0] == '#') {
      strcpy(buffer, line_buffer);
      
      if (length > strlen("#define")) {
        int word_len = strlen("#define");
        
        if (!memcmp(buffer, "#define", word_len)) {
          if (cond) return;
          
          int offset_1 = 0;
          int offset_2 = 0;
          
          while (buffer[word_len] == ' ') word_len++;
          
          while (buffer[word_len + offset_1] && buffer[word_len + offset_1] != ' ') {
            if (buffer[word_len + offset_1] == '`') {
              def_buf[def_cnt].name[offset_1] = ' ';
            } else {
              def_buf[def_cnt].name[offset_1] = buffer[word_len + offset_1];
            }
            
            offset_1++;
          }
          
          def_buf[def_cnt].name[offset_1] = '\0';
          while (buffer[word_len + offset_1] == ' ') offset_1++;
          
          while (buffer[word_len + offset_1 + offset_2] && buffer[word_len + offset_1 + offset_2] != ' ') {
            if (buffer[word_len + offset_1 + offset_2] == '`') {
              def_buf[def_cnt].value[offset_2] = ' ';
            } else {
              def_buf[def_cnt].value[offset_2] = buffer[word_len + offset_1 + offset_2];
            }
            
            offset_2++;
          }
          
          def_buf[def_cnt].value[offset_2] = '\0';
          def_cnt++;
          
          return;
        }
      }
      
      if (length > strlen("#include")) {
        int word_len = strlen("#include");
        
        if (!memcmp(buffer, "#include", word_len)) {
          if (cond) return;
          
          char path[256];
          int offset = 0;
          
          while (buffer[word_len] != '"' && buffer[word_len] != '<') word_len++;
          word_len++;
          
          while (buffer[word_len + offset] && buffer[word_len + offset] != '"' && buffer[word_len + offset] != '>') {
            path[offset] = buffer[word_len + offset];
            offset++;
          }
          
          path[offset] = '\0';
          
          FILE *next = fopen(path, "r");
          
          if (!next) {
            for (int i = 0; i < inc_cnt; i++) {
              char new_path[512] = {0};
              
              strcpy(new_path, inc_buf[i]);
              strcat(new_path, path);
              
              if (next = fopen(new_path, "r")) {
                break;
              }
            }
          }
          
          if (!next) {
            printf("error: cannot open '%s'\n", path);
            exit(1);
          }
          
          parse(next);
          fclose(next);
          
          return;
        }
      }
      
      if (length > strlen("#undef")) {
        int word_len = strlen("#undef");
        
        if (!memcmp(buffer, "#undef", word_len)) {
          if (cond) return;
          
          char name[64];
          int offset = 0;
          
          while (buffer[word_len] == ' ') word_len++;
          
          while (buffer[word_len + offset] && buffer[word_len + offset] != ' ') {
            if (buffer[word_len + offset] == '`') {
              name[offset] = ' ';
            } else {
              name[offset] = buffer[word_len + offset];
            }
            
            offset++;
          }
          
          name[offset] = '\0';
          
          for (int i = 0; i < def_cnt; i++) {
            if (!strcmp(def_buf[i].name, name)) {
              memmove(def_buf + i, def_buf + i + 1, (def_cnt - i - 1) * sizeof(def_t));
              def_cnt--, i--;
            }
          }
          
          return;
        }
      }
      
      if (length > strlen("#ifdef")) {
        int word_len = strlen("#ifdef");
        
        if (!memcmp(buffer, "#ifdef", word_len)) {
          char name[64];
          int offset = 0;
          
          while (buffer[word_len] == ' ') word_len++;
          
          while (buffer[word_len + offset] && buffer[word_len + offset] != ' ') {
            if (buffer[word_len + offset] == '`') {
              name[offset] = ' ';
            } else {
              name[offset] = buffer[word_len + offset];
            }
            
            offset++;
          }
          
          name[offset] = '\0';
          int bit = 1;
          
          for (int i = 0; i < def_cnt; i++) {
            if (!strcmp(def_buf[i].name, name)) {
              bit = 0;
              break;
            }
          }
          
          cond = (cond << 1) | bit;
          return;
        }
      }
      
      if (length > strlen("#ifndef")) {
        int word_len = strlen("#ifndef");
        
        if (!memcmp(buffer, "#ifndef", word_len)) {
          char name[64];
          int offset = 0;
          
          while (buffer[word_len] == ' ') word_len++;
          
          while (buffer[word_len + offset] && buffer[word_len + offset] != ' ') {
            if (buffer[word_len + offset] == '`') {
              name[offset] = ' ';
            } else {
              name[offset] = buffer[word_len + offset];
            }
            
            offset++;
          }
          
          name[offset] = '\0';
          int bit = 0;
          
          for (int i = 0; i < def_cnt; i++) {
            if (!strcmp(def_buf[i].name, name)) {
              bit = 1;
              break;
            }
          }
          
          cond = (cond << 1) | bit;
          return;
        }
      }
      
      if (length >= strlen("#endif")) {
        int word_len = strlen("#endif");
        
        if (!memcmp(buffer, "#endif", word_len)) {
          cond = cond >> 1;
          return;
        }
      }
      
      if (length >= strlen("#else")) {
        int word_len = strlen("#else");
        
        if (!memcmp(buffer, "#else", word_len)) {
          cond = cond ^ 1;
          return;
        }
      }
    } else {
      if (cond) return;
      
      fwrite(buffer, 1, length, output);
      fputc('\n', output);
    }
  }
}

void parse(FILE *input) {
  char line_buffer[2048];
  int line_length = 0;
  
  while (!feof(input)) {
    char c = fgetc(input);
    
    if (c == '\n') {
      line_buffer[line_length] = '\0';
      parse_line(line_buffer, line_length);
      
      line_length = 0;
    } else {
      line_buffer[line_length++] = c;
    }
  }
  
  if (line_length) {
    line_buffer[line_length] = '\0';
    parse_line(line_buffer, line_length);
  }
}

int main(int argc, const char **argv) {
  def_buf = malloc(sizeof(def_t) * 512);
  
  if (!def_buf) {
    printf("error: cannot allocate define buffer\n");
    return 1;
  }
  
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-I")) {
      inc_buf[inc_cnt++] = argv[++i];
    } else if (!strcmp(argv[i], "-d")) {
      strcpy(def_buf[def_cnt].name, argv[++i]);
      strcpy(def_buf[def_cnt].value, argv[++i]);
      
      def_cnt++;
    } else if (!strcmp(argv[i], "-D")) {
      strcpy(def_buf[def_cnt].name, argv[++i]);
      def_buf[def_cnt].value[0] = '\0';
      
      def_cnt++;
    } else if (!strcmp(argv[i], "-o")) {
      if (output) {
        printf("warning: cannot set multiple outputs\n");
        i++;
        
        continue;
      }
      
      if (!(output = fopen(argv[++i], "w"))) {
        printf("error: cannot open '%s'\n", argv[i - 1]);
        return 1;
      }
    } else if (argv[i][0] != '-') {
      if (!output) {
        printf("error: must specify output first\n");
        return 1;
      }
      
      FILE *input = fopen(argv[i], "r");
      
      if (!input) {
        printf("error: cannot open '%s'\n", argv[i]);
        return 1;
      }
      
      parse(input);
      fclose(input);
    }
  }
  
  if (output) {
    fclose(output);
  }
  
  return 0;
}
