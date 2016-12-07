# AMD AGS SDK Changelog

**Note:** See `ags_lib\CHANGELOG.md` for changes to the core AGS library.

### v5.0.5 - 2016-12-07
* Update samples for latest AGS API

### v4.0.3 - 2016-08-18
* Fix up crossfire_sample for latest AGS API

### v4.0.0 - 2016-05-24
* Fix up crossfire_sample now that extension API calls specify DX11 or DX12

### v3.2.2 - 2016-05-23
* Display `radeonSoftwareVersion` in ags_sample again, now that updated driver is public
  * Radeon Software Crimson Edition 16.5.2 or later

### v3.2.0 - 2016-02-12
* Update ags_sample to use the library version number from the optional info parameter of `agsInit`
* Update crossfire_sample to call `agsInit` prior to device creation
* Update eyefinity_sample with latest DXUT

### v3.1.1 - 2016-01-28
* Display library version number in ags_sample
* Do not display `radeonSoftwareVersion` in ags_sample while we wait for a needed driver update
* Set `WindowsTargetPlatformVersion` to 8.1 for VS2015, to eliminate unnecessary dependency on optional Windows 10 SDK

### v3.1.0 - 2016-01-26
* Initial release on GitHub
