#include "database.h"
#include "utils.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcompound-token-split-by-macro"

#define BENCHMARK_EXEC_TIME(x)                                     \
({                                                                 \
    clock_t _benchmark_begin_clock = clock();                      \
    (x);                                                           \
    ((double)(clock() - _benchmark_begin_clock) / CLOCKS_PER_SEC); \
})


void benchmark_insertions() {
    fprintf(stderr, "Benchmarking many insertions...\n");

    Database* db = database_create_database("benchmark_insertions", 0);
    ASSERT_TRUE(db);
    size_t const NDIRS = 3;
    Directory* dirs[NDIRS];
    dirs[0] = database_create_directory(db, NULL, "dir0");
    ASSERT_TRUE(dirs[0]);
    dirs[1] = database_create_directory(db, NULL, "dir1");
    ASSERT_TRUE(dirs[1]);
    dirs[2] = database_create_directory(db, NULL, "dir2");
    ASSERT_TRUE(dirs[2]);

    for (size_t i = 0, stop = 1; i < 7; ++i, stop *= 10) {
        size_t n_insertions = stop * NDIRS;
        assert(n_insertions <= INT_MAX);
        double total = BENCHMARK_EXEC_TIME({
            for (size_t j = 0; j < n_insertions; ++j) {
                ASSERT_TRUE(database_create_leaf(db, dirs[j % NDIRS], "",
                                                 INT, (Value){ .int_value = (int)j }));
            }
        });
        fprintf(stderr, "%7lu insertions total time: %10f ms, average time: %8f ms\n",
                n_insertions, total * 1000., total / (double)n_insertions * 1000.);
//        database_traverse_and_print_database(db);
        for (int j = 0; j < NDIRS; ++j) {
            database_clear_directory(db, dirs[j]);
        }
//        database_traverse_and_print_database(db);
    }

    database_destroy_database(db);
}

void benchmark_deletions() {
    fprintf(stderr, "Benchmarking many deletions...\n");

    Database* db = database_create_database("benchmark_deletions", 1UL << 32);
    size_t const NDIRS = 3;
    Directory* dirs[NDIRS];
    dirs[0] = database_create_directory(db, NULL, "dir0");
    dirs[1] = database_create_directory(db, NULL, "dir1");
    dirs[2] = database_create_directory(db, NULL, "dir2");

    for (size_t i = 1, stop = 10; i < 7; ++i, stop *= 10) {
        size_t n_insertions = stop * NDIRS;
        assert(n_insertions <= INT_MAX);
        size_t n_deletions = n_insertions / 10;

        Node** leafs = malloc(sizeof(Node*) * n_deletions);

        for (size_t j = 0; j < n_insertions; ++j) {
            Node* res = database_create_leaf(db, dirs[j % NDIRS], "",
                                             INT, (Value){ .int_value = (int)j });
            if (j % 10 == 0) {
                leafs[j / 10] = res;
            }
        }
        double total = BENCHMARK_EXEC_TIME({
            for (size_t j = 0; j < n_deletions; ++j) {
                ASSERT_TRUE(database_delete_leaf(db, leafs[j]));
            }
        });
        fprintf(stderr, "%7lu deletions from %8lu elements total time: %10f ms, average time: %8f ms\n",
                n_deletions, n_insertions, total * 1000., total / (double)n_deletions * 1000.);
//        database_traverse_and_print_database(db);
        for (int j = 0; j < NDIRS; ++j) {
            database_clear_directory(db, dirs[j]);
        }
//        database_traverse_and_print_database(db);
        free(leafs);
    }

    size_t n_insertions = 10000000 * NDIRS;
    assert(n_insertions <= INT_MAX);
    Node** leafs = malloc(sizeof(Node*) * (n_insertions / 10));

    for (size_t j = 0; j < n_insertions; ++j) {
        Node* res = database_create_leaf(db, dirs[j % NDIRS], "",
                                         INT, (Value){ .int_value = (int)j });
        if (j % 10 == 0) {
            leafs[j / 10] = res;
        }
    }

    for (size_t i = 0, stop = 1; i < 7; ++i, stop *= 10) {
        size_t n_deletions = stop * NDIRS;

        double total = BENCHMARK_EXEC_TIME({
            for (size_t j = 0; j < n_deletions; ++j) {
                ASSERT_TRUE(database_delete_leaf(db, leafs[j]));
            }
        });
        fprintf(stderr, "%7lu deletions from %8lu elements total time: %10f ms, average time: %8f ms\n",
                n_deletions, n_insertions, total * 1000., total / (double)n_deletions * 1000.);
//        database_traverse_and_print_database(db);
        for (size_t j = 0; j < n_deletions; ++j) {
            leafs[j] = database_create_leaf(db, dirs[(j * 10) % NDIRS], "",
                                            INT, (Value){ .int_value = (int)(j * 10) });
        }
//        database_traverse_and_print_database(db);
    }
    free(leafs);
    database_destroy_database(db);
}

void benchmark_updates() {
    fprintf(stderr, "Benchmarking many updates...\n");

    Database* db = database_create_database("benchmark_updates", 1UL << 32);
    size_t const NDIRS = 3;
    Directory* dirs[NDIRS];
    dirs[0] = database_create_directory(db, NULL, "dir0");
    dirs[1] = database_create_directory(db, NULL, "dir1");
    dirs[2] = database_create_directory(db, NULL, "dir2");

    size_t n_elements = 1000000 * NDIRS;
    assert(n_elements <= INT_MAX);
    Node** leafs = malloc(sizeof(Node*) * n_elements);

    for (size_t j = 0; j < n_elements; ++j) {
        leafs[j] = database_create_leaf(db, dirs[j % NDIRS], "",
                                        INT, (Value){ .int_value = (int)j });
    }

    for (size_t i = 0, n_updates = 1; i < 7; ++i, n_updates *= 10) {
        srand(i);
        double total = BENCHMARK_EXEC_TIME({
            for (size_t j = 0; j < n_updates; ++j) {
                size_t id = rand() % n_elements; // NOLINT(*-msc50-cpp)
//                fprintf(stderr, "Updating leaf %lu\n", id);
                ASSERT_TRUE(database_update_leaf(db, leafs[id], (Value){.int_value = -(int)id }));
            }
        });
//        database_traverse_and_print_database(db);
        fprintf(stderr, "%7lu updates from %7lu elements total time: %10f ms, average time: %8f ms\n",
                n_updates, n_elements, total * 1000., total / (double)n_updates * 1000.);
    }
    free(leafs);
    database_destroy_database(db);
}

void benchmark_accesses() {
    fprintf(stderr, "Benchmarking many accesses...\n");

    Database* db = database_create_database("benchmark_accesses", 1UL << 32);
    size_t const NDIRS = 3;
    Directory* dirs[NDIRS];
    dirs[0] = database_create_directory(db, NULL, "dir0");
    dirs[1] = database_create_directory(db, NULL, "dir1");
    dirs[2] = database_create_directory(db, NULL, "dir2");

    size_t n_elements = 10000000 * NDIRS;
    assert(n_elements <= INT_MAX);
    Node** leafs = malloc(sizeof(Node*) * n_elements);

    for (size_t j = 0; j < n_elements; ++j) {
        leafs[j] = database_create_leaf(db, dirs[j % NDIRS], "",
                                        INT, (Value){ .int_value = (int)j });
    }

    for (size_t i = 0, n_accesses = 1; i < 8; ++i, n_accesses *= 10) {
        srand(i);
        double total = BENCHMARK_EXEC_TIME({
            for (size_t j = 0; j < n_accesses; ++j) {
                size_t id = rand() % n_elements; // NOLINT(*-msc50-cpp)
                EXPECT_TRUE(database_get_leaf_value(db, leafs[id])->int_value == id);
            }
        });
        fprintf(stderr, "%8lu accesses from %8lu elements total time: %11f ms, average time: %8f ms\n",
                n_accesses, n_elements, total * 1000., total / (double)n_accesses * 1000.);
    }
    free(leafs);
    database_destroy_database(db);
}

int main() {
    benchmark_insertions();
    fprintf(stderr, "\n");
    benchmark_deletions();
    fprintf(stderr, "\n");
    benchmark_updates();
    fprintf(stderr, "\n");
    benchmark_accesses();
    return 0;
}

#pragma clang diagnostic pop
