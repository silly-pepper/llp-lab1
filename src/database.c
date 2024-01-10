#include "allocator.h"
#include "database.h"
#include "internals.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Database {
    Allocator* allocator;
    Node* root;
};

Node* create_node(Allocator* allocator, Types type, uint64_t name_len, char const* name) {
    Node* res = (Node*) alloc_malloc(allocator, sizeof(Node));
    if (!res) return NULL;
    res->type = type;
    res->next = NULL;
    res->prev = NULL;
    res->child = NULL;
    if (name) {
        res->name = (char*) alloc_malloc(allocator, name_len + 1);
        if (!res->name) {
            alloc_free(allocator, res);
            return NULL;
        }
        memcpy(res->name, name, name_len);
        *(res->name + name_len) = '\0';
    }
    return res;
}

Node* create_dir_node(Allocator* allocator, uint64_t name_len, char const* name) {
    return create_node(allocator, DIR, name_len, name);
}

Node* create_leaf_node(Allocator* allocator, Types type, char const* name, Value value) {
    Node* res = create_node(allocator, type, strlen(name), name);
    if (!res) return NULL;
    if (type == STR) {
        char* cpy = (char*) alloc_malloc(allocator, value.str_value.size + 1);
        if (!cpy) {
            alloc_free(allocator, res->name);
            alloc_free(allocator, res);
            return NULL;
        }
        memcpy(cpy, value.str_value.data, value.str_value.size);
        *(cpy + value.str_value.size) = '\0';
        value.str_value.data = cpy;
    }
    res->data = value;
    return res;
}

Database* database_create_database(char const* filename, size_t initial_size) {
    if (initial_size == 0) {
        initial_size = 1ULL << 31; // 2GB
    }
    Database* res = (Database*) malloc(sizeof(Database));
    if (!res) return NULL;
    res->allocator = alloc_create(filename, initial_size);
    if (!res->allocator) return NULL;
    res->root = create_dir_node(res->allocator, 0, NULL);
    if (!res->root) return NULL;
    return res;
}

// does not erase any data
void database_shutdown_database(Database* ptr) {
    alloc_destroy(ptr->allocator);
    free(ptr);
}

void database_destroy_database(Database* ptr) {
//    fprintf(stderr, "\nDestroying database...\n");
    database_clear_directory(ptr, ptr->root);
    alloc_free(ptr->allocator, ptr->root);
    database_shutdown_database(ptr);
}

Directory* database_create_directory(Database* db, Directory* parent, char const* name) {
    if (!parent) parent = db->root;
    if (parent->type != DIR) return NULL;
    Node* res = create_dir_node(db->allocator, strlen(name), name); // todo check for existing name?
    if (!res) return NULL;
    if (parent->child) parent->child->prev = res;
    res->next = parent->child;
    parent->child = res;
    res->prev = parent;
    return res;
}

Leaf* database_create_leaf(Database* db, Directory* parent, char const* name,
                           Types type, Value value) { // todo remove copy-paste
    if (!parent) parent = db->root;
    if (parent->type != DIR || type == DIR) return NULL; // todo check type is correct
    Node* res = create_leaf_node(db->allocator, type, name, value);
    if (!res) return NULL;
    if (parent->child) parent->child->prev = res;
    res->next = parent->child;
    parent->child = res;
    res->prev = parent;
    return res;
}

bool database_update_leaf(Database* db, Leaf* leaf, Value new_value) {
    if (!leaf) return false;
    if (leaf->type == DIR) return false;
    if (leaf->type == STR) {
        char* cpy = (char*) alloc_malloc(db->allocator, new_value.str_value.size + 1);
        if (!cpy) return false;
        memcpy(cpy, new_value.str_value.data, new_value.str_value.size);
        *(cpy + new_value.str_value.size) = '\0';
        new_value.str_value.data = cpy;
    }
    leaf->data = new_value;
    return true;
}

bool delete_node(Allocator* allocator, Node* ptr) {
    if (!ptr) return false;
    if (ptr->next) {
        ptr->next->prev = ptr->prev;
    }
    assert(ptr->prev); // not root
    if (ptr->prev->child == ptr) { // ptr->prev is our parent
        ptr->prev->child = ptr->next;
    } else {
        ptr->prev->next = ptr->next;
    }
    if (ptr->type == STR) {
        alloc_free(allocator, ptr->data.str_value.data);
    }
    alloc_free(allocator, ptr->name);
    alloc_free(allocator, ptr);
    return true;
}

bool database_delete_directory(Database* db, Directory* ptr) {
    if (!db || !ptr) return false;
    if (ptr->type != DIR) return false;
    if (ptr->child) return false; // not empty
    if (ptr == db->root) return false; // do not delete root
    return delete_node(db->allocator, ptr);
}

bool database_delete_leaf(Database* db, Leaf* ptr) {
    if (!db || !ptr) return false;
    if (ptr->type == DIR) return false;
    return delete_node(db->allocator, ptr);
}

void clear_dir_dfs(Database* db, Directory* dir) { // NOLINT(*-no-recursion)
    Iterator it = database_get_directory_content_iterator(db, dir);
    if (!iterator_is_valid(&it)) { // todo implement method "has_children" or "is_empty"
        // empty dir
        return;
    }
    do {
        Types type = iterator_get_type(&it);
        if (type == DIR) {
            clear_dir_dfs(db, it._ptr);
        }
        Node* next = it._ptr->next;
        delete_node(db->allocator, it._ptr);
        it._ptr = next;
    } while (iterator_is_valid(&it));
}

void database_clear_directory(Database* db, Directory* dir) {
    if (!db) return;
    if (!dir) dir = db->root;
    clear_dir_dfs(db, dir);
}

Directory* database_get_root_directory(Database* db) {
    return db->root;
}

Value const* database_get_leaf_value(Database const* db, Leaf const* leaf) {
    if (!leaf || leaf->type == DIR) return NULL;
    return &leaf->data;
}

Iterator database_get_directory_content_iterator(Database const* db, Directory const* dir) {
    if (!dir) dir = db->root;
    Iterator res;
    res._ptr = (dir->type == DIR ? dir->child : NULL);
    return res;
}

void traverse_and_print(Database const* db, Directory const* dir, size_t depth) { // NOLINT(*-no-recursion)
    size_t sep_len = 4; // "|-- "
    size_t psz = (depth + 1) * sep_len;
    char prefix[psz];
    memset(prefix, ' ', psz);
    strcpy(prefix + psz - sep_len, "|-- ");

    Iterator it = database_get_directory_content_iterator(db, dir);
    if (!iterator_is_valid(&it)) {
        memset(prefix, ' ', psz);
        fprintf(stderr, "%s empty dir\n", prefix);
        return;
    }
    do {
        assert(iterator_is_valid(&it));
        fprintf(stderr, "%s%s", prefix, iterator_get_name(&it));
        Types type = iterator_get_type(&it);
        if (type == DIR) {
            fprintf(stderr, ":\n");
            traverse_and_print(db, iterator_get(&it), depth + 1);
        } else {
            Value const* value = iterator_get_value(&it);
            if (type == INT) {
                fprintf(stderr, "=%d\n", value->int_value);
            } else if (type == STR) {
                fprintf(stderr, "=%s (len=%lu)\n", value->str_value.data, value->str_value.size);
            } else if (type == FLOAT) {
                fprintf(stderr, "=%f\n", value->float_value);
            } else if (type == BOOL) {
                fprintf(stderr, "=%s\n", (value->bool_value ? "true" : "false"));
            } else {
                fprintf(stderr, " UNREACHABLE. Invalid type: %d\n", type);
                break;
            }
        }
    } while (iterator_next(&it));
}

void database_traverse_and_print_database(Database const* db) {
    if (!db) return;
    fprintf(stderr, "Traversing database...\n");
    traverse_and_print(db, db->root, 0);
}
