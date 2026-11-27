# PS4 Save Guard & Recovery

Target: PS4 firmware 11.00, GoldHEN 2.4.

## Current status
Only upstream Apollo baseline build has succeeded. No custom executable is available yet.

## Implementation milestones
1. Separate application identity; preserve Apollo installation and data.
2. Controller-driven graphical interface for read-only discovery of existing saves, orphan files, Apollo backups and savedata database metadata.
3. Export to USB with integrity verification; never overwrite source files.
4. Versioned backups and explicit restore confirmation.
5. Investigate system-level deletion interception and PIN authentication without interfering with normal game writes.
6. Investigate offline raw filesystem recovery on a disk image. Do not claim deleted-save recovery from database metadata alone.

## Safety
Do not install experimental builds on the disk containing deleted saves before preserving its contents. No raw disk writes. A 32 GB USB device cannot hold a complete 500 GB disk image. System hooks and deep recovery remain unimplemented and require hardware testing.

## Attribution
Upstream Apollo: https://github.com/bucanero/apollo-ps4 . Retain upstream licensing and attribution in derivative builds.
