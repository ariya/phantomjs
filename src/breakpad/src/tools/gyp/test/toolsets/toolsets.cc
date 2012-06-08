const char *GetToolset() {
#ifdef TARGET
  return "Target";
#else
  return "Host";
#endif
}
