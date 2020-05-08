# DRP Driver for RZ/A2M
This new DRP driver reduces the overhead time between DRP-execution-end and next DRP-execution-start.

# Features
- asynchronous DRP operation start API
- individual callback function for each DRP operation start API
![drpDrv](https://user-images.githubusercontent.com/48343451/81360087-e1fcc000-9115-11ea-99bb-c2966c75e83d.png)

# Requirement
- [Renesas RZ/A2M FreeRTOS software development kit](https://www.renesas.com/jp/ja/products/software-tools/software-os-middleware-driver/software-package/rza2-software-development-kit-free-rtos.html)
- [Renesas e2studio](https://www.renesas.com/jp/ja/products/software-tools/tools/ide/e2studio.html)
- RZ/A2M Evaluation Board: [SEMB1402](http://www.shimafuji.co.jp/products/1505), [SBEV-RZ/A2M](http://www.shimafuji.co.jp/products/1486), [RZ/A2M Eva-Lite](http://www.shimafuji.co.jp/products/1767)

# Installation
- install Renesas e2studio
- inport program from Renesas RZ/A2M FreeRTOS software development kit
- update following files
```
directry: generate/sc_driver/r_drp/

inc/
  r_dk2_if.h      : add function ptorotypes to original file
src/
  sf_drp.c        : this new DRP driver
  sf_drp.h        : header
  r_dk2_if.c      : add functions to original file
  drp_iodefine.h  : modify macros to original file
  r_dk2_core.c    : same as original file
  r_dk2_code.h    : same as original file
```

# Usage
APIs of this DRP driver:
```
sf_DK2_Initialize(void)

  - Initialize ONLY this driver
  - You should also call original R_DK2_Initialise().
```
```
int32_t sf_DK2_Load(pconfig, top_tiles, tile_pattern, pload, pint, &paid)

  - Loads configuration data in DRP
  - You should call this API instead of original R_DK2_Load().
  - Arguments are same as original DRP driver API.
  - The argument "pload"(callback function at DRP configuration completed)
  - and "pint"(callback function at DRP execution completed) are ignored.
```
```
int32_t sf_DK2_Start2(id, &pparam, size, pint)

  - Starts operation of circuit in DRP
  - You should call this API instead of original R_DK2_Start().
  - The Argument is different from the original API,
  - "pint"(callback function at DRP execution completed) is added.
```

# Note
- The Argument of "sf_DK2_Start2" is different from the original API.
- You can still use original API "R_DK2_Active" and "R_DK2_Inactive" and "R_DK2_Start",
- and "R_DK2_Unload" are available for this new DRP driver's API.

# Auther
- [Shimafuji Electroc inc.](http://www.shimafuji.co.jp/)
- E-mail: shimafuji-fvio@shimafuji.co.jp

# License
"DRP Driver for RZ/A2M" is under [MIT license](https://en.wikipedia.org/wiki/MIT_License).
