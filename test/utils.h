#ifndef LLP_LAB1_UTILS_H
#define LLP_LAB1_UTILS_H

#define ASSERT_TRUE(x) do {                                                    \
    if (!(x)) {                                                                \
        fprintf(stderr, "Assertion \"%s\" is not satisfied, stopping.\n", #x); \
        return;                                                                \
    }                                                                          \
} while (0)

#define EXPECT_FALSE(x) do { if ((x)) fprintf(stderr, "Condition \"%s\" is not satisfied.\n", #x); } while (0)

#define EXPECT_TRUE(x) EXPECT_FALSE(!(x))

#endif //LLP_LAB1_UTILS_H
