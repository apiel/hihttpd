#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "mongoose.h"

// Return: 1 if regular file, 2 if directory, 0 if not found
static int exists(const char *path) {
  struct stat st;
  return stat(path, &st) != 0 ? 0 : S_ISDIR(st.st_mode)  == 0 ? 1 : 2;
}

// Return: 1 if regular file, 2 if directory, 0 if not found
static int is_local_file(const char *uri, char *path, size_t path_len) {
  snprintf(path, path_len, "data%s", uri);
  return exists(path);
}

static int has_wr_permission(struct mg_connection *conn, char * wr) {
  char path[550], apikey[33];
  mg_get_var(conn, "apikey", apikey, sizeof(apikey));
  printf("apikey %s\n", apikey);
  snprintf(path, sizeof(path) - strlen(path) - 1, "data%s.%s.%s", conn->uri, wr, apikey);
  printf("Test Permission: [%s]\n", path);
  return exists(path);
}

static int try_to_serve_locally(struct mg_connection *conn) {
  char path[500], buf[2000];
  int n;
  FILE *fp = NULL;

  if (is_local_file(conn->uri, path, sizeof(path)) && has_wr_permission(conn, "read")) {
    if ((fp = fopen(path, "rb")) != NULL) {
      printf("Serving [%s] locally \n", path);
      mg_send_header(conn, "Connection", "close");
      mg_send_header(conn, "Content-Type", mg_get_mime_type(path, "text/plain"));
      while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        mg_send_data(conn, buf, n);
      }
      mg_send_data(conn, "", 0);
      fclose(fp);
    }
  }
  return fp == NULL ? MG_FALSE : MG_TRUE;
}

static int try_to_write_locally(struct mg_connection *conn, char *write) {
  char path[500];
  FILE *fp = NULL;

  if (is_local_file(conn->uri, path, sizeof(path)) && has_wr_permission(conn, "write")) {
    if ((fp = fopen(path, "w")) != NULL) {
      fprintf(fp, "%s", write);
      fclose(fp);
      printf("Writing [%s] locally \n", path);
      mg_send_header(conn, "Connection", "close");
      mg_send_header(conn, "Content-Type", mg_get_mime_type(path, "text/plain"));
      mg_printf_data(conn, "success");
    }
  }
  return fp == NULL ? MG_FALSE : MG_TRUE;
}

static int ev_handler(struct mg_connection *conn, enum mg_event ev) {
  char *write;
  switch (ev) {
    case MG_AUTH: return MG_TRUE;
    case MG_REQUEST:
      write = malloc(sizeof(conn->query_string)); 
      mg_get_var(conn, "write", write, sizeof(write));
      if (write[0]) {
        return try_to_write_locally(conn, write);
      }
      else {
        return try_to_serve_locally(conn);
      }
    default: return MG_FALSE;
  }
}

int main(void) {
  struct mg_server *server;

  // Create and configure the server
  server = mg_create_server(NULL, ev_handler);
  mg_set_option(server, "listening_port", "8080");

  // Serve request. Hit Ctrl-C to terminate the program
  printf("Starting on port %s\n", mg_get_option(server, "listening_port"));
  for (;;) {
    mg_poll_server(server, 1000);
  }

  // Cleanup, and free server instance
  mg_destroy_server(&server);

  return 0;
}
