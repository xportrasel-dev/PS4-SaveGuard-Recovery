# PS4 SaveGuard Recovery

Prototype for PS4 firmware 11.00 / GoldHEN. Current development work is on the `main` branch.

## Recovery limits

- The scanner inventories save directories and encrypted save-file pairs that still exist on the console. It can identify orphaned directories and incomplete or ambiguous pairs.
- It does not perform sector-level undelete. If PS4 Settings removed the save directory and no cloud, USB, or console backup exists, recovery is not established by this prototype.
- PS4 encrypted saves are console/account data. Raw snapshots are checksummed copies for preservation; restore compatibility has not been validated.
- Keep the console idle while checking for existing backups. Do not install this prototype or launch games before preserving the current drive state.

## PIN-protected deletion

- One PIN record is stored in the shared console path `/data/SaveGuard/SaveGuard-pin.bin`, so SaveGuard uses the same PIN for every local PS4 user.
- Deletion through SaveGuard requires a complete scanned save pair, a PIN check for each deletion, and a successful checksummed snapshot of that user’s game-save directory to the selected USB drive before the system save-data delete request.
- If the PIN is missing, the selected save is incomplete, the USB is unavailable, or snapshot verification fails, SaveGuard refuses the deletion.
- This protection applies only to deletion initiated inside SaveGuard. It does not intercept the PS4 Settings menu or other applications; system-wide deletion blocking is not implemented.

## Backups and restore

Use PS4's supported cloud or USB saved-data backups when available. A full-console backup restore erases current console data, so verify the backup and its contents before selecting Restore PS4. Apollo Save Tool can import/export saves that still exist or are already available as files; it is not a deleted-sector recovery tool.

## Build status

The PKG build and on-console behavior for the latest changes have not yet been confirmed. No recovery success has been verified for deleted saves.
