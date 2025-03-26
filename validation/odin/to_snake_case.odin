package lmalloc

import "core:fmt"
import "core:strings"
import "core:os"

main :: proc()
{
    in_file, oerr := os.open("./in_file.c", os.O_RDONLY)
    if oerr != os.ERROR_NONE {
        fmt.println("Error opening input file!", oerr)
        return
    } else {
        fmt.println("Opened input file!")
    }
    defer os.close(in_file)

    RWE_PERMISSION :: 0o755
    out_file, oerr2 := os.open("./out_file.c", os.O_RDWR, RWE_PERMISSION);
    if oerr2 != os.ERROR_NONE {
        fmt.println("Error opening output file!", oerr2)
        return
    } else {
        fmt.println("Opened output file!")
    }
    defer os.close(out_file)

    in_data, oerr3:= os.read_entire_file_from_handle(in_file)
    in_string := strings.clone_from_bytes(in_data)
    fmt.println(in_string)

    builder: strings.Builder
    strings.builder_init_none(&buidler)

    out_string := strings.to_snake_case(in_string)
    os.write_string(out_file, out_string)
}
