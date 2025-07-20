/*
 * Locate the first NUL (\0) byte within the given character array.
 *
 *  arr      - character array to inspect
 *  max_len  - maximum number of bytes that may be examined
 *
 * The macro iterates up to @max_len characters looking for a NUL byte and
 * returns a pointer to that byte if found, or NULL otherwise.  This is used
 * by many of the validation macros below to ensure a VARCHAR contains a
 * proper C style string.
 */
#define FIND_FIRST_NUL_BYTE(arr, max_len)      \
    ({                                         \
        char *found = NULL;                    \
        for (size_t i = 0; i < (max_len); ++i) {\
            if ((arr)[i] == '\0') {            \
                found = &(arr)[i];             \
                break;                         \
            }                                  \
        }                                      \
        found;                                 \
    })

/*
 * Initialise a VARCHAR structure to an empty value.  The length field is
 * reset to zero and the character buffer is cleared.
 */
#define VARCHAR_INIT(v)                        \
    do {                                       \
        (v).len = 0;                           \
        if (sizeof((v).arr) > 0) {             \
            memset((v).arr, 0x00, sizeof((v).arr)); \
        }                                      \
    } while (0)

/*
 * Validate that the .len field does not exceed the capacity of the character
 * array.  If the length is invalid the macro prints a diagnostic message and
 * aborts the program.  This is intended for debugging and catching programming
 * errors early.
 */
#define VARCHAR_CHECK_LEN(v)                   \
    do {                                       \
        if ((v).len >= sizeof((v).arr)) {      \
            fprintf(stderr, "Line %d, varchar %s : .len %d >= %lu sizeof(.arr)\n",\
                 __LINE__, #v, (v).len, sizeof((v).arr)); \
            abort();                           \
            { int *p = NULL; *p = 0; }         \
        }                                      \
    } while (0)

/*
 * Ensure that the character array of the VARCHAR contains at least one NUL
 * terminator within its allocated space.  If none is found the macro prints an
 * error and aborts.  This helps detect cases where a buffer returned from the
 * database is not properly terminated.
 */
#define VARCHAR_CHECK_STR(v)                   \
    do {                                       \
        unsigned siz = sizeof((v).arr);        \
        char *nul = FIND_FIRST_NUL_BYTE((v).arr, siz); \
        if (nul == NULL) {                     \
            fprintf(stderr, "Line %d : VARCHAR_CHECK_STR(%s) : No NUL byte found within %u sizeof(.arr) bytes\n",\
                 __LINE__, #v, siz);           \
            abort();                           \
            { int *p = NULL; *p = 0; }         \
        }                                      \
    } while (0)

/*
 * Convenience wrapper that performs both VARCHAR_CHECK_LEN and
 * VARCHAR_CHECK_STR on the supplied variable.
 */
#define VARCHAR_CHECK(v)                       \
    do {                                       \
        VARCHAR_CHECK_LEN(v);                  \
        VARCHAR_CHECK_STR(v);                  \
    } while (0)

/*
 * Set the .len field from the current NUL terminated string stored in .arr.
 * This should be used when a VARCHAR buffer has been manipulated as a C string
 * and you now want a valid length value.  The function verifies that the string
 * actually fits inside the buffer before assigning the length.
 */
#define VARCHAR_ZSETLEN(v)                     \
    do {                                       \
        unsigned len = strlen((v).arr);        \
        unsigned max = sizeof((v).arr);        \
        if (len >= max) {                      \
            fprintf(stderr, "Line %d : VARCHAR_ZSETLEN(%s) : strlen(.arr) %u >= %u sizeof(.arr)\n",\
                __LINE__, #v, len, max );      \
            abort();                           \
            { int *p = NULL; *p = 0; }         \
        }                                      \
        (v).len = len;                         \
    } while (0)

/*
 * Ensure the VARCHAR buffer is NUL terminated at the position indicated by its
 * length.  This is typically used for buffers returned from the database which
 * may not include a terminating NUL.  If .len would write beyond the end of the
 * array the macro aborts.
 */
#define VARCHAR_SETLENZ(v)                     \
    do {                                       \
        unsigned max = sizeof((v).arr);        \
        if ((v).len >= max) {                  \
            fprintf(stderr, "Line %d : VARCHAR_SETLENZ(%s) : .len %u >= %u sizeof(.arr)\n",\
                __LINE__, #v, (v).len, max );  \
            abort();                           \
            { int *p = NULL; *p = 0; }         \
        }                                      \
        (v).arr[(v).len] = '\0';               \
    } while (0)

// Copy one VARCHAR to another VARCHAR
/*
 * Copy the contents of one VARCHAR into another.  The destination must be at
 * least as large as the source.  If not, the macro aborts.  The copied value is
 * always NUL terminated in the destination buffer.
 */
#define VARCHAR_COPY(dst, src)                 \
    do {                                       \
        unsigned len = (src).len;              \
        unsigned max = sizeof((dst).arr);      \
        if (len >= max) {                      \
            fprintf(stderr, "Line %d : VARCHAR_COPY %s <- %s : src.len %u >= %u sizeof(dst.arr)\n",\
                __LINE__, #dst, #src, (src).len, max); \
            abort();                           \
            { int *p = NULL; *p = 0; }         \
        }                                      \
        memcpy((dst).arr, (src).arr, len);     \
        (dst).len = (src).len;                 \
        (dst).arr[(dst).len] = '\0';           \
    } while (0)

// Copy a C String into a VARCHAR
/*
 * Populate a VARCHAR from a regular C string.  The destination must have
 * enough space to hold the source including the terminating NUL.  On success
 * the length field is updated to match the copied string.
 */
#define VARCHAR_COPY_IN(v, src)                \
    do {                                       \
        unsigned len = strlen(src);            \
        unsigned siz = sizeof((v).arr);        \
        if (len >= siz) {                      \
            fprintf(stderr, "Line %d, VARCHAR_COPY_IN %s <- %s : sizeof(dst.arr) %u <= %u strlen(src)\n",\
               __LINE__, #v, #src, siz, len);  \
            abort();                           \
            { int *p = NULL; *p = 0; }         \
        }                                      \
        strcpy((v).arr, (src));                \
        VARCHAR_ZSETLEN(v);                    \
    } while (0)

// Copy a VARCHAR out to a C String
// Note: dst must be a character array, not a character pointer
/*
 * Copy the contents of a VARCHAR into a plain character array.  The destination
 * must be a fixed size array rather than a pointer so that sizeof(dst) yields
 * the available space.  The copied string is always NUL terminated.
 */
#define VARCHAR_COPY_OUT(dst, v)               \
    do {                                       \
        VARCHAR_CHECK_LEN(v);                  \
        unsigned len = (v).len;                \
        unsigned siz = sizeof(dst);            \
        if (len >= siz) {                      \
            fprintf(stderr, "Line %d, VARCHAR_COPY_OUT %s <- %s : sizeof(dst) %u <= %u src.len\n",\
                __LINE__, #dst, #v, siz, len); \
            abort();                           \
            { int *p = NULL; *p = 0; }         \
        }                                      \
        memcpy((dst), (v).arr, len);           \
        (dst)[len] = '\0';                     \
    } while (0)
