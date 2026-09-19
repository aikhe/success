#include "nfd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <windows.h>
  #include <commdlg.h>
#else
  #include <unistd.h>
#endif

static char g_nfd_error[256] = {0};

const char *NFD_GetError(void) {
  return g_nfd_error;
}

static void set_nfd_error(const char *msg) {
  strncpy(g_nfd_error, msg, sizeof(g_nfd_error) - 1);
  g_nfd_error[sizeof(g_nfd_error) - 1] = '\0';
}

static void populate_pathset(nfdpathset_t *outPaths, char **paths, size_t count) {
  if (!outPaths || count == 0) {
    if (outPaths) {
      outPaths->buf = NULL;
      outPaths->indices = NULL;
      outPaths->count = 0;
    }
    return;
  }

  size_t total_bytes = 0;
  for (size_t i = 0; i < count; i++) {
    total_bytes += strlen(paths[i]) + 1;
  }

  outPaths->buf = malloc(total_bytes);
  outPaths->indices = malloc(count * sizeof(size_t));
  outPaths->count = count;

  size_t offset = 0;
  for (size_t i = 0; i < count; i++) {
    outPaths->indices[i] = offset;
    size_t len = strlen(paths[i]);
    memcpy(outPaths->buf + offset, paths[i], len + 1);
    offset += len + 1;
  }
}

size_t NFD_PathSet_GetCount(const nfdpathset_t *pathSet) {
  if (!pathSet) return 0;
  return pathSet->count;
}

nfdchar_t *NFD_PathSet_GetPath(const nfdpathset_t *pathSet, size_t index) {
  if (!pathSet || index >= pathSet->count || !pathSet->buf || !pathSet->indices) {
    return NULL;
  }
  return pathSet->buf + pathSet->indices[index];
}

void NFD_PathSet_Free(nfdpathset_t *pathSet) {
  if (!pathSet) return;
  if (pathSet->buf) {
    free(pathSet->buf);
    pathSet->buf = NULL;
  }
  if (pathSet->indices) {
    free(pathSet->indices);
    pathSet->indices = NULL;
  }
  pathSet->count = 0;
}

nfdresult_t NFD_OpenDialogMultiple(const nfdchar_t *filterList,
                                   const nfdchar_t *defaultPath,
                                   nfdpathset_t *outPaths) {
  (void)defaultPath;
  if (!outPaths) return NFD_ERROR;
  memset(outPaths, 0, sizeof(*outPaths));

#ifndef _WIN32
  // Check if a graphical display session is active
  const char *display = getenv("DISPLAY");
  const char *wayland = getenv("WAYLAND_DISPLAY");

  if (display || wayland) {
    char cmd[512];
    // Try zenity first
    snprintf(cmd, sizeof(cmd), "zenity --file-selection --multiple --separator=\"|\" --title=\"Select Attachment\" 2>/dev/null");
    FILE *fp = popen(cmd, "r");
    if (fp) {
      char output[4096] = {0};
      if (fgets(output, sizeof(output), fp) != NULL) {
        int status = pclose(fp);
        output[strcspn(output, "\r\n")] = '\0';
        if (status == 0 && strlen(output) > 0) {
          // Count items separated by '|'
          size_t count = 1;
          for (char *p = output; *p; p++) {
            if (*p == '|') count++;
          }
          char **paths = malloc(count * sizeof(char *));
          size_t idx = 0;
          char *token = strtok(output, "|");
          while (token && idx < count) {
            paths[idx++] = token;
            token = strtok(NULL, "|");
          }
          populate_pathset(outPaths, paths, idx);
          free(paths);
          return NFD_OKAY;
        }
      } else {
        pclose(fp);
      }
    }
  }

  // Terminal fallback: if GUI is unavailable or canceled/failed
  printf("\n\033[36m[File Attachment] Enter file path (or press Enter to cancel): \033[0m");
  fflush(stdout);
  char input_buf[1024] = {0};
  if (fgets(input_buf, sizeof(input_buf), stdin) != NULL) {
    input_buf[strcspn(input_buf, "\r\n")] = '\0';
    // Trim leading/trailing spaces
    char *start = input_buf;
    while (*start == ' ' || *start == '\t') start++;
    if (strlen(start) > 0) {
      // Check if file exists
      if (access(start, F_OK) == 0) {
        char *paths[1] = {start};
        populate_pathset(outPaths, paths, 1);
        return NFD_OKAY;
      } else {
        printf("\033[31m[Error] File '%s' not found.\033[0m\n", start);
        set_nfd_error("File not found");
        return NFD_ERROR;
      }
    }
  }
  return NFD_CANCEL;

#else
  // Windows Win32 OpenFileDialog
  OPENFILENAMEA ofn;
  char szFile[4096] = {0};
  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = "Supported Files (*.png;*.jpg;*.pdf)\0*.png;*.jpg;*.jpeg;*.pdf\0All Files (*.*)\0*.*\0";
  ofn.nFilterIndex = 1;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

  if (GetOpenFileNameA(&ofn)) {
    // OFN_ALLOWMULTISELECT with OFN_EXPLORER puts directory followed by null-separated filenames, ending with double null
    char *p = szFile + strlen(szFile) + 1;
    if (*p == '\0') {
      // Single file selected
      char *paths[1] = {szFile};
      populate_pathset(outPaths, paths, 1);
      return NFD_OKAY;
    } else {
      // Multiple files: first is dir, rest are filenames
      char dir[MAX_PATH];
      strncpy(dir, szFile, sizeof(dir) - 1);
      dir[sizeof(dir) - 1] = '\0';

      size_t count = 0;
      char *scan = p;
      while (*scan != '\0') {
        count++;
        scan += strlen(scan) + 1;
      }

      char **paths = malloc(count * sizeof(char *));
      char full_paths[count][MAX_PATH];
      scan = p;
      for (size_t i = 0; i < count; i++) {
        snprintf(full_paths[i], sizeof(full_paths[i]), "%s\\%s", dir, scan);
        paths[i] = full_paths[i];
        scan += strlen(scan) + 1;
      }
      populate_pathset(outPaths, paths, count);
      free(paths);
      return NFD_OKAY;
    }
  }
  return NFD_CANCEL;
#endif
}

nfdresult_t NFD_OpenDialog(const nfdchar_t *filterList,
                           const nfdchar_t *defaultPath,
                           nfdchar_t **outPath) {
  if (!outPath) return NFD_ERROR;
  *outPath = NULL;
  nfdpathset_t pathSet;
  nfdresult_t res = NFD_OpenDialogMultiple(filterList, defaultPath, &pathSet);
  if (res == NFD_OKAY && pathSet.count > 0) {
    nfdchar_t *first = NFD_PathSet_GetPath(&pathSet, 0);
    if (first) {
      *outPath = strdup(first);
    }
    NFD_PathSet_Free(&pathSet);
    return NFD_OKAY;
  }
  NFD_PathSet_Free(&pathSet);
  return res;
}
