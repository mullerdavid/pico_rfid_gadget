# Visual Studio Code environment

## Workspace settings
Modify the hardcoded paths in the following files. Also set the board if applicable.
- `.vscode/`
    - `settings.json`
        - `cmake.cmakePath`
        - `cmake.environment`
    - `cmake-kits.json`
        - `toolchainFile`
        - `environmentSetupScript`

## Open and Configure
Open the folder. Install the recommended extensions. 
Cmake is automatically configuring the project on open after the extension is installed.
If the Kit is not selected, select Pico ARM GCC (don't use builtin GCC).

