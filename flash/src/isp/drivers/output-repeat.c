#include <stdarg.h>
#include <string.h>

#define OUTPUT_JSON_REPEAT (3)

struct MallocReturn {
  char* address;
  char* original;
};

char* repeat_malloc(const char* format, ...) {
  va_list args;
  va_start(args, format);
  const int count = vsnprintf(NULL, 0, format, args);
  va_end(args);

  char* original = malloc((count - 1) * OUTPUT_JSON_REPEAT + 1);
  return original;
}

void repeat_printf(char* buffer, const char* format, ...) {
  va_list args;
  va_start(args, format);
  const unsigned int actualSize = vsprintf(buffer, format, args);
  // printf("actualSize = %d [%s]\n", actualSize, buffer);
  va_end(args);

  int i = actualSize - 1;
  unsigned int j = actualSize * OUTPUT_JSON_REPEAT - 1;
  buffer[j + 1] = '\0';

  for (; i >= 0; i--) {
    for (int k = 0; k < OUTPUT_JSON_REPEAT; k++) {
      // printf("buffer[%d] = buffer[%d] = %c(%x)\n", j, i, buffer[i],
      // buffer[i]);
      buffer[j] = buffer[i];
      j--;
    }
  }

  uart_send_data(STDIO_UART_NUM, buffer, actualSize * 3);
}
