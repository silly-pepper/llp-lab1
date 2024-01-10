#include "database.h"
#include "utils.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

void test_insertions() {
    fprintf(stderr, "Testing insertions... ");

    Database* db = database_create_database("test_insertions", 1024);
    Value v;
    v.int_value = 42;
    Leaf* l1i = database_create_leaf(db, NULL, "L1i", INT, v);
    ASSERT_TRUE(l1i);
    EXPECT_TRUE(database_get_leaf_value(db, l1i)->int_value == 42);
    char* s = "abc";
    String ss;
    ss.size = 3;
    ss.data = s;
    v.str_value = ss;
    Leaf* l1s = database_create_leaf(db, NULL, "L1s", STR, v);
    ASSERT_TRUE(l1s);
    EXPECT_TRUE(database_get_leaf_value(db, l1s)->str_value.size == 3);
    EXPECT_TRUE(strcmp(database_get_leaf_value(db, l1s)->str_value.data, "abc") == 0);
    Directory* l1d = database_create_directory(db, NULL, "L1d");
    ASSERT_TRUE(l1d);
    Directory* l2d1 = database_create_directory(db, l1d, "L2d1");
    ASSERT_TRUE(l2d1);
    Directory* l2d2 = database_create_directory(db, l1d, "L2d2");
    ASSERT_TRUE(l2d2);
    v.float_value = 0.42f;
    Leaf* l2f = database_create_leaf(db, l1d, "L2f", FLOAT, v);
    ASSERT_TRUE(l2f);
    EXPECT_TRUE(fabsf(database_get_leaf_value(db, l2f)->float_value - 0.42f) < 0.000001);
    Directory* l3d = database_create_directory(db, l2d1, "L3d");
    ASSERT_TRUE(l3d);

    database_destroy_database(db);

    fprintf(stderr, "OK\n");
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedValue"
#pragma ide diagnostic ignored "UnusedLocalVariable"
void test_deletions() {
    fprintf(stderr, "Testing deletions... ");

    Database* db = database_create_database("test_deletions", 1024);
    Value v;
    v.int_value = 42;
    Leaf* l1i = database_create_leaf(db, NULL, "L1i", INT, v);
    char* s = "abc";
    String ss;
    ss.size = 3;
    ss.data = s;
    v.str_value = ss;
    Leaf* l1s = database_create_leaf(db, NULL, "L1s", STR, v);
    Directory* l1d = database_create_directory(db, NULL, "L1d");
    Directory* l2d1 = database_create_directory(db, l1d, "L2d1");
    Directory* l2d2 = database_create_directory(db, l1d, "L2d2");
    v.float_value = 0.42f;
    Leaf* l2f = database_create_leaf(db, l1d, "L2f", FLOAT, v);
    Directory* l3d = database_create_directory(db, l2d1, "L3d");

    EXPECT_TRUE(database_delete_directory(db, l3d));
    EXPECT_FALSE(database_create_directory(db, l3d, "l3d_is_invalid_parent"));
    EXPECT_FALSE(database_delete_directory(db, l1d));
    EXPECT_TRUE(database_create_directory(db, l1d, "l1d_is_correct_parent"));
    EXPECT_TRUE(database_delete_leaf(db, l2f));

    database_destroy_database(db);

    fprintf(stderr, "OK\n");
}
#pragma clang diagnostic pop

void example() {
    Database* db = database_create_database("example", 1024);
    Value v;
    v.int_value = 42;
    Leaf* l1i = database_create_leaf(db, NULL, "L1i", INT, v);
    fprintf(stderr, "Created L1i. null: %d, value: %d\n",
            l1i == NULL, (l1i == NULL ? -1 : database_get_leaf_value(db, l1i)->int_value));
    char* s = "abc";
    String ss;
    ss.size = 3;
    ss.data = s;
    v.str_value = ss;
    Leaf* l1s = database_create_leaf(db, NULL, "L1s", STR, v);
    fprintf(stderr, "Created L1s. null: %d, value: %s\n",
            l1s == NULL, (l1s == NULL ? "null" : database_get_leaf_value(db, l1s)->str_value.data));
    Directory* l1d = database_create_directory(db, NULL, "L1d");
    fprintf(stderr, "Created L1d. null: %d\n", l1d == NULL);
    Directory* l2d1 = database_create_directory(db, l1d, "L2d1");
    fprintf(stderr, "Created L2d1. null: %d\n", l2d1 == NULL);
    Directory* l2d2 = database_create_directory(db, l1d, "L2d2");
    fprintf(stderr, "Created L2d2. null: %d\n", l2d2 == NULL);
    v.float_value = 0.42f;
    Leaf* l2f = database_create_leaf(db, l1d, "L2f", FLOAT, v);
    fprintf(stderr, "Created L2f. null: %d, value: %f\n",
            l2f == NULL, (l2f == NULL ? -1.0f : database_get_leaf_value(db, l2f)->float_value));
    Directory* l3d = database_create_directory(db, l2d1, "L3d");
    fprintf(stderr, "Created L3d. null: %d\n", l3d == NULL);

    fprintf(stderr, "Finished setup.\n");
    database_traverse_and_print_database(db);

    bool res = database_delete_directory(db, l3d);
    fprintf(stderr, "Deleted L3d: %d\n", res);
    database_traverse_and_print_database(db);

    res = database_delete_directory(db, l1d);
    fprintf(stderr, "Deleted L1d: %d\n", res);

    res = database_delete_leaf(db, l2f);
    fprintf(stderr, "Deleted L2f: %d\n", res);
    database_traverse_and_print_database(db);

    database_shutdown_database(db);
}

int main() {
    fprintf(stderr, "Running example...\n");
    example();
    fprintf(stderr, "\nRunning tests...\n");
    test_insertions();
    test_deletions();
    return 0;
}
