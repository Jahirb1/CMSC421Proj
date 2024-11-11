#ifndef APP_H
#define APP_H

#include "buffer.h"

#define TIMEOUT_SECONDS 300  // 5 minutes timeout

void run_application(int app_id, struct temp_buffer *temp_buffers);

#endif // APP_H