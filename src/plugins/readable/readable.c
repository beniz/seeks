#define _BSD_SOURCE
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#undef _BSD_SOURCE

#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <libxml/xpath.h>

#ifdef READABLE_USE_LIBICU
#include <unicode/uregex.h>
#else
#include <pcre.h>
#endif

#include "rd_list.h"
#include "khash.h"
#include "readable.h"

#define UNLIKELY_CANDIDATES_RE "combx|comment|community|disqus|extra|foot|header|menu|remark|rss|shoutbox|sidebar|sponsor|ad-break|agegate|pagination|pager|popup|jobs|selector"
#define OK_MAYBE_ITS_A_CANDIDATE_RE "and|article|body|column|main|shadow"
#define POSITIVE_SCORE_RE "article|body|content|entry|hentry|main|page|pagination|post|text|blog|story"
#define NEGATIVE_SCORE_RE "combx|comment|com-|contact|foot|footer|footnote|masthead|media|meta|outbrain|promo|related|scroll|shoutbox|sidebar|sponsor|shopping|tags|tool|widget"
#define SMALL_TEXT_RE "\\.( |$)"
#define VIDEO_RE "http:\\/\\/(www\\.)?(youtube|vimeo)\\.com"
#define UNLIKELY_ARTICLE_IMAGE_RE "[/_\\-]icon[/_\\-]"

#ifdef MAX
#undef MAX
#endif
#define MAX(a,b) ({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b; })

#ifdef MIN
#undef MIN
#endif
#define MIN(a,b) ({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b; })


KHASH_MAP_INIT_STR(str, int);
#define __pointer_hash(x) (uintptr_t)(x)
#define __pointer_equal(x, y) (x == y)
KHASH_INIT(score, htmlNodePtr, float, 1, __pointer_hash, __pointer_equal);

#ifdef READABLE_USE_LIBICU
UChar *uastrdup(const char *s)
{
    int len = strlen(s);
    UChar *us = malloc(sizeof(UChar) * (len + 1));
    u_uastrcpy(us, s);
    return us;
}
#define RD_RE URegularExpression
#define matches(re, s) ({ \
    UErrorCode status = U_ZERO_ERROR; \
    UChar *cs = uastrdup(s); \
    uregex_setText(re, cs, -1, &status); \
    UBool result = uregex_find(re, 0, &status); \
    uregex_reset(re, 0, &status); \
    free(cs); \
    result; \
})
#define INITIALIZE_RE(re, s) do { \
    UParseError pe; \
    UErrorCode status = U_ZERO_ERROR; \
    re = uregex_openC(s, UREGEX_CASE_INSENSITIVE, &pe, &status); \
    if (U_FAILURE(status)) { \
        fprintf(stderr, "Error at offset %d compiling %s: %s", \
            pe.offset, s, u_errorName(status)); \
        exit(1); \
    } \
} while (0)
#define FINALIZE_RE(re) uregex_close(re)
#else
#define RD_RE pcre
#define matches(re, s)  (pcre_exec(re, NULL, (char *)s, strlen((char *)s), 0, 0, NULL, 0) >= 0)
#define INITIALIZE_RE(pointer, re) do { \
    const char *error = NULL; \
    int erroroffset; \
    pointer = pcre_compile(re, PCRE_CASELESS | PCRE_UTF8, &error, &erroroffset, NULL); \
    if (pointer == NULL) { \
        fprintf(stderr, "Error at offset %d compiling %s: %s", \
                erroroffset, re, error); \
        exit(1); \
    } \
} while(0)
#define FINALIZE_RE(re) pcre_free(re)
#endif

static RD_RE *UNLIKELY_CANDIDATES = NULL;
static RD_RE *OK_MAYBE_ITS_A_CANDIDATE = NULL;
static RD_RE *POSITIVE_SCORE = NULL;
static RD_RE *NEGATIVE_SCORE = NULL;
static RD_RE *SMALL_TEXT = NULL;
static RD_RE *VIDEO = NULL;
static RD_RE *UNLIKELY_ARTICLE_IMAGE = NULL;

#ifdef READABLE_DEBUG
#define DEBUG_LOG(...) fprintf(stderr, __VA_ARGS__)
char *
node_debug_name(htmlNodePtr node)
{
    char buf[2048];
    memset(buf, '\0', sizeof(buf));
    strcat(buf, (char *)node->name);
    xmlChar *node_id = xmlGetProp(node, BAD_CAST "id");
    if (node_id) {
        strcat(buf, "#");
        strcat(buf, (char *)node_id);
        xmlFree(node_id);
    }
    xmlChar *node_class = xmlGetProp(node, BAD_CAST "class");
    if (node_class) {
        strcat(buf, ".");
        strcat(buf, (char *)node_class);
        xmlFree(node_class);
    }
    sprintf(buf + strlen(buf), "@%p", node);
    return strdup(buf);
}
#else
#define DEBUG_LOG(...)
#endif


#define lookup_count(tbl, key, def) ({ khiter_t ii = kh_get_str(tbl, key); (ii == kh_end(tbl)) ? 0 : kh_value(tbl, ii); })
#define lookup_score(tbl, key) ({ khiter_t ii = kh_get_score(tbl, key); (ii == kh_end(tbl)) ? 0 : kh_value(tbl, ii); })
#define lookup_score_ptr(tbl, key) ({ khiter_t ii = kh_get_score(tbl, key); (ii == kh_end(tbl)) ? NULL : &(kh_value(tbl, ii)); })

__attribute__ ((constructor)) void
initialize_regexps(void)
{
    INITIALIZE_RE(UNLIKELY_CANDIDATES, UNLIKELY_CANDIDATES_RE);
    INITIALIZE_RE(OK_MAYBE_ITS_A_CANDIDATE, OK_MAYBE_ITS_A_CANDIDATE_RE);
    INITIALIZE_RE(POSITIVE_SCORE, POSITIVE_SCORE_RE);
    INITIALIZE_RE(NEGATIVE_SCORE, NEGATIVE_SCORE_RE);
    INITIALIZE_RE(SMALL_TEXT, SMALL_TEXT_RE);
    INITIALIZE_RE(VIDEO, VIDEO_RE);
    INITIALIZE_RE(UNLIKELY_ARTICLE_IMAGE, UNLIKELY_ARTICLE_IMAGE_RE);
}

__attribute__ ((destructor)) void
finalize_regexps(void)
{
    FINALIZE_RE(UNLIKELY_CANDIDATES);
    FINALIZE_RE(OK_MAYBE_ITS_A_CANDIDATE);
    FINALIZE_RE(POSITIVE_SCORE);
    FINALIZE_RE(NEGATIVE_SCORE);
    FINALIZE_RE(SMALL_TEXT);
    FINALIZE_RE(VIDEO);
    FINALIZE_RE(UNLIKELY_ARTICLE_IMAGE);
#ifdef READABLE_USE_LIBICU
    u_cleanup();
#endif
}

void
readable_error_handler(void *userdata, xmlErrorPtr error)
{
    if (error->level == XML_ERR_FATAL) {
        fprintf(stderr, "Error on %s:%d->%s", error->file, error->line, error->message);
    }
}

__attribute__ ((constructor)) void
initialize_error_handler(void)
{
    xmlSetStructuredErrorFunc(NULL, readable_error_handler);
}

void
xfree(void *p)
{
    if (p) {
        free(p);
    }
}

char *
node_test_name(htmlNodePtr node)
{
    xmlChar *node_class = xmlGetProp(node, BAD_CAST "class");
    xmlChar *node_id = xmlGetProp(node, BAD_CAST "id");
    int class_len;
    int len;
    char *val = NULL;
    if (node_class && node_id) {
        class_len = xmlStrlen(node_class);
        len = class_len + xmlStrlen(node_id) + 1;
        val = malloc(len + 1);
        strncpy(val, (char *)node_class, len);
        strncpy(val + class_len, (char *)node_id, len - class_len);
        val[len] = '\0';
    } else if (node_class) {
        val = strdup((const char *)node_class);
    } else if (node_id) {
        val = strdup((const char *)node_id);
    }
    xfree(node_class);
    xfree(node_id);
    return val;
}

bool
should_alter_to_p(htmlNodePtr node)
{
    for(htmlNodePtr cur = node->children; cur; cur = cur->next) {
        if (xmlStrEqual(cur->name, BAD_CAST "a") ||
            xmlStrEqual(cur->name, BAD_CAST "blockquote") ||
            xmlStrEqual(cur->name, BAD_CAST "dl") ||
            xmlStrEqual(cur->name, BAD_CAST "div") ||
            xmlStrEqual(cur->name, BAD_CAST "ol") ||
            xmlStrEqual(cur->name, BAD_CAST "p") ||
            xmlStrEqual(cur->name, BAD_CAST "pre") ||
            xmlStrEqual(cur->name, BAD_CAST "table") ||
            xmlStrEqual(cur->name, BAD_CAST "ul")) {
                return false;
        }
        bool val = should_alter_to_p(cur);
        if (!val) {
            return false;
        }
    }
    return true;
}

void
node_inner_text_recursive(htmlNodePtr node, char **data,
        ssize_t *data_size, ssize_t *data_allocated_size)
{
    if (node->type == XML_TEXT_NODE && node->content) {
        ssize_t available_size = *data_allocated_size - *data_size - 1;
        ssize_t len = xmlStrlen(node->content);
        if (len > available_size) {
            while (len > available_size) {
                *data_allocated_size = MAX(*data_allocated_size * 1.2, 512);
                available_size = *data_allocated_size - *data_size - 1;
            }
            *data = realloc(*data, *data_allocated_size);
        }
        strncpy(*data + *data_size, (const char*)node->content, len);
        *data_size += len;
        (*data)[*data_size] = '\0';
    }
    for (htmlNodePtr cur = node->children; cur; cur = cur->next) {
        node_inner_text_recursive(cur, data, data_size, data_allocated_size);
    }
}

char *
node_inner_text(htmlNodePtr node)
{
    ssize_t data_size = 0;
    ssize_t data_allocated_size = 0;
    char *data = NULL;
    node_inner_text_recursive(node, &data, &data_size,
                            &data_allocated_size);
    return data;
}

char *
node_html(htmlDocPtr doc, htmlNodePtr node)
{
    char *retval;
    xmlBufferPtr buffer = xmlBufferCreateSize(1024);
    xmlOutputBufferPtr outputBuffer = xmlOutputBufferCreateBuffer(buffer, NULL);
    htmlNodeDumpFormatOutput(outputBuffer, doc, node, "UTF-8", 0);
    xmlOutputBufferFlush(outputBuffer);
    const xmlChar *bufferContents = xmlBufferContent(buffer);
    retval = strdup((char *)bufferContents);
    xmlOutputBufferClose(outputBuffer);
    xmlBufferFree(buffer);
    return retval;
}

char *
node_inner_html(htmlDocPtr doc, htmlNodePtr node)
{
    ssize_t allocated_size = 0;
    ssize_t data_size = 0;
    char *html = NULL;
    for (htmlNodePtr cur = node->children; cur; cur = cur->next) {
        char *cur_html = node_html(doc, cur);
        int len = strlen(cur_html);
        ssize_t available_size = allocated_size - data_size - 1;
        if (len > available_size) {
            while (len > available_size) {
                allocated_size = MAX(allocated_size * 1.2, 512);
                available_size = allocated_size - data_size - 1;
            }
            html = realloc(html, allocated_size);
        }
        strncpy(html + data_size, cur_html, len);
        data_size += len;
        html[data_size] = '\0';
        free(cur_html);
    }
    return html;
}

int
number_of_commas(const char *text)
{
    int c = 0;
    for (const char *p = text; *p; p++) {
        if (*p == ',') {
            c++;
        }
    }
    return c;
}

int
node_text_len(htmlNodePtr node)
{
    int len = 0;
    char *inner_text = node_inner_text(node);
    if (inner_text) {
        len = strlen(inner_text);
        free(inner_text);
    }
    return len;
}

int
node_nospaces_len(htmlNodePtr node)
{
    int nospaces = 0;
    char *node_text = node_inner_text(node);
    if (node_text) {
        for (char *p = node_text; *p; ++p) {
            if (!isspace(*p)) {
                nospaces++;
            }
        }
        free(node_text);
    }
    return nospaces;
}

int
anchors_text_len(htmlNodePtr node)
{
    int len = 0;
    if (xmlStrEqual(node->name, BAD_CAST "a")) {
        len += node_text_len(node);
    }
    for (htmlNodePtr cur = node->children; cur; cur = cur->next) {
        len += anchors_text_len(cur);
    }
    return len;
}

float
node_link_density(htmlNodePtr node)
{
    int text_len = node_text_len(node);
    int anchors_len = anchors_text_len(node);
    if (text_len == 0) {
        return anchors_len;
    }
    return ((float)anchors_len) / text_len;
}

float
node_score(htmlNodePtr node)
{
    const xmlChar *name = node->name;

    if (xmlStrEqual(name, BAD_CAST "div")) {
        return 5;
    }
    if (xmlStrEqual(name, BAD_CAST "pre") ||
        xmlStrEqual(name, BAD_CAST "td") ||
        xmlStrEqual(name, BAD_CAST "blockquote")) {

        return 3;
    }
    if (xmlStrEqual(name, BAD_CAST "address") ||
        xmlStrEqual(name, BAD_CAST "ol") ||
        xmlStrEqual(name, BAD_CAST "ul") ||
        xmlStrEqual(name, BAD_CAST "dl") ||
        xmlStrEqual(name, BAD_CAST "dd") ||
        xmlStrEqual(name, BAD_CAST "dt") ||
        xmlStrEqual(name, BAD_CAST "li") ||
        xmlStrEqual(name, BAD_CAST "form")) {

        return -3;
    }
    if (xmlStrEqual(name, BAD_CAST "h1") ||
        xmlStrEqual(name, BAD_CAST "h2") ||
        xmlStrEqual(name, BAD_CAST "h3") ||
        xmlStrEqual(name, BAD_CAST "h4") ||
        xmlStrEqual(name, BAD_CAST "h5") ||
        xmlStrEqual(name, BAD_CAST "h6") ||
        xmlStrEqual(name, BAD_CAST "th")) {

        return -5;
    }
    return 0;
}

float
name_score(const xmlChar *name)
{
    float score = 0;
    if (matches(POSITIVE_SCORE, name)) {
        score += 25;
    }
    if (matches(NEGATIVE_SCORE, name)) {
        score -= 25;
    }
    return score;
}

float
node_class_score(htmlNodePtr node)
{
    float score = 0;
    xmlChar *node_class = xmlGetProp(node, BAD_CAST "class");
    if (node_class) {
        score += name_score(node_class);
        free(node_class);
    }
    xmlChar *node_id = xmlGetProp(node, BAD_CAST "id");
    if (node_id) {
        score += name_score(node_id);
        free(node_id);
    }
    return score;
}

float *
initialize_node_score(kh_score_t *scores, htmlNodePtr node, int options)
{
    float score = node_score(node);
    if (options & READABLE_OPTION_WEIGHT_CLASSES) {
        score += node_class_score(node);
    }
    int ret;
    khiter_t iter = kh_put_score(scores, node, &ret);
    kh_value(scores, iter) = score;
    return &(kh_value(scores, iter));
}

void
append_readable_node(htmlNodePtr readable, htmlNodePtr node)
{
    if (!xmlStrEqual(node->name, BAD_CAST "div") &&
            !xmlStrEqual(node->name, BAD_CAST "p")) {

        xmlFree((void *)node->name);
        node->name = xmlCharStrdup("div");
    }
    xmlUnlinkNode(node);
    xmlAddChild(readable, node);
}

bool
iframe_looks_like_video(htmlNodePtr node)
{
    bool retval = false;
    xmlChar *src = xmlGetProp(node, BAD_CAST "src");
    if (src ) {
        if (strstr((char *)src, "youtube") ||
            strstr((char *)src, "vimeo")) {

            retval = true;
        }
        free(src);
    }
    return retval;
}

bool
object_looks_like_video(htmlNodePtr node)
{
    for (htmlNodePtr cur = node->children; cur; cur = cur->next) {
        if (xmlStrEqual(cur->name, BAD_CAST "param")) {
            for (xmlAttrPtr attr = cur->properties; attr; attr = attr->next) {
                if (xmlStrEqual(attr->name, BAD_CAST "value") &&
                    attr->children) {

                    xmlChar *value = attr->children->content;
                    if (strstr((char *)value, "youtube") ||
                        strstr((char *)value, "vimeo")) {

                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool
node_looks_like_video(htmlNodePtr node)
{
    if (xmlStrEqual(node->name, BAD_CAST "iframe") ||
        xmlStrEqual(node->name, BAD_CAST "embed")) {

        return iframe_looks_like_video(node);
    } else if (xmlStrEqual(node->name, BAD_CAST "object")) {

        return object_looks_like_video(node);
    }
    return false;
}

void
node_tags_count_recursive(htmlNodePtr node, kh_str_t *tags)
{
    int ret;
    khiter_t iter = kh_get_str(tags, (char *)node->name);
    int count = 1;
    if (iter == kh_end(tags)) {
        iter = kh_put_str(tags, (char *)node->name, &ret);
    } else {
        count = kh_value(tags, iter) + 1;
    }
    kh_value(tags, iter) = count;
    for (htmlNodePtr cur = node->children; cur; cur = cur->next) {
        node_tags_count_recursive(cur, tags);
    }
}

kh_str_t *
node_tags_count(htmlNodePtr node)
{
    kh_str_t *tags = kh_init_str();
    node_tags_count_recursive(node, tags);
    return tags;
}

void
node_video_count(htmlNodePtr node, int *count)
{
    if (node_looks_like_video(node)) {
        ++(*count);
    }
    for (htmlNodePtr cur = node->children; cur; cur = cur->next) {
        node_video_count(cur, count);
    }
}

bool
clean_node_conditionally(htmlNodePtr node, kh_score_t *scores,
                        int *node_images, int options)
{
    float node_score = lookup_score(scores, node);
    float class_score = (options & READABLE_OPTION_WEIGHT_CLASSES) ?
                        node_class_score(node) : 0;
    if (node_score + class_score < 0) {
#ifdef READABLE_DEBUG
        char *debug_name = node_debug_name(node);
        DEBUG_LOG("Removing node %s (score < 0)\n", debug_name);
        free(debug_name);
#endif
        return true;
    }

    bool clean = false;
    int commas = 0;
    int text_len = 0;
    char *node_text = node_inner_text(node);
    if (node_text) {
        commas = number_of_commas(node_text);
        text_len = strlen(node_text);
        free(node_text);
    }
    if (commas < 10) {
        kh_str_t *tags = node_tags_count(node);
        int p_count = lookup_count(tags, "p", 0);
        int img_count = lookup_count(tags, "img", 0);
        int li_count = lookup_count(tags, "li", 0) - 100;
        int input_count = lookup_count(tags, "input", 0);
        float link_density = node_link_density(node);
        if (((li_count > p_count && node->name[0] != '\0' &&
                    node->name[0] != 'o' && node->name[0] != 'u' &&
                    node->name[1] != 'l')) ||
            (input_count > floor(p_count / 3.0)) ||
            (text_len < 25 && (img_count == 0 || img_count > 2)) ||
            (class_score < 25 && link_density > 0.2) ||
            (class_score >= 25 && link_density > 0.5)) {

            clean = true;

#ifdef READABLE_DEBUG
            char *debug_name = node_debug_name(node);
            DEBUG_LOG("Removing node %s (first condition)\n", debug_name);
            DEBUG_LOG("p:%d\timg:%d\tli:%d\tinput:%d\tld:%f\tcs:%f\n",
                        p_count, img_count, li_count, input_count,
                        link_density, class_score);
            free(debug_name);
#endif

        } else if (img_count > p_count && link_density > 0 && text_len > 0) {

            clean = true;

#ifdef READABLE_DEBUG
            char *debug_name = node_debug_name(node);
            DEBUG_LOG("Removing node %s (second condition)\n", debug_name);
            DEBUG_LOG("p:%d\timg:%d\tli:%d\tinput:%d\tld:%f\tcs:%f\n",
                        p_count, img_count, li_count, input_count,
                        link_density, class_score);
            free(debug_name);
#endif
        } else {
            int might_be_video_count = lookup_count(tags, "object", 0) +
                                        lookup_count(tags, "embed", 0) +
                                        lookup_count(tags, "iframe", 0);
            int video_count = 0;
            if (might_be_video_count) {
                node_video_count(node, &video_count);
            }
            if ((video_count == 1 && text_len < 75) || video_count > 1 ||
                (xmlStrEqual(node->name, BAD_CAST "table") && video_count != 1 && img_count != 1)) {

                clean = true;
#ifdef READABLE_DEBUG
                char *debug_name = node_debug_name(node);
                DEBUG_LOG("Removing node %s (third condition)\n", debug_name);
                free(debug_name);
#endif
            }
        }
        *node_images = img_count;
        kh_destroy_str(tags);
    }
    return clean;
}

htmlNodePtr
clean_node(htmlDocPtr doc, htmlNodePtr node, kh_score_t *scores, int options,
            int *nimages)
{
#ifdef READABLE_DEBUG
    char *debug_name = node_debug_name(node);
    DEBUG_LOG("Cleaning node %s\n", debug_name);
    free(debug_name);
#endif
    if (node->name && node->name[0] == 'h') {
        if (xmlStrEqual(node->name, BAD_CAST "h1")) {
            xmlUnlinkNode(node);
            xmlFreeNode(node);
            return NULL;
        }
        if (xmlStrEqual(node->name, BAD_CAST "h2") ||
            xmlStrEqual(node->name, BAD_CAST "h3") ||
            xmlStrEqual(node->name, BAD_CAST "h4")) {

            if (node_class_score(node) < 0 || node_link_density(node) > 0.33) {
                xmlUnlinkNode(node);
                xmlFreeNode(node);
                return NULL;
            }
        }
    }
    /* TODO: If there's only one h2, remove it */
    if (xmlStrEqual(node->name, BAD_CAST "font")) {
        /* Make it a span */
        xmlFree((void *)node->name);
        node->name = xmlCharStrdup("span");
    }
    if (xmlStrEqual(node->name, BAD_CAST "iframe")) {
        if (!iframe_looks_like_video(node)) {
            xmlUnlinkNode(node);
            xmlFreeNode(node);
            return NULL;
        }
    }
    if (xmlStrEqual(node->name, BAD_CAST "img")) {
        if (options & READABLE_OPTION_REMOVE_IMAGES) {
            xmlUnlinkNode(node);
            xmlFreeNode(node);
            return NULL;
        }
        xmlChar *src = xmlGetProp(node, BAD_CAST "src");
        if (!src || !src[0]) {
            xfree(src);
            xmlUnlinkNode(node);
            xmlFreeNode(node);
            return NULL;
        }
        xmlChar *alt = xmlGetProp(node, BAD_CAST "alt");
        xmlChar *title = xmlGetProp(node, BAD_CAST "title");
#define xlen(x) (x ? strlen((char *)x) : 0)
        int len = xlen(alt) + xlen(title) + strlen((char *)src);
        char *test = malloc(len + 1);
        strcpy(test, (char *)src);
        free(src);
        if (alt) {
            strcat(test, (char *)alt);
            free(alt);
        }
        if (title) {
            strcat(test, (char *)title);
            free(title);
        }
        DEBUG_LOG("Testing image %s\n", test);
        int match = matches(UNLIKELY_ARTICLE_IMAGE, test);
        free(test);
        if (match) {
#ifdef READABLE_DEBUG
            char *debug_name = node_debug_name(node);
            DEBUG_LOG("Removing unlikely image %s\n", debug_name);
            free(debug_name);
#endif
            xmlUnlinkNode(node);
            xmlFreeNode(node);
            return NULL;
        }
        ++(*nimages);
    }
    if ((xmlStrEqual(node->name, BAD_CAST "object") ||
        xmlStrEqual(node->name, BAD_CAST "embed")) &&
        !node_looks_like_video(node)) {

        xmlUnlinkNode(node);
        xmlFreeNode(node);
        return NULL;
    }
    if (!htmlTagLookup(node->name)) {
        /* When parsing bad html, a p might end in an
        unknown tag. It won't happen with a div, since
        the parser will always start a new tag for it
        as a sibling of the unknown tag.
        */
        htmlNodePtr cur = node->children;
        while (cur) {
            /* See comment a few lines down, with
            the same bucle structure
            */
            htmlNodePtr next = cur->next;
            if (cur->name[0] == 'p') {
                xmlAddNextSibling(node, cur);
                clean_node(doc, cur, scores, options, nimages);
            }
            cur = next;
        }
        xmlUnlinkNode(node);
        xmlFreeNode(node);
        return NULL;
    }
    htmlNodePtr cur = node->children;
    while(cur) {
        /* Grab the next node first, since clean_node()
            might free the memory associated with the
            current node
        */
        htmlNodePtr next = cur->next;
        if (cur->type == XML_ELEMENT_NODE) {
            clean_node(doc, cur, scores, options, nimages);
        }
        cur = next;
    }
    /* Do these clean operations after removing any children,
        since they depend on the node content which
        might be altered while cleaning it
    */
    if (options & READABLE_OPTION_CLEAN_CONDITIONALLY) {
        if (xmlStrEqual(node->name, BAD_CAST "form") ||
            xmlStrEqual(node->name, BAD_CAST "table") ||
            xmlStrEqual(node->name, BAD_CAST "ul") ||
            xmlStrEqual(node->name, BAD_CAST "div")) {

            int node_images = 0;
            if (clean_node_conditionally(node, scores, &node_images, options)) {

                *nimages -= node_images;
                xmlUnlinkNode(node);
                xmlFreeNode(node);
                return NULL;
            }
        }
    }
    if (node->name[0] == 'p') {
        int nospaces = node_nospaces_len(node);
        if (!nospaces) {
            kh_str_t *tags = node_tags_count(node);
            int img_count = lookup_count(tags, "img", 0);
            int embed_count = lookup_count(tags, "embed", 0);
            int object_count = lookup_count(tags, "object", 0);
            int iframe_count = lookup_count(tags, "iframe", 0);
            kh_destroy_str(tags);
            if (!img_count && !embed_count && !object_count && !iframe_count) {
                xmlUnlinkNode(node);
                xmlFreeNode(node);
                return NULL;
            }
        }
    }
    for (xmlAttrPtr attr = node->properties, next; attr; attr = next) {
        next = attr->next;
        if (xmlStrEqual(attr->name, BAD_CAST "style") ||
            xmlStrEqual(attr->name, BAD_CAST "border") ||
            xmlStrEqual(attr->name, BAD_CAST "class") ||
            xmlStrEqual(attr->name, BAD_CAST "target") ||
            (attr->name[0] == 'o' && attr->name[1] == 'n')) {
            xmlRemoveProp(attr);
        } else if (xmlStrEqual(attr->name, BAD_CAST "href")) {
            if (xmlStrstr(attr->children->content, BAD_CAST "javascript") == attr->children->content) {
                xmlRemoveProp(attr);
            }
        }
    }
    return node;
}

struct rd_list *
add_images_to_list(htmlNodePtr node, struct rd_list *list)
{
    if (xmlStrEqual(node->name, BAD_CAST "img")) {
        list = rd_list_append(list, node);
    }
    for (htmlNodePtr cur = node->children; cur; cur = cur->next) {
        if (cur->type == XML_ELEMENT_NODE) {
            list = add_images_to_list(cur, list);
        }
    }
    return list;
}

htmlNodePtr
search_article_image(htmlNodePtr node, htmlNodePtr prev)
{
    struct rd_list *candidates = NULL;
    kh_score_t *scores = kh_init_score();
    float initial_score = -3;
    int len = 0;
    int ret;
    while (node) {
        for (htmlNodePtr cur = node->children; cur; cur = cur->next) {
            if (cur != prev && cur->type == XML_ELEMENT_NODE) {
                candidates = add_images_to_list(cur, candidates);
                for (; len < rd_list_length(candidates); ++len) {
                    htmlNodePtr image = rd_list_index_data(candidates, len);
                    khiter_t iter = kh_put_score(scores, image, &ret);
                    kh_value(scores, iter) = initial_score;
                }
            }
        }
        /* Reduce the score as we climb the tree */
        initial_score *= 5;
        prev = node;
        node = node->parent;
    }
    htmlNodePtr top_image = NULL;
    float top_score = 0;
    for (int ii = 0; ii < rd_list_length(candidates); ++ii) {
        htmlNodePtr image = rd_list_index_data(candidates, ii);
        float score = lookup_score(scores, image);
        xmlChar *src = xmlGetProp(image, BAD_CAST "src");
        if (!src) {
            continue;
        }
#ifdef READABLE_DEBUG
        char *debug_name = node_debug_name(image);
        DEBUG_LOG("Image candidate %s (%s)\n", debug_name, src);
        free(debug_name);
#endif
        xmlChar *width = xmlGetProp(image, BAD_CAST "width");
        xmlChar *height = xmlGetProp(image, BAD_CAST "height");

        if (matches(UNLIKELY_ARTICLE_IMAGE, src)) {
            score -= 20;
        }
        char *dot = strrchr((char *)src, '.');
        if (dot) {
            for (char *p = dot; *p; ++p) {
                *p = tolower(*p);
            }
            dot++;
            if (strcmp(dot, "jpg") == 0 || strcmp(dot, "jpeg") == 0) {
                score += 15;
            } else if (strcmp(dot, "gif") == 0) {
                score -= 10;
            }
        }
        if (width && height && MIN(atoi((char *)width), atoi((char *)height)) > 150) {
            score += 10;
        }
        if (score > top_score) {
            top_image = image;
            top_score = score;
        }
        xfree(src);
        xfree(width);
        xfree(height);
    }
    if (top_score < 0) {
        top_image = NULL;
    }
    if (top_image) {
        xmlAttrPtr attr = top_image->properties;
        while (attr) {
            xmlAttrPtr next = attr->next;
            if (!xmlStrEqual(attr->name, BAD_CAST "width") &&
                !xmlStrEqual(attr->name, BAD_CAST "height") &&
                !xmlStrEqual(attr->name, BAD_CAST "src") &&
                !xmlStrEqual(attr->name, BAD_CAST "alt")) {

                xmlRemoveProp(attr);
            }
            attr = next;
        }
    }
    rd_list_free(candidates);
    kh_destroy_score(scores);
    return top_image;
}

htmlNodePtr
score_nodes(htmlNodePtr node, struct rd_list **list, int options)
{
    /* If list == NULL, we're working with the root node,
        so we shouldn't remove it
    */
    if ((options & READABLE_OPTION_STRIP_UNLIKELYS) && *list) {
        char *test_name = node_test_name(node);
        if (test_name) {
            if (matches(UNLIKELY_CANDIDATES, test_name) &&
                !matches(OK_MAYBE_ITS_A_CANDIDATE, test_name)) {

#ifdef READABLE_DEBUG
                char *debug_name = node_debug_name(node);
                DEBUG_LOG("Removing unlikely candidate %s\n", debug_name);
                free(debug_name);
#endif
                htmlNodePtr next = node->next;
                xmlUnlinkNode(node);
                xmlFreeNode(node);
                free(test_name);
                return next;
            }
            free(test_name);
        }
    }
    if (xmlStrEqual(node->name, BAD_CAST "p") ||
        xmlStrEqual(node->name, BAD_CAST "td") ||
        xmlStrEqual(node->name, BAD_CAST "pre")) {

        *list = rd_list_append(*list, node);
    }
    if (xmlStrEqual(node->name, BAD_CAST "div")) {
        if (should_alter_to_p(node)) {
#ifdef READABLE_DEBUG
            char *debug_name = node_debug_name(node);
            DEBUG_LOG("Altering div %s to p \n", debug_name);
            free(debug_name);
#endif
            xmlFree((void *)node->name);
            node->name = xmlCharStrdup("p");
            *list = rd_list_append(*list, node);
        }
    }

    if (xmlStrEqual(node->name, BAD_CAST "script") ||
        xmlStrEqual(node->name, BAD_CAST "noscript") ||
        xmlStrEqual(node->name, BAD_CAST "style")) {

        htmlNodePtr next = node->next;
        xmlUnlinkNode(node);
        xmlFreeNode(node);
        return next;
    }

    if (node->type == XML_COMMENT_NODE) {
        htmlNodePtr next = node->next;
        xmlUnlinkNode(node);
        xmlFreeNode(node);
        return next;
    }

    if (xmlStrEqual(node->name, BAD_CAST "link")) {
        xmlChar *rel = xmlGetProp(node, BAD_CAST "rel");
        if (rel) {
            if (xmlStrstr(rel, BAD_CAST "css")) {

                xmlFree(rel);
                htmlNodePtr next = node->next;
                xmlUnlinkNode(node);
                xmlFreeNode(node);
                return next;
            }
            xmlFree(rel);
        }
    }
    if (xmlStrEqual(node->name, BAD_CAST "br") && node->next &&
        xmlStrEqual(node->next->name, BAD_CAST "br") &&
        (node->parent->name[0] == 'p' || xmlStrEqual(node->parent->name, BAD_CAST "div"))) {

        bool parent_is_p = node->parent->name[0] == 'p';
        DEBUG_LOG("Merging brs\n");
        if (node->prev && node->prev->type == XML_TEXT_NODE &&
            !parent_is_p) {

            /* There's some text before this double br and the parent
            is not a p. In order to count this node as a candidate and
            evaluate its score correctly, we need to put this text
            inside a p
            */
            htmlNodePtr p = xmlNewNode(NULL, BAD_CAST "p");
            xmlAddPrevSibling(node, p);
            *list = rd_list_append(*list, p);
            for (htmlNodePtr prev, cur = p->prev; cur; cur = prev) {
                prev = cur->prev;
                if (cur->type != XML_TEXT_NODE) {
                    const htmlElemDesc *desc = htmlTagLookup(cur->name);
                    if (!desc || !desc->isinline) {
                        break;
                    }
                }
                xmlUnlinkNode(cur);
                if (p->children) {
                    xmlAddPrevSibling(p->children, cur);
                } else {
                    xmlAddChild(p, cur);
                }
            }
        }
        /* Close the previous p and start a new one */
        htmlNodePtr p = xmlNewNode(NULL, BAD_CAST "p");
        if (parent_is_p) {
            xmlAddNextSibling(node->parent, p);
        } else {
            xmlAddPrevSibling(node, p);
        }
        *list = rd_list_append(*list, p);
        for (htmlNodePtr cur = node, next; cur; cur = next) {
            next = cur->next;
            if (xmlStrEqual(cur->name, BAD_CAST "br")) {
                xmlUnlinkNode(cur);
                xmlFreeNode(cur);
            } else {
                const htmlElemDesc *desc = htmlTagLookup(cur->name);
                if (cur->type == XML_TEXT_NODE || (desc && desc->isinline)) {
                    xmlUnlinkNode(cur);
                    xmlAddChild(p, cur);
                } else {
                    break;
                }
            }
        }
        return p;
    }
#if 0
    /* TODO */
    if (node->children == node->last) { /* Just 1 children */
        HtmlNodePtr child = node->children;
        if (xmlStrEqual(child->name, BAD_CAST "p")) {
        }
    }
#endif
    htmlNodePtr cur = node->children;
    while (cur) {
        cur = score_nodes(cur, list, options);
    }
    return node->next;
}

bool
style_px_dimensions(xmlChar *style, int *width, int *height)
{
    const char *wp = strstr((char *)style, "width:");
    const char *hp = strstr((char *)style, "height:");
    if (wp && hp) {
        while(!isdigit(*wp) && *wp) {
            wp++;
        }
        while(!isdigit(*hp) && *hp) {
            hp++;
        }
        if (*wp && *hp) {
            char *wep = NULL;
            char *hep = NULL;
            *width = strtol(wp, &wep, 10);
            *height = strtol(hp, &hep, 10);
            if (wep && hep && *wep == 'p' && *hep == 'p' &&
                *width && *height) {

                return true;
            }
        }
    }
    return false;
}

void
search_multimedia_candidates(htmlNodePtr node,
                struct rd_list **image_candidates,
                struct rd_list **video_candidates,
                int *image_candidates_count,
                int *video_candidates_count)
{
    if (node->name[0] == 'i' || node->name[0] == 'o' || node->name[0] == 'e') {
        if (xmlStrEqual(node->name, BAD_CAST "img")) {
            if (xmlHasProp(node, BAD_CAST "src")) {

                int img_width = 0;
                int img_height = 0;
                xmlChar *width = xmlGetProp(node, BAD_CAST "width");
                if (width) {
                    img_width = atoi((char *)width);
                    xmlFree(width);
                }
                xmlChar *height = xmlGetProp(node, BAD_CAST "height");
                if (height) {
                    img_height = atoi((char *)height);
                    xmlFree(height);
                }
                if (!img_height || !img_width) {
                    xmlChar *style = xmlGetProp(node, BAD_CAST "style");
                    if (style) {
                        if (style_px_dimensions(style, &img_width,
                            &img_height)) {

                            xmlChar buf[256];
                            snprintf((char *)buf, sizeof(buf), "%d", img_width);
                            xmlSetProp(node, BAD_CAST "width", buf);
                            snprintf((char *)buf, sizeof(buf), "%d", img_height);
                            xmlSetProp(node, BAD_CAST "height", buf);
                        }
                        xmlFree(style);
                    }
                }
                if (MIN(img_width, img_height) > 400) {
                    *image_candidates = rd_list_append(*image_candidates, node);
                    ++(*image_candidates_count);
                }
            }
        } else if (xmlStrEqual(node->name, BAD_CAST "iframe") ||
                xmlStrEqual(node->name, BAD_CAST "embed")) {

            if (iframe_looks_like_video(node)) {
                *video_candidates = rd_list_append(*video_candidates, node);
                ++(*video_candidates_count);
            }
        } else if (xmlStrEqual(node->name, BAD_CAST "object")) {

            if (object_looks_like_video(node)) {

                *video_candidates = rd_list_append(*video_candidates, node);
                ++(*video_candidates_count);
            }
        }
    }
    for (htmlNodePtr cur = node->children; cur; cur = cur->next) {
        if (cur->type == XML_ELEMENT_NODE) {
            search_multimedia_candidates(cur, image_candidates,
                    video_candidates, image_candidates_count,
                    video_candidates_count);
        }
    }
}

char *
readable(const char *html, const char *url, const char *encoding, int options)
{
    const xmlChar *input = (const xmlChar *)html;
    htmlDocPtr doc = htmlReadDoc(input, url, encoding, HTML_PARSE_RECOVER);
    char *retval = NULL;
    htmlNodePtr readable_node = NULL;

    if (!doc) {
        return NULL;
    }

    htmlNodePtr root = xmlDocGetRootElement(doc);
    if (!xmlStrEqual(root->name, BAD_CAST "html")) {
        xmlFreeDoc(doc);
        return NULL;
    }
    htmlNodePtr body = NULL;
    for(htmlNodePtr cur = root->children; cur; cur = cur->next) {
        if (xmlStrEqual(cur->name, BAD_CAST "body")) {
            body = cur;
            break;
        }
    }
    if (!body) {
        /* TODO: Create a body */
        xmlFreeDoc(doc);
        return NULL;
    }
    struct rd_list *scorable = NULL;
    score_nodes(body, &scorable, options);
    struct rd_list *candidates = NULL;
    kh_score_t *scores = kh_init_score();
    for (int ii = 0; ii < rd_list_length(scorable); ++ii) {
        htmlNodePtr node = rd_list_index_data(scorable, ii);
        htmlNodePtr parent = node->parent;
        if (!parent) {
            continue;
        }
        char *inner_text = node_inner_text(node);
        if (!inner_text) {
            continue;
        }
        int text_length = strlen(inner_text);
        if (text_length < 25) {
            free(inner_text);
            continue;
        }
        float *parent_score = lookup_score_ptr(scores, parent);
        if (!parent_score) {
            parent_score = initialize_node_score(scores, parent, options);
            candidates = rd_list_append(candidates, parent);
        }
        htmlNodePtr grand_parent = parent->parent;
        float *grand_parent_score = NULL;
        if (grand_parent) {
            grand_parent_score = lookup_score_ptr(scores, parent);
            if (!grand_parent_score) {
                grand_parent_score = initialize_node_score(scores, grand_parent, options);
                candidates = rd_list_append(candidates, grand_parent);
                /* Look up the parent score again, since the hash
                table might have grown and rehased, invalidating
                the pointer
                */
                parent_score = lookup_score_ptr(scores, parent);
            }
        }
        float score = 1;
        score += number_of_commas(inner_text);
        score += MIN(floor(text_length / 100.0), 3);
        int ret;
        *parent_score += score;
        if (grand_parent_score) {
            *grand_parent_score += score / 2;
        }
        free(inner_text);
        /* Don't insert the key until parent_score and
        grand_parent_score have been modified, since
        inserting in the table could invalidate the
        pointers
        */
        khiter_t iter = kh_put_score(scores, node, &ret);
        kh_value(scores, iter) = score;
    }
    DEBUG_LOG("%d candidates\n", rd_list_length(candidates));
    rd_list_free(scorable);
    /* Choose the top candidate */
    htmlNodePtr top_candidate = NULL;
    float *top_candidate_score = NULL;
    for (int ii = 0; ii < rd_list_length(candidates); ++ii) {
        htmlNodePtr node = rd_list_index_data(candidates, ii);
        float *score = lookup_score_ptr(scores, node);
        *score *= (1 - node_link_density(node));
#ifdef READABLE_DEBUG
        char *name = node_test_name(node);
        DEBUG_LOG("Node %s:\tscore %f\t link density%f\n", name, *score,
                    node_link_density(node));
        xfree(name);
#endif
        if (!top_candidate || *score > *top_candidate_score) {
            top_candidate = node;
            top_candidate_score = score;
        }
    }
    rd_list_free(candidates);
    if (!top_candidate || xmlStrEqual(top_candidate->name, BAD_CAST "body") ||
        xmlStrEqual(top_candidate->name, BAD_CAST "html")) {

        top_candidate = xmlNewNode(NULL, BAD_CAST "div");
        xmlAddChild(body, top_candidate);
        for (htmlNodePtr cur = body->children; cur; cur = cur->next) {
            xmlUnlinkNode(cur);
            xmlAddChild(top_candidate, cur);
        }
        int ret;
        khiter_t iter = kh_put_score(scores, top_candidate, &ret);
        kh_value(scores, iter) = 0;
        top_candidate_score = &(kh_value(scores, iter));
    }
#ifdef READABLE_DEBUG
    char *debug_name = node_test_name(top_candidate);
    DEBUG_LOG("Top candidate %s with score %f\n", debug_name, *top_candidate_score);
    free(debug_name);
#endif
    readable_node = xmlNewNode(NULL, BAD_CAST "div");
    xmlSetProp(readable_node, BAD_CAST "id", BAD_CAST "readability-content");
    for (htmlNodePtr cur = top_candidate; cur; cur = cur->parent) {
        xmlChar *dir = xmlGetProp(cur, BAD_CAST "dir");
        if (dir) {
            if (xmlStrcasecmp(dir, BAD_CAST "rtl") == 0) {
                xmlFree(dir);
                htmlNodePtr rtl_node = xmlNewNode(NULL, BAD_CAST "div");
                xmlSetProp(rtl_node, BAD_CAST "dir", BAD_CAST "rtl");
                xmlAddChild(readable_node, rtl_node);
                readable_node = rtl_node;
                break;
            }
            xmlFree(dir);
        }
    }
    /* TODO: RTL text support */
    float threshold = MAX(10, *top_candidate_score * 0.2f);
    xmlChar *top_candidate_class = xmlGetProp(top_candidate, BAD_CAST "class");
    DEBUG_LOG("Threshold %f\n", threshold);
    /* Insert nodes in the article */
    htmlNodePtr start = top_candidate->parent ? : top_candidate;
    htmlNodePtr next;
    for (htmlNodePtr cur = start->children; cur; cur = next) {
        next = cur->next;
        if (cur == top_candidate) {
#ifdef READABLE_DEBUG
            char *debug_name = node_debug_name(cur);
            DEBUG_LOG("Appending %s (top candidate)\n", debug_name);
            free(debug_name);
#endif
            append_readable_node(readable_node, cur);
            continue;
        }
        float bonus = 0;
        xmlChar *cur_class = xmlGetProp(cur, BAD_CAST "class");
        if (top_candidate_class && cur_class && xmlStrEqual(top_candidate_class, cur_class)) {
            bonus = *top_candidate_score * 0.2f;
        }
        xfree(cur_class);
        float *cur_score = lookup_score_ptr(scores, cur);
        if (cur_score && (*cur_score  + bonus) > threshold) {
#ifdef READABLE_DEBUG
            char *debug_name = node_debug_name(cur);
            DEBUG_LOG("Appending %s (score > threshold)\n", debug_name);
            free(debug_name);
#endif
            append_readable_node(readable_node, cur);
            continue;
        }

        if (xmlStrEqual(cur->name, BAD_CAST "p")) {
            float link_density = node_link_density(cur);
            char *inner_text = node_inner_text(cur);
            int text_len = inner_text ? strlen(inner_text) : 0;

            if (text_len > 80 && link_density < 0.25) {
#ifdef READABLE_DEBUG
                char *debug_name = node_debug_name(cur);
                DEBUG_LOG("Appending %s (text with low link density)", debug_name);
                free(debug_name);
#endif
                append_readable_node(readable_node, cur);
            } else if (text_len < 80 && link_density == 0 && inner_text &&
                        matches(SMALL_TEXT, inner_text)) {

#ifdef READABLE_DEBUG
                char *debug_name = node_debug_name(cur);
                DEBUG_LOG("Appending %s (short text without links)", debug_name);
                free(debug_name);
#endif
                append_readable_node(readable_node, cur);
            }
            xfree(inner_text);
        }
    }
    xfree(top_candidate_class);
    /* Might have a div nested for RTL support */
    while(readable_node->parent) {
        readable_node = readable_node->parent;
    }
    if (readable_node) {
        int nimages = 0;
        readable_node = clean_node(doc, readable_node, scores, options, &nimages);
        DEBUG_LOG("Content cleaned, %d images left\n", nimages);
        if (readable_node) {
            if (options & READABLE_OPTION_CHECK_MIN_LENGTH) {
                if (node_nospaces_len(readable_node) < READABLE_MIN_ARTICLE_LENGTH) {
                    DEBUG_LOG("Article length is %d (< %d), discarding\n",
                            node_nospaces_len(readable_node),
                            READABLE_MIN_ARTICLE_LENGTH);
                    xmlFreeNode(readable_node);
                    readable_node = NULL;
                }
            }

            if (readable_node) {
                if ((options & READABLE_OPTION_LOOK_HARDER_FOR_IMAGES) &&
                    nimages == 0) {

                    DEBUG_LOG("Looking harder for images\n");
                    /* Don't pass top_candidate->parent here, since top_candidate
                    has been reparented and now its hanging from readable_node
                    */
                    htmlNodePtr image_node = search_article_image(start, NULL);
                    if (image_node) {
                        xmlUnlinkNode(image_node);
                        if (readable_node->children) {
                            xmlAddPrevSibling(readable_node->children, image_node);
                        } else {
                            xmlAddChild(readable_node, image_node);
                        }
                    }
                }
                /* Create the final HTML */
                if (options & READABLE_OPTION_WRAP_CONTENT) {
                    retval = node_html(doc, readable_node);
                } else {
                    retval = node_inner_html(doc, readable_node);
                }
                xmlFreeNode(readable_node);
            }
        }
    }

    if (!readable_node && (options & READABLE_OPTION_TRY_MULTIMEDIA_ARTICLE)) {
        /* Reload the doc, just in case we
        stripped the multimedia element
        */
        xmlFreeDoc(doc);
        doc = htmlReadDoc(input, url, encoding, HTML_PARSE_RECOVER);
        struct rd_list *image_candidates = NULL;
        struct rd_list *video_candidates = NULL;
        int image_candidates_count = 0;
        int video_candidates_count = 0;
        if (doc && doc->children && doc->children->next) {
            search_multimedia_candidates(doc->children->next,
                                    &image_candidates,
                                    &video_candidates,
                                    &image_candidates_count,
                                    &video_candidates_count);
        }
        DEBUG_LOG("Multimedia candidates: %d images, %d videos\n",
                image_candidates_count, video_candidates_count);
        if (image_candidates_count) {
            if (image_candidates_count < 3) {
                if (!video_candidates_count) {
                    int nimages;
                    if (clean_node(doc, image_candidates->data, scores,
                        options, &nimages)) {

                        htmlNodePtr container = xmlNewNode(NULL,
                                                BAD_CAST "div");
                        xmlAddChild(doc->children->next, container);
                        xmlUnlinkNode(image_candidates->data);
                        xmlAddChild(container, image_candidates->data);
                        retval = node_html(doc, container);
                    }
                } else {
                    rd_list_free(video_candidates);
                }
            } else if (video_candidates_count) {
                /* TODO: Include video if deemed appropiate,
                bet we need better heuristics for determining the
                video candidates
                */
                rd_list_free(video_candidates);
            }
            rd_list_free(image_candidates);
        } else if (video_candidates_count) {
            rd_list_free(video_candidates);
        }
    }
    kh_destroy_score(scores);
    xmlFreeDoc(doc);
    return retval;
}

xmlChar *
find_next_link(htmlDocPtr doc, xmlNodePtr node, const char *url)
{
    xmlChar *href = xmlGetProp(node, BAD_CAST "href");
    if (!href) {
        return NULL;
    }

    if (xmlStrstr(href, BAD_CAST "http://") == href ||
        xmlStrstr(url, BAD_CAST "https://") == href) {
    
        return href;
    }

    char *base = strdup(url);
    char *slash = NULL;
    if (*href == '/') {
        char *scheme = strstr(base, "://");
        slash = strchr(scheme + 3, '/');
    } else {
       slash = strrchr(base, '/');
    }
    if (slash) {
        *slash = '\0';
    }
    size_t len = strlen(base) + xmlStrlen(href);
    char *full_href = malloc(len + 1);
    strncpy(full_href, base, strlen(base) + 1);
    strncat(full_href, (char *)href, xmlStrlen(href));
    full_href[len] = '\0';
    xmlFree(href);
    free(base);
    return (xmlChar *)full_href;
}

char *
next_page_url(const char *html, const char *url,
              const char *encoding)
{
    const xmlChar *input = (const xmlChar *)html;
    htmlDocPtr doc = htmlReadDoc(input, url, encoding, HTML_PARSE_RECOVER);
    xmlChar *next_url = NULL;
    
    if (!doc) {
        return NULL;
    }

    xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(doc);
    xmlXPathObjectPtr anchors = xmlXPathEvalExpression(BAD_CAST "//a", xpath_ctx);
    xmlNodeSetPtr nodes = anchors->nodesetval;
    for (int ii = 0; ii < nodes->nodeNr; ++ii) {
        xmlNodePtr node = nodes->nodeTab[ii];
        xmlChar *rel = xmlGetProp(node, BAD_CAST "rel");
        if (rel) {
            if (xmlStrEqual(rel, BAD_CAST "next")) {
                next_url = find_next_link(doc, node, url);
                if (next_url) {
                    xmlFree(rel);
                    break;
                }
            }
            xmlFree(rel);
        }
        xmlChar *klass = xmlGetProp(node, BAD_CAST "class");
        if (klass) {
            if (xmlStrstr(klass, BAD_CAST "next")) {
                next_url = find_next_link(doc, node, url);
                if (next_url) {
                    xmlFree(klass);
                    break;
                }
            }
            xmlFree(klass);
        }
    }
    xmlXPathFreeObject(anchors);
    xmlXPathFreeContext(xpath_ctx);
    return (char *)next_url;
}

