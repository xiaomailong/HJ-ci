
/*
 * CI version identification scheme.
 */

/* Values for CI_RELEASE_LEVEL */
#define CI_RELEASE_LEVEL_ALPHA    0xA
#define CI_RELEASE_LEVEL_BETA     0xB
#define CI_RELEASE_LEVEL_GAMMA    0xC     /* For release candidates */
#define CI_RELEASE_LEVEL_FINAL    0xF     /* Serial should be 0 here */
                    /* Higher for patch releases */

/* Version parsed out into numeric values */
#define CI_MAJOR_VERSION        0
#define CI_MINOR_VERSION        1
#define CI_MICRO_VERSION        1
#define CI_RELEASE_LEVEL        CI_RELEASE_LEVEL_ALPHA
#define CI_RELEASE_SERIAL       23

/* Version as a string */
#define CI_VERSION          "0.1.2"

/* Version as a single 4-byte hex number, e.g. 0x010502B2 == 1.5.2b2.
   Use this for numeric comparisons, e.g. #if CI_VERSION_HEX >= ... */
#define CI_VERSION_HEX ((CI_MAJOR_VERSION << 24) | \
                        (CI_MINOR_VERSION << 16) | \
                        (CI_MICRO_VERSION <<  8) | \
                        (CI_RELEASE_LEVEL <<  4) | \
                        (CI_RELEASE_SERIAL << 0))
