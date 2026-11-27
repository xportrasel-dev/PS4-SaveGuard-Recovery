# PS4 save integration
Reference: https://github.com/bucanero/apollo-ps4/blob/master/include/saves.h and source/main.c (upstream inspected).
- HDD save database: /system_data/savedata/%08x/db/user/savedata.db
- HDD files: /user/home/%08x/savedata/
- Apollo uses ReadUserList with database path, not just directory enumeration.
- save_entry_t contains title_id, dir_name, path, blocks and flags.
- orbis_SaveMount(save, mode, mountPath) and orbis_SaveUmount(mountPath) are upstream interfaces. Mode semantics and implementation must be inspected before use.
- Apollo initializes SYSTEM_SERVICE, USER_SERVICE and SAVE_DATA modules and obtains current user from its pad configuration.
- Current SaveGuard raw snapshots do not establish mount compatibility or database registration.
## Integration gates
1. Read-only inventory of existing database and save directories, no database rebuild.
2. Inspect upstream mount implementation and dependencies; preserve licensing.
3. Test read-only mount and export on disposable saves only.
4. Restore into a new disposable slot using supported save APIs, verify game loading.
5. Never write to the original deleted-save disk during forensic acquisition. Existing database rows are not proof of recoverable deleted content.
6. System deletion interception and raw recovery remain separate unverified workstreams.
