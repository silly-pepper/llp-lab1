#ifndef LLP_LAB1_DATABASE_H
#define LLP_LAB1_DATABASE_H

#include "types.h"
#include "database_iterator.h"

#include <stddef.h>

typedef struct Database Database;
typedef struct Node Directory;
typedef struct Node Leaf;

Database* database_create_database(char const* filename, size_t initial_size);

void database_shutdown_database(Database* ptr); // does not erase any data
void database_destroy_database(Database* ptr); // clears all data before shutting down

Directory* database_get_root_directory(Database* db);

Directory* database_create_directory(Database* db, Directory* parent, char const* name);
bool database_delete_directory(Database* db, Directory* ptr);
void database_clear_directory(Database* db, Directory* dir);

Leaf* database_create_leaf(Database* db, Directory* parent, char const* name,
                           Types type, Value value);
bool database_update_leaf(Database* db, Leaf* leaf, Value new_value);
Value const* database_get_leaf_value(Database const* db, Leaf const* leaf);
bool database_delete_leaf(Database* db, Leaf* ptr);

Iterator database_get_directory_content_iterator(Database const* db, Directory const* dir);

void database_traverse_and_print_database(Database const* db);

#endif //LLP_LAB1_DATABASE_H
