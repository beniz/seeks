#ifndef readable_h
#define readable_h

#define READABLE_MIN_ARTICLE_LENGTH (25)

#ifdef __cplusplus
extern "C" {
#endif

  enum {
    READABLE_OPTION_STRIP_UNLIKELYS = 1,
    READABLE_OPTION_WEIGHT_CLASSES = 1 << 1,
    READABLE_OPTION_CHECK_MIN_LENGTH = 1 << 2,
    READABLE_OPTION_CLEAN_CONDITIONALLY = 1 << 3,
    READABLE_OPTION_REMOVE_IMAGES = 1 << 4,
    READABLE_OPTION_LOOK_HARDER_FOR_IMAGES = 1 << 5,
    READABLE_OPTION_TRY_MULTIMEDIA_ARTICLE = 1 << 6,
    READABLE_OPTION_WRAP_CONTENT = 1 << 7,
    READABLE_OPTIONS_DEFAULT = (0xFFFF & ~(READABLE_OPTION_REMOVE_IMAGES | READABLE_OPTION_WRAP_CONTENT)),
  };

  char * readable(const char *html, const char *url,
                  const char *encoding, int options);

  char * next_page_url(const char *html, const char *url,
                       const char *encoding);

#ifdef __cplusplus
}
#endif

#endif
