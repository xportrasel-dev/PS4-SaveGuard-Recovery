# PS4 SaveGuard Recovery

Prototype for PS4 firmware 11.00 / GoldHEN. Current development work is on the `main` branch.

## Recovery limits

- The scanner inventories save directories and encrypted save-file pairs that still exist on the console. It can identify orphaned directories and incomplete or ambiguous pairs.
- It does not perform sector-level undelete. If PS4 Settings removed the save directory and no cloud, USB, or console backup exists, recovery is not established by this prototype.
- PS4 encrypted saves are console/account data. Raw snapshots are checksummed copies for preservation; restore compatibility has not been validated.
- Keep the console idle while checking for existing backups. Do not install this prototype or launch games before preserving the current drive state.

## PIN-protected deletion

- One PIN record is stored in the shared console path `/data/SaveGuard/SaveGuard-pin.bin`, so SaveGuard uses the same PIN for every local PS4 user.
- Deletion through SaveGuard requires a complete scanned save pair, a PIN check for each deletion, and a successful checksummed snapshot of that user's game-save directory to the selected USB drive before the system save-data delete request.
- If the PIN is missing, the selected save is incomplete, the USB is unavailable, or snapshot verification fails, SaveGuard refuses the deletion.
- This protection applies only to deletion initiated inside SaveGuard. It does not intercept the PS4 Settings menu or other applications; system-wide deletion blocking is not implemented.

## Backups and restore

Use PS4's supported cloud or USB saved-data backups when available. A full-console backup restore erases current console data, so verify the backup and its contents before selecting Restore PS4. Apollo Save Tool can import/export saves that still exist or are already available as files; it is not a deleted-sector recovery tool.

## Build status

- The latest prototype PKG compiled successfully in GitHub Actions on November 27, 2026 (commit `db775a136b5e5c95aacb0aa0a5af8911ff22b8af`). The workflow artifact is `SaveGuard-prototype` (17.6 MB; SHA-256 `9cb67005c18bab0d816803c4bcda7588c66b5d81fbbb43dbb3d0aa466c89cabd`).
- Native safety tests for the same commit passed in GitHub Actions.
- A PS4 install attempt for this PKG returned `CE-34706-0` (`0x80F00100`, package header format error). The extracted PKG has the expected PS4 PKG magic and matching content ID, but that does not validate the package on-console.
- Installation and runtime behavior remain unresolved. Do not treat this artifact as a usable release, and do not launch it on the console until the install failure is resolved.
- The prototype does not yet perform sector-level undelete or restore a staged snapshot back into an active PS4 save-data slot.
