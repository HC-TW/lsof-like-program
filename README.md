# lsof-like-program
## Program Arguments
- **`-c REGEX`**: a regular expression (REGEX) filter for filtering command line. For example **`-c sh`** would match **`bash`**, **`zsh`**, and **`share`**.
- **`-t TYPE`**: a TYPE filter. Valid TYPE includes **`REG`**, **`CHR`**, **`DIR`**, **`FIFO`**, **`SOCK`**, and **`unknown`**. TYPEs other than the listed should be considered as invalid. For invalid types, print out an error message **`Invalid TYPE option.`** in a single line and terminate the program.
- **`-f REGEX`**: a regular expression (REGEX) filter for filtering filenames.
