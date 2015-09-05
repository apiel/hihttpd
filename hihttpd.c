#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "mongoose.h"

//#define PERMISSION_BY_FILENAME

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

#if defined(PERMISSION_BY_FILENAME)
/**
 * Check for permission base on filenames
 * 
 * filename.html.tpyepermission.apikkey
 * filename.html.tpyepermission.all
 * eg.
 * test.html.r.098f6bcd4621d373cade4e832627b4f6
 * test.html.w.098f6bcd4621d373cade4e832627b4f6
 * test.html.x.098f6bcd4621d373cade4e832627b4f6
 * 
 * tpyepermission: xrw
 * 
 * @param conn
 * @param rwx
 * @param key
 * @return 0 or 1
 */
static int _has_rwx_permission(struct mg_connection *conn, char rwx, char * key) {
  char path[550];
  snprintf(path, sizeof(path) - strlen(path) - 1, "data%s.%s.%s", conn->uri, rwx, key);
  printf("Test [%c] Permission [%s] for [%s]: %d\n", rwx, path, key);
  return exists(path);  
}
#else
static int _has_rwx_permission(struct mg_connection *conn, char rwx, char * key) {
  char path[550], keycmp[100];
  char * line = NULL;
  size_t len = 0;
  int permission = 0;
  FILE *fp = NULL;
    
  snprintf(path, sizeof(path) - strlen(path) - 1, "data%s.permission", conn->uri);
  if ((fp = fopen(path, "r")) != NULL) {
    while (getline(&line, &len, fp) != -1) {
      line[strlen(line)-1] = '\0'; // remove \n
      memcpy(keycmp, &line[4], 100);
      if (strcmp(keycmp, key) == 0) {
        permission = line[0] == rwx || line[1] == rwx || line[2] == rwx;
        //printf("campare %s to %s %d\n", keycmp, key, permission);
        break;
      }
    }
    fclose(fp);
  }
  printf("Test [%c] Permission [%s] for [%s]: %d\n", rwx, path, key, permission);
  return permission;  
}
#endif

static int has_rwx_permission(struct mg_connection *conn, char rwx) {
  char apikey[33];
  mg_get_var(conn, "apikey", apikey, sizeof(apikey));
  printf("apikey %s\n", apikey);
  return _has_rwx_permission(conn, rwx, "all") 
          || (apikey[0] != '\0' && _has_rwx_permission(conn, rwx, apikey));
}

static int try_to_serve_locally(struct mg_connection *conn) {
  char path[500], buf[2000];
  int n;
  FILE *fp = NULL;

  if (is_local_file(conn->uri, path, sizeof(path)) && has_rwx_permission(conn, 'r')) {
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

  if (is_local_file(conn->uri, path, sizeof(path)) && has_rwx_permission(conn, 'w')) {
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

static int try_to_exec_locally(struct mg_connection *conn, char *exec) {
  char path[500], buf[2000];
  char * cmd;
  int n, cmd_size;
  FILE *fp = NULL;

  if (is_local_file(conn->uri, path, sizeof(path)) && has_rwx_permission(conn, 'x')) {
    cmd_size = strlen(path)+1+strlen(exec)+1;
    cmd = malloc(cmd_size);
    snprintf(cmd, cmd_size, "%s %s", path, exec);
    printf("Try to run [%s] locally \n", cmd);
    if ((fp = popen(path, "r")) != NULL) {
      printf("Exec [%s] locally \n", path);
      mg_send_header(conn, "Connection", "close");
      mg_send_header(conn, "Content-Type", mg_get_mime_type(path, "text/plain"));
      while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        mg_send_data(conn, buf, n);
      }
      mg_send_data(conn, "", 0);
      pclose(fp);
    }
  }
  return fp == NULL ? MG_FALSE : MG_TRUE;
}

static int ev_handler(struct mg_connection *conn, enum mg_event ev) {
  char *action;
  int action_size;
  switch (ev) {
    case MG_AUTH: return MG_TRUE;
    case MG_REQUEST:
      action_size = strlen(conn->query_string)+1;
      action = malloc(action_size); 
      if (mg_get_var(conn, "exec", action, action_size) > -1) {
        printf("Exec...\n");
        return try_to_exec_locally(conn, action);
      }
      else if (mg_get_var(conn, "write", action, action_size) > 0) {
        printf("Write...\n");
        return try_to_write_locally(conn, action);
      }
      else {
        printf("Read...\n");
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
