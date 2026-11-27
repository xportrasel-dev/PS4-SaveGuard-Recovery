# PS4 SaveGuard Recovery

Prototype for PS4 firmware 11.00 / GoldHEN. Current development work is on the `main` branch.

## Recovery limits

- The scanner inventories save directories and encrypted save-file pairs that still exist on the console. It can identify orphaned directories and incomplete or ambiguous pairs.
- It does not perform sector-level undelete. If PS4 Settings removed the save directory and no cloud, USB, or console backup exists, recovery is not established by this prototype.
- PS4 encrypted saves are console/account data. Raw snapshots are checksummed copies for preservation; restore compatibility has not been validated.
- Keep the console idle while checking for existing backups. Do not launch games or perform further SaveGuard install/backup/delete tests before preserving the current drive state.

## PIN-protected deletion

- One PIN record is stored in the shared console path `/data/SaveGuard/SaveGuard-pin.bin`, so SaveGuard uses the same PIN for every local PS4 user.
- Deletion through SaveGuard requires a complete scanned save pair, a PIN check for each deletion, and a successful checksummed snapshot of that user's game-save directory to the selected USB drive before the system save-data delete request.
- If the PIN is missing, the selected save is incomplete, the USB is unavailable, or snapshot verification fails, SaveGuard refuses the deletion.
- This protection applies only to deletion initiated inside SaveGuard. It does not intercept the PS4 Settings menu or other applications; system-wide deletion blocking is not implemented.

## Backups and restore

Use PS4's supported cloud or USB saved-data backups when available. A full-console backup restore erases current console data, so verify the backup and its contents before selecting Restore PS4. Apollo Save Tool can import/export saves that still exist or are already available as files; it is not a deleted-sector recovery tool.

## Dashboard typography and controller icons

The bitmap text was replaced with a smooth, anti-aliased DejaVu Sans Bold atlas, generated at build time and bundled with the app. The PS4 glyphs come from [flat-gamepad-icons](https://github.com/RobTheFiveNine/flat-gamepad-icons) by RobTheFiveNine, licensed CC BY-SA 4.0. The artwork source and font/license notices are retained in `assets/`.

## Build status

- The prototype PKG with smooth typography and full-color controller glyphs compiled successfully in GitHub Actions on November 27, 2026 (commit `c6f96267fbcc916cb64b26b191dcc621a1eeb98a`, run #49). The workflow artifact is `SaveGuard-prototype` (18 MB; ZIP SHA-256 `49e2bed00320904be6ecffdf5526329fedac3488673bd8bce434927082cb30cf`).
- Native dashboard compilation and safety tests passed in GitHub Actions run #75 with the generated font and icon atlases.
- The user successfully opened SaveGuard on a PS4, but screenshots show `USB NOT READY`, `FILES 0 / ERRORS 1` on SCAN FILES, and a subsequent application crash `CE-34878-0`.
- Installation and runtime behavior are unresolved. Do not treat this artifact as a usable release.
- The prototype does not perform sector-level undelete or restore a staged snapshot back into an active PS4 save-data slot.
