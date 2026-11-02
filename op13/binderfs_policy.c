/* SPDX-License-Identifier: Apache-2.0
 * Add private BinderFS labels and the exact DRM operations used by this
 * port's composer/allocator and Mesa software path. No permissive settings,
 * new types or unrelated access changes.
 * Linked against the open-source libsepol built in the local workspace.
 */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sepol/policydb.h>
#include <sepol/kernel_to_cil.h>
#include <sepol/policydb/policydb.h>
#include <sepol/policydb/context.h>

static void dump(policydb_t *policy, const char *path)
{
    FILE *file = fopen(path, "wx");
    if (!file || sepol_kernel_policydb_to_cil(file, policy) || fclose(file)) {
        fprintf(stderr, "policy dump failed: %s\n", path);
        exit(1);
    }
}

static avtab_key_t drm_key(policydb_t *policy, const char *source, uint16_t specified)
{
    type_datum_t *from = hashtab_search(policy->p_types.table, source);
    type_datum_t *to = hashtab_search(policy->p_types.table, "dev_dri_file");
    class_datum_t *class = hashtab_search(policy->p_classes.table, "chr_file");
    if (!from || !to || !class) exit(1);
    return (avtab_key_t){.source_type=from->s.value, .target_type=to->s.value,
                         .target_class=class->s.value, .specified=specified};
}

static void allow_drm_map(policydb_t *policy, const char *source)
{
    avtab_key_t key = drm_key(policy, source, AVTAB_ALLOWED);
    class_datum_t *class = hashtab_search(policy->p_classes.table, "chr_file");
    perm_datum_t *permission = hashtab_search(class->permissions.table, "map");
    if (!permission && class->comdatum) permission = hashtab_search(class->comdatum->permissions.table, "map");
    avtab_datum_t *access = avtab_search(&policy->te_avtab, &key);
    if (!permission || !access || !permission->s.value || permission->s.value > 32) exit(1);
    access->data |= 1U << (permission->s.value - 1);
}

static void allow_drm_ioctl(policydb_t *policy, const char *source, unsigned function)
{
    avtab_key_t key = drm_key(policy, source, AVTAB_XPERMS_ALLOWED);
    for (avtab_ptr_t node = avtab_search_node(&policy->te_avtab, &key); node;
         node = avtab_search_node_next(node, AVTAB_XPERMS_ALLOWED)) {
        avtab_extended_perms_t *perms = node->datum.xperms;
        if (perms && perms->specified == AVTAB_XPERMS_IOCTLFUNCTION && perms->driver == 0x64) {
            perms->perms[function / 32] |= 1U << (function % 32);
            return;
        }
    }
    fprintf(stderr, "expected existing DRM ioctl allowlist for %s\n", source);
    exit(1);
}

int main(int argc, char **argv)
{
    if (argc != 5) {
        fprintf(stderr, "usage: binderfs-policy donor.policy candidate.policy before.cil after.cil\n");
        return 2;
    }
    sepol_policydb_t *db;
    sepol_policy_file_t *file;
    if (sepol_policydb_create(&db) || sepol_policy_file_create(&file)) return 1;
    FILE *input = fopen(argv[1], "rb");
    if (!input) return 1;
    sepol_policy_file_set_fp(file, input);
    if (sepol_policydb_read(db, file)) return 1;
    fclose(input);
    policydb_t *policy = &db->p;
    dump(policy, argv[3]);
    genfs_t **position = &policy->genfs;
    while (*position && strcmp((*position)->fstype, "binder") < 0) position = &(*position)->next;
    genfs_t *binder = *position;
    if (!binder || strcmp(binder->fstype, "binder")) {
        binder = calloc(1, sizeof(*binder));
        if (!binder || !(binder->fstype = strdup("binder"))) return 1;
        binder->next = *position;
        *position = binder;
    }
    // The root entry selects GENFS labeling for this filesystem. The tmpfs
    // context reuses the donor's existing association rules for RAM-backed
    // objects; the BinderFS mount itself is hidden before native init runs.
    const char *names[] = {"/", "/hmos-binder", "/hmos-hwbinder", "/hmos-vndbinder"};
    const char *types[] = {"tmpfs", "dev_binder_file", "dev_hwbinder_file", "dev_vndbinder_file"};
    user_datum_t *user = hashtab_search(policy->p_users.table, "u");
    role_datum_t *role = hashtab_search(policy->p_roles.table, "object_r");
    level_datum_t *level = hashtab_search(policy->p_levels.table, "s0");
    if (!user || !role || !level) return 1;
    for (size_t i = 0; i < 4; ++i) {
        for (ocontext_t *entry = binder->head; entry; entry = entry->next) {
            if (!strcmp(entry->u.name, names[i])) {
                fprintf(stderr, "private BinderFS path already exists; refuse implicit replacement\n");
                return 1;
            }
        }
        type_datum_t *type = hashtab_search(policy->p_types.table, types[i]);
        if (!type) return 1;
        ocontext_t *entry = calloc(1, sizeof(*entry));
        if (!entry || !(entry->u.name = strdup(names[i]))) return 1;
        context_init(&entry->context[0]);
        entry->context[0].user = user->s.value;
        entry->context[0].role = role->s.value;
        entry->context[0].type = type->s.value;
        entry->context[0].range.level[0].sens = level->level->sens;
        entry->context[0].range.level[1].sens = level->level->sens;
        if (!policydb_context_isvalid(policy, &entry->context[0])) return 1;
        entry->next = binder->head;
        binder->head = entry;
    }
    allow_drm_map(policy, "composer_host");
    allow_drm_map(policy, "render_service");
    allow_drm_ioctl(policy, "allocator_host", 0x11); // drmIsMaster: AUTH_MAGIC(0)
    allow_drm_ioctl(policy, "composer_host", 0xb3); // MAP_DUMB
    allow_drm_ioctl(policy, "render_service", 0x00); // VERSION
    allow_drm_ioctl(policy, "render_service", 0x0c); // GET_CAP
    allow_drm_ioctl(policy, "render_service", 0xb3); // MAP_DUMB
    FILE *output = fopen(argv[2], "wbx");
    if (!output) return 1;
    sepol_policy_file_set_fp(file, output);
    if (sepol_policydb_write(db, file) || fclose(output)) return 1;
    sepol_policydb_t *reloaded;
    if (sepol_policydb_create(&reloaded)) return 1;
    output = fopen(argv[2], "rb");
    if (!output) return 1;
    sepol_policy_file_set_fp(file, output);
    if (sepol_policydb_read(reloaded, file) || fclose(output)) return 1;
    dump(&reloaded->p, argv[4]);
    printf("Added 4 BinderFS labels and scoped DRM map/ioctl access; policy version remains %u\n", policy->policyvers);
    sepol_policydb_free(reloaded);
    sepol_policy_file_free(file);
    sepol_policydb_free(db);
    return 0;
}
